// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleDef.h"

#include <sstream>
#include "spug/check.h"
#include "spug/stlutil.h"
#include "builder/Builder.h"
#include "util/SourceDigest.h"
#include "Context.h"
#include "Deserializer.h"
#include "ModuleStub.h"
#include "Serializer.h"
#include "StatState.h"
#include "Visitor.h"

using namespace std;
using namespace model;
using namespace crack::util;

void ModuleDef::getNestedTypeDefs(std::vector<TypeDef*> &typeDefs,
                                  ModuleDef *master
                                  ) {
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves)
        (*slave)->getTypeDefs(typeDefs, master);
}

ModuleDef::ModuleDef(const std::string &name, Namespace *parent) :
    VarDef(0, name),
    Namespace(name),
    parent(parent),
    master(0),
    finished(false),
    fromExtension(false),
    cacheable(false) {
}

ModuleDef::~ModuleDef() {
    // We're lazily assuming that the master won't be destroyed until all of
    // his slaves are, and this currently seems to be the case.  If it ceases
    // to be the case we need to take remedial steps to ensure that we don't
    // try to serialize the slaves without their master.
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves)
        SPUG_CHECK((*slave)->refcnt() == 1,
                   "Slave module " << (*slave)->getNamespaceName() <<
                    "would live on after deletion of its master."
                   );
}

bool ModuleDef::hasInstSlot() {
    return false;
}

void ModuleDef::addDependency(ModuleDef *other) {
    if (other != this &&
        dependencies.find(other->getNamespaceName()) == dependencies.end()
        )
        dependencies[other->getNamespaceName()] = other;
}

void ModuleDef::addSlave(ModuleDef *slave) {
    SPUG_CHECK(!slave->master,
               "Module " << slave->getNamespaceName() <<
               " is being added as a slave of " << getNamespaceName() <<
               " but it already has a master: " <<
               slave->master->getNamespaceName());
    slaves.push_back(slave);
    slave->master = this;
    slave->cacheable = false;
}

void ModuleDef::close(Context &context) {
    StatState sState(&context, ConstructStats::builder, this);
    context.builder.closeModule(context, this);
}

VarDef *ModuleDef::asVarDef() {
    return this;
}

NamespacePtr ModuleDef::getParent(unsigned index) {
    return index ? NamespacePtr(0) : parent;
}

NamespacePtr ModuleDef::getNamespaceOwner() {
    return owner;
}

ModuleDefPtr ModuleDef::getModule() {
    return this;
}

bool ModuleDef::isHiddenScope() {
    return false;
}

ModuleDef::StringVec ModuleDef::parseCanonicalName(const std::string &name) {
    StringVec result;

    // track the level of bracket nesting, we only split "outer" components.
    int nested = 0;
    int last = 0;

    int i;
    for (i = 0; i < name.size(); ++i) {
        if (!nested) {
            switch (name[i]) {
                case '.':
                    result.push_back(name.substr(last, i - last));
                    last = i + 1;
                    break;
                case '[':
                    ++nested;
                    break;
                case ']':
                    std::cerr << "Invalid canonical name: [" << name << "]" <<
                        std::endl;
                    assert(false);
                    break;
            }
        } else {
            switch (name[i]) {
                case '[':
                    ++nested;
                    break;
                case ']':
                    --nested;
                    break;
            }
        }
    }

    // add the last segment
    result.push_back(name.substr(last, i - last));
    return result;
}

string ModuleDef::joinName(const ModuleDef::StringVec &parts) {
    ostringstream result;

    bool first;
    for (StringVec::const_iterator i = parts.begin(); i < parts.end(); ++i) {
        if (!first) {
            result << '.';
            first = true;
        }
        result << *i;
    }

    return result.str();
}

#define CRACK_METADATA_V1 2271218416

void ModuleDef::serialize(Serializer &serializer) {
    int id = serializer.registerObject(this);
    SPUG_CHECK(id == 0,
               "Module id for serialized module " << getFullName() <<
                " is not 0: " << id
               );
    serializer.module = this;
    serializer.write(CRACK_METADATA_V1, "magic");

    // If we are a slave, just serialize a reference to the master.
    ModuleDefPtr master = getMaster();
    if (master.get() != this) {
        serializer.write(master->getFullName(), "master");
        return;
    } else {
        serializer.write("", "master");
    }

    // write source path and source digest
    serializer.write(sourcePath, "sourcePath");
    serializer.write(sourceDigest.asHex(), "sourceDigest");

    // write the dependencies
    serializer.write(dependencies.size(), "#deps");
    for (ModuleDefMap::const_iterator iter = dependencies.begin();
         iter != dependencies.end();
         ++iter
         ) {
        SPUG_CHECK(iter->second->master != this,
                   "module " << getFullName() << " has a dependency on "
                   "slave module " << iter->first
                   );
        serializer.write(iter->first, "canonicalName");
        serializer.write(iter->second->metaDigest.asHex(), "metaDigest");
    }

    // write all of the symbols
    serializer.digestEnabled = true;
    Namespace::serializeTypeDecls(serializer);
    Namespace::serializeDefs(serializer);

    // write all of the exports
    serializer.write(exports.size(), "#exports");
    for (std::map<std::string, bool>::iterator iter = exports.begin();
         iter != exports.end();
         ++iter
         )
        serializer.write(iter->first, "exports");

    // sign the metadata
    metaDigest = serializer.hasher.getDigest();
}

ModuleDefPtr ModuleDef::deserialize(Deserializer &deser,
                                    const string &canonicalName
                                    ) {
    if (Serializer::trace)
        cerr << ">>>> Deserializing module " << canonicalName << endl;
    if (deser.readUInt("magic") != CRACK_METADATA_V1)
        return 0;

    string master = deser.readString(Serializer::modNameSize, "master");
    if (master.size()) {
        // Make sure we have the master.   In theory, we cannot reference a
        // slave without having referenced the master.
        Construct &construct = *deser.context->construct;
        construct.getModule(master);

        // Now we should be able to load the slave, or it doesn't exist as a
        // slave any more.
        Construct::ModuleMap::iterator iter =
            construct.moduleCache.find(canonicalName);
        if (iter != construct.moduleCache.end())
            return iter->second;
        else
            return 0;
    }

    string sourcePath = deser.readString(Serializer::modNameSize, "sourcePath");
    SourceDigest recordedSourceDigest =
        SourceDigest::fromHex(deser.readString(Serializer::modNameSize,
                                               "sourceDigest"
                                               )
                              );

    // check the digest against that of the actual source file (if the source
    // file can be found)
    Construct::ModulePath modPath =
        deser.context->construct->searchSourcePath(sourcePath);
    if (modPath.found) {
        SourceDigest fileDigest = SourceDigest::fromFile(modPath.path);
        if (fileDigest != recordedSourceDigest) {
            if (Construct::traceCaching)
                cerr << "digests don't match for " << sourcePath <<
                    " got " << recordedSourceDigest.asHex() <<
                    "\n  current = " <<
                    fileDigest.asHex() << "\n  module: " <<
                    canonicalName << endl;
            return 0;
        }
    }

    // store a module placeholder so that if we end up cycling back to this
    // module, we don't recurse.
    ModuleStubPtr stub = new ModuleStub(canonicalName);
    pair<Construct::ModuleMap::iterator, bool> existing =
        deser.context->construct->moduleCache.insert(
            make_pair(canonicalName, stub)
        );

    // read and load the dependencies
    int count = deser.readUInt("#deps");
    for (int i = 0; i < count; ++i) {
        ModuleDefPtr mod =
            deser.context->construct->getModule(
                deser.readString(64, "canonicalName")
            );
        SourceDigest moduleDigest =
            SourceDigest::fromHex(deser.readString(64, "metaDigest"));

        // if the dependency isn't finished, don't do a depdendency check.
        if (!mod || !mod->finished)
            continue;

        // if the dependency has a different definition hash from what we were
        // built against, we have to recompile.
        if (mod->metaDigest != moduleDigest) {
            if (Construct::traceCaching)
                cerr << "meta digest doesn't match for dependency " <<
                    mod->getFullName() << ", need to rebuild " <<
                    canonicalName << "(depending on " <<
                    moduleDigest.asHex() <<
                    " current = " << mod->metaDigest.asHex() << ")" << endl;
            deser.context->construct->moduleCache.erase(existing.first);
            return 0;
        }
    }

    // The cached meta-data is up-to-date.

    // deserialize the actual code through the builder.
    ModuleDefPtr mod =
        deser.context->builder.materializeModule(*deser.context, canonicalName,
                                                 0 // owner
                                                 );

    // storing the module in the construct cache - this is actually also done
    // later within construct, but we need the module to be present while
    // we're constructing it so we can resolve types by name when building
    // them.
    deser.context->construct->moduleCache[canonicalName] = mod;

    // register the module as id 0.
    deser.registerObject(0, mod.get());

    deser.context->ns = mod.get();
    deser.digestEnabled = true;
    mod->deserializeTypeDecls(deser);
    mod->deserializeDefs(deser);

    // deserialize exports
    int exportsCount = deser.readUInt("#exports");
    for (int i = 0; i < exportsCount; ++i)
        mod->exports[deser.readString(Serializer::varNameSize, "exports")] =
            true;

    mod->metaDigest = deser.hasher.getDigest();
    mod->sourcePath = sourcePath;
    mod->sourceDigest = recordedSourceDigest;
    mod->onDeserialized(*deser.context);

    if (Serializer::trace)
        cerr << ">>>> Finished deserializing module " << canonicalName << endl;
    return mod;
}

void ModuleDef::serializeSlaveRef(Serializer &serializer) {
    if (serializer.writeObject(this, "owner"))
        serializer.write(canonicalName, "canonicalName");
}

namespace {
    struct SlaveModuleReader : public Deserializer::ObjectReader {
        ModuleDefPtr master;
        SlaveModuleReader(ModuleDef *master) : master(master) {}
        virtual spug::RCBasePtr read(Deserializer &deser) const {
            string name = deser.readString(Serializer::modNameSize,
                                           "canonicalName"
                                           );
            ModuleDefPtr mod = deser.context->builder.materializeModule(
                *deser.context,
                name,
                master.get()
            );
            return mod;
        }
    };
}

ModuleDefPtr ModuleDef::deserializeSlaveRef(Deserializer &deser) {
    Deserializer::ReadObjectResult readObj =
        deser.readObject(SlaveModuleReader(this), "owner");
    return ModuleDefPtr::arcast(readObj.object);
}

void ModuleDef::replaceStubsInDefs(Context &context) {
    for (VarDefMap::iterator iter = defs.begin();
         iter != defs.end();
         ++iter
         )
        iter->second = iter->second->replaceAllStubs(context);
}

VarDefPtr ModuleDef::replaceAllStubs(Context &context) {
    if (stubFree)
        return this;
    stubFree = true;
    VarDefPtr replacement = replaceStub(context);
    if (replacement)
        return replacement;

    replaceStubsInDefs(context);
    return 0;
}

TypeDefPtr ModuleDef::getType(const string &name) {
    return lookUp(name);
}

void ModuleDef::visit(Visitor *visitor) {
    for (VarDefMap::iterator iter = defs.begin();
         iter != defs.end();
         ++iter
         )
        iter->second->visit(visitor);

    visitor->onModuleDef(this);
}

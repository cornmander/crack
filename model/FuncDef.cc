// Copyright 2009-2012 Google Inc.
// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "FuncDef.h"

#include <sstream>
#include "builder/Builder.h"
#include "spug/check.h"
#include "Deserializer.h"
#include "Context.h"
#include "ArgDef.h"
#include "Expr.h"
#include "OverloadDef.h"
#include "Serializer.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace model;
using namespace std;

FuncDef::FuncDef(Flags flags, const std::string &name, size_t argCount) :
    // function types are assigned after the fact.
    VarDef(0, name),
    flags(flags),
    args(argCount),
    vtableSlot(0) {
}

bool FuncDef::matches(Context &context, const vector<ExprPtr> &vals, 
                      vector<ExprPtr> &newVals,
                      FuncDef::Convert convertFlag
                      ) {
    ArgVec::iterator arg;
    vector<ExprPtr>::const_iterator val;
    int i;
    for (arg = args.begin(), val = vals.begin(), i = 0;
         arg != args.end() && val != vals.end();
         ++arg, ++val, ++i
         ) {
        switch (convertFlag) {
            case adapt:
            case adaptSecondary:
                if ((convertFlag == adapt || i) &&
                    (*val)->isAdaptive()
                    ) {
                    newVals[i] = (*val)->convert(context, (*arg)->type.get());
                    if (!newVals[i])
                        return false;
                } else if ((*arg)->type->matches(*(*val)->type)) {
                    newVals[i] = *val;
                } else {
                    return false;
                }
                break;
            case convert:
                newVals[i] = (*val)->convert(context, (*arg)->type.get());
                if (!newVals[i])
                    return false;
                break;
            case noConvert:
                if (!(*arg)->type->matches(*(*val)->type))
                    return false;
                break;
        }
    }

    // make sure that we checked everything in both lists
    if (arg != args.end() || val != vals.end())
        return false;
    
    return true;
}

bool FuncDef::matches(const ArgVec &other_args) {
    ArgVec::iterator arg;
    ArgVec::const_iterator other_arg;
    for (arg = args.begin(), other_arg = other_args.begin();
         arg != args.end() && other_arg != other_args.end();
         ++arg, ++other_arg
         )
        // if the types don't _exactly_ match, the signatures don't match.
        if ((*arg)->type.get() != (*other_arg)->type.get())
            return false;

    // make sure that we checked everything in both lists   
    if (arg != args.end() || other_arg != other_args.end())
        return false;
    
    return true;
}

bool FuncDef::matchesWithNames(const ArgVec &other_args) {
    ArgVec::iterator arg;
    ArgVec::const_iterator other_arg;
    for (arg = args.begin(), other_arg = other_args.begin();
         arg != args.end() && other_arg != other_args.end();
         ++arg, ++other_arg
         ) {
        // if the types don't _exactly_ match, the signatures don't match.
        if ((*arg)->type.get() != (*other_arg)->type.get())
            return false;
        
        // if the arg names don't exactly match, the signatures don't match.
        if ((*arg)->name != (*other_arg)->name)
            return false;
    }

    // make sure that we checked everything in both lists   
    if (arg != args.end() || other_arg != other_args.end())
        return false;

    return true;
}

bool FuncDef::isOverridable() const {
    return flags & virtualized || name == "oper init" || flags & forward;
}

unsigned int FuncDef::getVTableOffset() const {
    return vtableSlot;
}

bool FuncDef::hasInstSlot() {
    return false;
}

bool FuncDef::isStatic() const {
    return !(flags & method);
}

string FuncDef::getDisplayName() const {
    ostringstream out;
    display(out, "");
    return out.str();
}

string FuncDef::getUniqueId(Namespace *ns) const {
    ostringstream out;
    SPUG_CHECK(owner || ns, 
               "getUniqueId() called without a namespace or owner for "
                "function " << name
               );
    if (!ns) ns = owner;
    out << ns->getNamespaceName() << "." << name << "(";
    bool first = true;
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         ) {
        if (first)
            first = false;
        else
            out << ",";
        out << (*iter)->type->getFullName();
    }
    out << ")";
    return out.str();
}

bool FuncDef::isVirtualOverride() const {
    return flags & virtualized && receiverType != owner;
}

TypeDef *FuncDef::getThisType() const {
    return TypeDefPtr::cast(owner);
}

bool FuncDef::isConstant() {
    return true;
}

ExprPtr FuncDef::foldConstants(const vector<ExprPtr> &args) const {
    return 0;
}
    

void FuncDef::dump(ostream &out, const string &prefix) const {
    out << prefix << returnType->getFullName() << " " << getFullName() <<
        args << "\n";
}

void FuncDef::dump(ostream &out, const ArgVec &args) {
    out << '(';
    bool first = true;
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end(); 
         ++iter
         ) {
        if (!first)
            out << ", ";
        else
            first = false;
        out << (*iter)->type->getFullName() << ' ' << (*iter)->name;
    }
    out << ")";    
}

void FuncDef::display(ostream &out, const ArgVec &args) {
    out << '(';
    bool first = true;
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end(); 
         ++iter
         ) {
        if (!first)
            out << ", ";
        else
            first = false;
        out << (*iter)->type->getDisplayName() << ' ' << (*iter)->name;
    }
    out << ")";    
}

void FuncDef::display(ostream &out, const string &prefix) const {
    out << prefix << returnType->getDisplayName() << " " << 
        VarDef::getDisplayName();
    display(out, args);
}

void FuncDef::addDependenciesTo(ModuleDef *mod, VarDef::Set &added) const {
    mod->addDependency(getModule());
    returnType->addDependenciesTo(mod, added);
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        (*iter)->type->addDependenciesTo(mod, added);
}

bool FuncDef::isSerializable(const Namespace *ns, const string &name) const {
    return VarDef::isSerializable(ns, name) && !(flags & FuncDef::builtin);
}

void FuncDef::serializeArgs(Serializer &serializer) const {
    serializer.write(args.size(), "#args");
    for (ArgVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        (*iter)->serialize(serializer, false, 0);
}

void FuncDef::serialize(Serializer &serializer, bool writeKind,
                        const Namespace *ns
                        ) const {
    bool alias = owner != ns;
    serializer.write(alias ? 1 : 0, "isAlias");
    if (alias) {
        serializeExtern(serializer);
        serializeArgs(serializer);
        return;
    }
    
    returnType->serialize(serializer, false, 0);
    serializer.write(static_cast<unsigned>(flags), "flags");
    
    serializeArgs(serializer);
    
    if (flags & method) {
        ostringstream temp;
        Serializer sub(serializer, temp);
        // field id = 1 (<< 3) | type = 3 (reference)
        sub.write(11, "receiverType.header");
        receiverType->serialize(sub, false, 0);
        
        if (flags & virtualized) {
            // field id = 2 (<< 3) | type = 0 (varint)
            sub.write(16, "vtableSlot.header");
            sub.write(vtableSlot, "vtableSlot");
        }

        serializer.write(temp.str(), "optional");
    } else {
        serializer.write(0, "optional");
    }
}

FuncDef::ArgVec FuncDef::deserializeArgs(Deserializer &deser) {
    int argCount = deser.readUInt("#args");
    ArgVec args;
    for (int i = 0; i < argCount; ++i)
        args.push_back(ArgDef::deserialize(deser));
    return args;
}

FuncDefPtr FuncDef::deserialize(Deserializer &deser, const string &name) {
    bool alias = deser.readUInt("isAlias");
    if (alias) {
        OverloadDefPtr ovld = deserializeOverloadAliasBody(deser);
        return ovld->getSigMatch(deserializeArgs(deser), true);
    }
    TypeDefPtr returnType = TypeDef::deserialize(deser);
    Flags flags = static_cast<Flags>(deser.readUInt("flags"));

    ArgVec args = deserializeArgs(deser);    

    // read the optional data, the only field we're interested in is the 
    // receiverType
    string optionalDataString = deser.readString(256, "optional");
    TypeDefPtr receiverType;
    int vtableSlot = 0;
    if (optionalDataString.size()) {
        istringstream optionalData(optionalDataString);
        Deserializer sub(deser, optionalData);
        bool eof;
        int header;
        while ((header = sub.readUInt("optional.header", &eof)) || !eof) {
            switch (header) {
                case 11:
                    receiverType = TypeDef::deserialize(sub);
                    break;
                case 16:
                    vtableSlot = sub.readUInt("vtableSlot");
                    break;
                    
                default:
                    // this is just a field we don't know about.
                    break;
            }
        }
    }

    FuncDefPtr result = deser.context->builder.materializeFunc(
        *deser.context,
        flags,
        name,
        returnType.get(),
        args
    );
    
    result->receiverType = receiverType;
    result->vtableSlot = vtableSlot;

    return result;
}

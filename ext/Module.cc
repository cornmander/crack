// Copyright 2010 Google Inc.

#include "Module.h"

#include <string.h>
#include "builder/Builder.h"
#include "model/ConstVarDef.h"
#include "model/Context.h"
#include "model/FloatConst.h"
#include "model/IntConst.h"
#include "model/Namespace.h"
#include "Type.h"
#include "Func.h"

using namespace crack::ext;
using namespace model;

Module::Module(Context *context)  : context(context),
                                    finished(false) {
    memset(builtinTypes, 0, sizeof(builtinTypes));
}

Module::~Module() {
    int i;

    // cleanup the builtin types
    for (i = 0; i < sizeof(builtinTypes) / sizeof(Type *); ++i)
        delete builtinTypes[i];
    
    // cleanup new or looked up types
    for (TypeMap::iterator iter = types.begin(); iter != types.end(); ++iter)
         delete iter->second;
    
    // cleanup the funcs
    for (i = 0; i < funcs.size(); ++i)
        delete funcs[i];
}

#define GET_TYPE(capName, lowerName)                                        \
    Type *Module::get##capName##Type() {                                    \
        return builtinTypes[lowerName##Type] ?                              \
            builtinTypes[lowerName##Type] :                                 \
            (builtinTypes[lowerName##Type] =                                \
              new Type(this, context->construct->lowerName##Type.get()));   \
    }


GET_TYPE(Class, class)
GET_TYPE(Void, void)
GET_TYPE(Voidptr, voidptr)
GET_TYPE(Bool, bool)
GET_TYPE(Byteptr, byteptr)
GET_TYPE(Byte, byte)
GET_TYPE(Int32, int32)
GET_TYPE(Int64, int64)
GET_TYPE(Int, int)
GET_TYPE(Intz, intz)
GET_TYPE(Uint32, uint32)
GET_TYPE(Uint64, uint64)
GET_TYPE(Uint, uint)
GET_TYPE(Uintz, uintz)
GET_TYPE(Float32, float32)
GET_TYPE(Float64, float64)
GET_TYPE(Float, float)
GET_TYPE(VTableBase, vtableBase)
GET_TYPE(Object, object)
GET_TYPE(String, string)
GET_TYPE(StaticString, staticString)

Type *Module::getType(const char *name) {
    
    // try looking it up in the map, first
    TypeMap::iterator iter = types.find(name);
    if (iter != types.end())
        return iter->second;
    
    TypeDefPtr rawType = TypeDefPtr::rcast(context->ns->lookUp(name));
    assert(rawType && "Requested type not found");
    
    // Create a new type, add it to the map.
    Type *type = new Type(this, rawType.get());
    types[name] = type;

    return type;
}

Type *Module::addTypeWorker(const char *name, size_t instSize, bool forward) {

    TypeMap::iterator i = types.find(name);
    bool found = (i != types.end());
    if (found && !(i->second->typeDef && i->second->typeDef->forward))  {
        std::cerr << "Type " << name << " already registered!" << std::endl;
        assert(false);
    }

    Type *result;

    if (forward)
        result = new Type(this, name, context, instSize, context->createForwardClass(name).get());
    else
        result = new Type(this, name, context, instSize);

	types[name] = result;
    return result;
}

Type *Module::addType(const char *name, size_t instSize) {
    return Module::addTypeWorker(name, instSize, false);
}

Type *Module::addForwardType(const char *name, size_t instSize) {
    return Module::addTypeWorker(name, instSize, true);
}

Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr,
                      const char *symbolName) {
    assert(!finished && "Attempting to add a function to a finished module.");
    returnType->checkFinished();
    Func *f = new Func(context, returnType, name, funcPtr, Func::noFlags);
    if (symbolName)
        f->setSymbolName(symbolName);
    funcs.push_back(f);
    return f;
}

void Module::addConstant(Type *type, const std::string &name, double val) {
    type->checkFinished();
    FloatConstPtr valObj =
        context->builder.createFloatConst(*context, val, type->typeDef);
    vars.push_back(new ConstVarDef(type->typeDef, name, valObj));
}

void Module::addConstant(Type *type, const std::string &name, int64_t val) {
    type->checkFinished();
    IntConstPtr valObj =
        context->builder.createIntConst(*context, val, type->typeDef);
    vars.push_back(new ConstVarDef(type->typeDef, name, valObj));
}

void Module::finish() {
    // no-op if this is already finished
    if (finished)
        return;

    for (int i = 0; i < funcs.size(); ++i)
        funcs[i]->finish();
    
    // add the variable definitions to the context.
    for (int i = 0; i < vars.size(); ++i)
        context->ns->addDef(vars[i]);

    finished = true;
}

// Copyright 2010 Google Inc.

#include "Module.h"

#include <string.h>
#include "model/Context.h"
#include "Type.h"
#include "Func.h"

using namespace crack::ext;
using namespace model;

Module::Module(Context *context)  : context(context) {
    memset(builtinTypes, 0, sizeof(builtinTypes));
}

Module::~Module() {
    int i;

    // cleanup the builtin types
    for (i = 0; i < sizeof(builtinTypes) / sizeof(Type *); ++i)
        delete builtinTypes[i];
    
    // cleanup the funcs
    for (i = 0; i < funcs.size(); ++i)
        delete funcs[i];
}

#define GET_TYPE(capName, lowerName)                                    \
    Type *Module::get##capName##Type() {                                \
        return builtinTypes[lowerName##Type] ?                          \
            builtinTypes[lowerName##Type] :                             \
            (builtinTypes[lowerName##Type] =                            \
              new Type(context->globalData->lowerName##Type.get()));    \
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
GET_TYPE(Float32, float32)
GET_TYPE(Float64, float64)
GET_TYPE(Float, float)
GET_TYPE(VTableBase, vtableBase)
GET_TYPE(Object, object)
GET_TYPE(String, string)
GET_TYPE(StaticString, staticString)

Type *Module::getType(const char *name) {
    assert(0 && "not implemented");
}

Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr) {
    Func *f = new Func(this, returnType, name, funcPtr);
    funcs.push_back(f);
    return f;
}

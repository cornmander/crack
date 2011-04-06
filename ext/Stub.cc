// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Stub.h"

using namespace crack::ext;

// Func
void Func::setIsVariadic(bool isVariadic) { }
bool Func::isVariadic() { return false; }
void Func::setSymbolName(const std::string &name) { }
void Func::addArg(Type *type, const std::string &name) { }
void Func::finish() { }

// Module
Module::Module(model::Context *context) { }
Type *Module::getClassType() { }
Type *Module::getVoidType() { }
Type *Module::getVoidptrType() { }
Type *Module::getBoolType() { }
Type *Module::getByteptrType() { }
Type *Module::getByteType() { }
Type *Module::getInt32Type() { }
Type *Module::getInt64Type() { }
Type *Module::getUint32Type() { }
Type *Module::getUint64Type() { }
Type *Module::getIntType() { }
Type *Module::getUintType() { }
Type *Module::getFloat32Type() { }
Type *Module::getFloat64Type() { }
Type *Module::getFloatType() { }
Type *Module::getVTableBaseType() { }
Type *Module::getObjectType() { }
Type *Module::getStringType() { }
Type *Module::getStaticStringType() { }
Type *Module::getOverloadType() { }

Type *Module::getType(const char *name) { }
Type *Module::addType(const char *name) { }
Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr,
                      const char *symbolName) { }
void Module::addConstant(Type *type, const std::string &name, double val)  { }
void Module::addConstant(Type *type, const std::string &name, int64_t val)  { }
void Module::addConstant(Type *type, const std::string &name, int val)  { }
void Module::finish()  { }

// Type
void Type::checkFinished() { };
void Type::addBase(Type *base) { };
void Type::addInstVar(Type *type, const std::string &name) { };

Func *Type::addMethod(Type *returnType, const std::string &name,
                void *funcPtr
                ) { };

Func *Type::addConstructor(const char *name, void *funcPtr) { };

Func *Type::addStaticMethod(Type *returnType, const std::string &name,
                      void *funcPtr
                      ) { };

Type *Type::getSpecialization(const std::vector<Type *> &params) { };
void Type::finish() { };

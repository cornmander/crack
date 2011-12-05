// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_ArrayTypeDef_h_
#define _builder_llvm_ArrayTypeDef_h_

#include "model/Context.h"
#include "BTypeDef.h"
#include "Utils.h"
#include <string>

namespace llvm {
    class Type;
}

namespace builder {
namespace mvll {

class ArrayTypeDef : public BTypeDef {
public:
    ArrayTypeDef(model::TypeDef *metaType,
                 const std::string &name,
                 llvm::Type *rep
                 );

    // specializations of array types actually create a new type
    // object.
    virtual model::TypeDef *getSpecialization(model::Context &context,
                                              TypeVecObj *types
                                              );
};


} // end namespace builder::vmll
} // end namespace builder

#endif

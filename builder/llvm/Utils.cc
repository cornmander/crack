// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "Utils.h"

#include "BTypeDef.h"
#include "Ops.h"
#include "LLVMBuilder.h"
#include <llvm/LLVMContext.h>
#include <llvm/GlobalValue.h>
#include <llvm/GlobalVariable.h>
#include <spug/StringFmt.h>

using namespace llvm;
using namespace model;
using namespace std;

namespace builder { namespace mvll {

void addArrayMethods(Context &context, TypeDef *arrayType,
                     BTypeDef *elemType
                     ) {
    Context::GlobalData *gd = context.globalData;
    FuncDefPtr arrayGetItem =
            new GeneralOpDef<ArrayGetItemCall>(elemType, FuncDef::method,
                                               "oper []",
                                               1
                                               );
    arrayGetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
    arrayType->addDef(arrayGetItem.get());

    FuncDefPtr arraySetItem =
            new GeneralOpDef<ArraySetItemCall>(elemType, FuncDef::method,
                                               "oper []=",
                                               2
                                               );
    arraySetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
    arraySetItem->args[1] = new ArgDef(elemType, "value");
    arrayType->addDef(arraySetItem.get());

    FuncDefPtr arrayOffset =
            new GeneralOpDef<ArrayOffsetCall>(arrayType, FuncDef::noFlags,
                                              "oper +",
                                              2
                                              );
    arrayOffset->args[0] = new ArgDef(arrayType, "base");
    arrayOffset->args[1] = new ArgDef(gd->uintType.get(), "offset");
    context.getDefContext()->ns->addDef(arrayOffset.get());

    FuncDefPtr arrayAlloc =
            new GeneralOpDef<ArrayAllocCall>(arrayType, FuncDef::noFlags,
                                             "oper new",
                                             1
                                             );
    arrayAlloc->args[0] = new ArgDef(gd->uintType.get(), "size");
    arrayType->addDef(arrayAlloc.get());
}

// Create a new meta-class.
// context: enclosing context.
// name: the original class name.
// bases: the base classes.
// classImpl: returned impl object for the original class.
BTypeDefPtr createMetaClass(Context &context,
                            const string &name,
                            vector<TypeDefPtr> bases,
                            BGlobalVarDefImplPtr &classImpl
                            ) {
    LLVMBuilder &llvmBuilder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    LLVMContext &lctx = getGlobalContext();

    BTypeDefPtr metaType =
            new BTypeDef(context.globalData->classType.get(),
                         SPUG_FSTR("Class[" << name << "]"),
                         0,
                         true,
                         0
                         );
    BTypeDef *classType =
            BTypeDefPtr::arcast(context.globalData->classType);
    metaType->addBaseClass(classType);
    const PointerType *classPtrType = cast<PointerType>(classType->rep);
    const StructType *classStructType =
            cast<StructType>(classPtrType->getElementType());

    // Create a struct representation of the meta class.  This just has the
    // Class class as its only field.
    vector<const Type *> fields(1);
    fields[0] = classStructType;
    const StructType *metaClassStructType = StructType::get(lctx, fields);
    const Type *metaClassPtrType =
            PointerType::getUnqual(metaClassStructType);
    metaType->rep = metaClassPtrType;
    metaType->complete = true;

    // create a global variable holding the class object.
    vector<Constant *> classStructVals(3);

    Constant *zero = ConstantInt::get(Type::getInt32Ty(lctx), 0);
    Constant *index00[] = { zero, zero };

    // name
    Constant *nameInit = ConstantArray::get(lctx, name, true);
    GlobalVariable *nameGVar =
            new GlobalVariable(*llvmBuilder.module,
                               nameInit->getType(),
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               nameInit,
                               name + ":name"
                               );
    classStructVals[0] =
            ConstantExpr::getGetElementPtr(nameGVar, index00, 2);

    // numBases
    const Type *uintType =
            BTypeDefPtr::arcast(context.globalData->uintType)->rep;
    classStructVals[1] = ConstantInt::get(uintType, bases.size());

    // bases
    vector<Constant *> basesVal(bases.size());
    for (int i = 0; i < bases.size(); ++i) {
        // get the pointer to the inner "Class" object of "Class[BaseName]"
        BGlobalVarDefImplPtr impl =
                BGlobalVarDefImplPtr::arcast(
                        BTypeDefPtr::arcast(bases[i])->impl
                        );

        // extract the initializer from the rep (which is the global
        // variable for the _pointer_ to the class) and then GEP our way
        // into the base class (Class) instance.
        Constant *baseClassPtr = impl->rep->getInitializer();
        Constant *baseAsClass =
                ConstantExpr::getGetElementPtr(baseClassPtr, index00, 2);
        basesVal[i] = baseAsClass;
    }
    const ArrayType *baseArrayType =
            ArrayType::get(classType->rep, bases.size());
    Constant *baseArrayInit = ConstantArray::get(baseArrayType, basesVal);
    GlobalVariable *basesGVar =
            new GlobalVariable(*llvmBuilder.module,
                               baseArrayType,
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               baseArrayInit,
                               name + ":bases"
                               );
    classStructVals[2] = ConstantExpr::getGetElementPtr(basesGVar, index00,
                                                        2
                                                        );

    // build the instance of Class
    Constant *classStruct =
            ConstantStruct::get(classStructType, classStructVals);

    // the new meta class's structure is another structure with only
    // Class as a member.
    vector<Constant *> metaClassStructVals(1);
    metaClassStructVals[0] = classStruct;
    Constant *classObjVal =
            ConstantStruct::get(metaClassStructType, metaClassStructVals);

    // Create the class global variable
    GlobalVariable *classInst =
            new GlobalVariable(*llvmBuilder.module,
                               metaClassStructType,
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               classObjVal,
                               name + ":body"
                               );

    // create the pointer to the class instance
    GlobalVariable *classInstPtr =
            new GlobalVariable(*llvmBuilder.module,
                               metaClassPtrType,
                               true, // is constant
                               GlobalVariable::ExternalLinkage,
                               classInst,
                               name
                               );

    classImpl = new BGlobalVarDefImpl(classInstPtr);
    return metaType;
}

} } // namespace builder::mvll

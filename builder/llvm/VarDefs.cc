// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "VarDefs.h"

#include "BResultExpr.h"
#include "BTypeDef.h"

#include "model/AssignExpr.h"

#include <llvm/GlobalVariable.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

// BArgVarDefImpl
ResultExprPtr BArgVarDefImpl::emitRef(Context &context,
                                VarRef *var
                                ) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    b.emitArgVarRef(context, rep);

    return new BResultExpr((Expr*)var, b.lastValue);
}

ResultExprPtr BArgVarDefImpl::emitAssignment(Context &context, 
                                             AssignExpr *assign
                                             ) {
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    
    // we have to promote the argument to a local variable.
    BTypeDef *varType = BTypeDefPtr::arcast(assign->var->type);
    Value *var;
    BHeapVarDefImplPtr localVar = b.createLocalVar(varType, var);
    assign->var->impl = localVar;
    
    // then do the emitAssignment() on the local variable.
    return localVar->emitAssignment(context, assign);
}

// BMemVarDefImpl
ResultExprPtr BMemVarDefImpl::emitRef(Context &context, VarRef *var) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    b.emitMemVarRef(context, getRep(b));
    return new BResultExpr((Expr*)var, b.lastValue);
}

ResultExprPtr BMemVarDefImpl::emitAssignment(Context &context, AssignExpr *assign) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    ResultExprPtr result = assign->value->emit(context);
    b.narrow(assign->value->type.get(), assign->var->type.get());
    Value *exprVal = b.lastValue;
    b.builder.CreateStore(exprVal, getRep(b));
    result->handleAssignment(context);
    b.lastValue = exprVal;

    return new BResultExpr(assign, exprVal);
}


// BGlobalVarDefImpl
Value * BGlobalVarDefImpl::getRep(LLVMBuilder &builder) {
    if (rep->getParent() != builder.module)
        rep = builder.getModVar(this);
    return rep;
}


// BConstDefImpl
ResultExprPtr BConstDefImpl::emitRef(Context &context, VarRef *var) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    b.lastValue = rep;
    return new BResultExpr((Expr*)var, b.lastValue);
}

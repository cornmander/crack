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

VarDefImplPtr BArgVarDefImpl::promote(LLVMBuilder &builder, ArgDef *arg) {
    Value *var;
    BHeapVarDefImplPtr localVar =
        builder.createLocalVar(BTypeDefPtr::arcast(arg->type), var, rep);
    return localVar;
}

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
    // This should never happen - arguments except for "this" all get promoted 
    // to local variables at the start of the function.
    assert(0 && "attempting to emit an argument assignment");
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
    Value *exprVal = b.lastValue;
    b.narrow(assign->value->type.get(), assign->var->type.get());
    b.builder.CreateStore(b.lastValue, getRep(b));
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

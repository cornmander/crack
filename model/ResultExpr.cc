// Copyright 2009 Google Inc.

#include "ResultExpr.h"

#include "builder/Builder.h"
#include "CleanupFrame.h"
#include "Context.h"
#include "FuncCall.h"
#include "FuncDef.h"
#include "TypeDef.h"

using namespace model;

void ResultExpr::handleAssignment(Context &context) {
    // if the expression is productive, the assignment will just consume its 
    // reference.
    if (sourceExpr->isProductive())
        return;

    // the expression is non-productive: check for a bind function
    FuncCall::ExprVec args;
    FuncDefPtr bindFunc = type->context->lookUp(context, "bind", args);
    if (!bindFunc)
        return;
    
    // got a bind function: create a bind call and emit it.  (emit should 
    // return a ResultExpr for a void object, so we don't need to do anything 
    // special for it).
    FuncCallPtr bindCall = context.builder.createFuncCall(bindFunc.get());
    bindCall->receiver = this;
    bindCall->emit(context);
}

void ResultExpr::handleTransient(Context &context) {
    // we don't need to do anything if we're currently emitting cleanups or for
    // non-productive expressions.
    if (context.emittingCleanups || !sourceExpr->isProductive())
        return;
    
    // the expression is productive - check for a release function
    FuncCall::ExprVec args;
    FuncDefPtr releaseFunc = type->context->lookUp(context, "release", args);
    if (!releaseFunc)
        return;
    
    // got a release function: create a release call and store it in the 
    // cleanups.
    FuncCallPtr releaseCall = 
        context.builder.createFuncCall(releaseFunc.get());
    releaseCall->receiver = this;
    context.cleanupFrame->addCleanup(releaseCall.get());
}

bool ResultExpr::isProductive() const {
    return sourceExpr->isProductive();
}

void ResultExpr::writeTo(std::ostream &out) const {
    out << "result(" << *sourceExpr << ')';
}

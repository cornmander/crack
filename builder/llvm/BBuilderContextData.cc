// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "BBuilderContextData.h"

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Intrinsics.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include "model/Expr.h"
#include "BTypeDef.h"
#include "Incompletes.h"
#include "VarDefs.h"

using namespace std;
using namespace llvm;
using namespace builder::mvll;
using namespace model;

Value *
BBuilderContextData::getExceptionLandingPadResult(IRBuilder<> &builder) {
    VarDefPtr exStructVar = context->ns->lookUp(":exStruct");
    BHeapVarDefImplPtr exStructImpl = 
        BHeapVarDefImplPtr::rcast(exStructVar->impl);
    return 
        builder.CreateLoad(builder.CreateConstGEP2_32(exStructImpl->rep, 0, 0));
}

BasicBlock *BBuilderContextData::getUnwindBlock(Function *func) {
    if (!unwindBlock) {
        unwindBlock = BasicBlock::Create(getGlobalContext(), "unwind", func);
        IRBuilder<> b(unwindBlock);
        
        Module *mod = func->getParent();
        Function *f = mod->getFunction("__CrackExceptionFrame");
        if (f)
            b.CreateCall(f);

        // create the resume instruction
        Value *exObj = getExceptionLandingPadResult(b);

        // XXX We used to create an "unwind" instruction here, but that seems 
        // to cause a problem when creating a module with dependencies on 
        // classes in an unfinished module, as we can do when specializing a 
        // generic.  The problem is that _Unwind_Resume is resolved from the 
        // incorrect module.
        // To deal with this, we create an explicit call to _Unwind_Resume.  
        // The only problem here is that we have to call llvm.eh.exception to 
        // obtain the exception object, even though we might already have one.
        LLVMContext &lctx = getGlobalContext();
        Constant *c = mod->getOrInsertFunction("_Unwind_Resume", 
                                               Type::getVoidTy(lctx),
                                               Type::getInt8PtrTy(lctx),
                                               NULL
                                               );
        f = cast<Function>(c);
        b.CreateCall(f, exObj);
        b.CreateUnreachable();
    }

    // assertion to make sure this is the right unwind block
    if (unwindBlock->getParent() != func) {
        string bdataFunc = unwindBlock->getParent()->getName();
        string curFunc = func->getName();
        cerr << "bad function for unwind block, got " <<
            bdataFunc << " expected " << curFunc << endl;
        assert(false);
    }

    return unwindBlock;
}

void BBuilderContextData::CatchData::populateClassImpls(
    vector<Value *> &values,
    Module *module
) {
    for (int i = 0; i < catches.size(); ++i)
        values.push_back(catches[i].type->getClassInstRep(module, 0));
}

void BBuilderContextData::CatchData::fixAllSelectors(Module *module) {
    // fix all of the incomplete selector functions now that we have all of
    // the catch clauses.
    for (vector<IncompleteCatchSelector *>::iterator iter = selectors.begin();
        iter != selectors.end();
        ++iter
        ) {
        // get all of the class implementation objects into a Value vector
        vector<Value *> typeImpls;
        populateClassImpls(typeImpls, module);

        // fix the incomplete selector
        (*iter)->typeImpls = &typeImpls;
        (*iter)->fix();
    }

    // fix the switch instruction
    Type *int32Ty = Type::getInt32Ty(getGlobalContext());
    for (int i = 0; i < catches.size(); ++i) {
        ConstantInt *index =
            cast<ConstantInt>(ConstantInt::get(int32Ty, i + 1));
        switchInst->addCase(index, catches[i].block);
    }

    // do the same for all nested try/catches
    for (int childIndex = 0; childIndex < nested.size(); ++childIndex) {
        CatchData &child = *nested[childIndex];

        // add all of the branches from the parent that the child won't already
        // catch
        vector<CatchBranch> catchesToAdd;
        for (int i = 0; i < catches.size(); ++i) {
            CatchBranch &branch = catches[i];
            for (int j = 0; j < child.catches.size(); ++j)
                if (!branch.type->isDerivedFrom(child.catches[j].type.get()))
                    catchesToAdd.push_back(branch);

            for (int j = 0; j < catchesToAdd.size(); ++j)
                child.catches.push_back(catchesToAdd[j]);
        }


        // fix all of the selectors in the child
        child.fixAllSelectors(module);
    }
}

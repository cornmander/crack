// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMLinkerBuilder.h"
#include "BModuleDef.h"
#include "DebugInfo.h"
#include "model/Context.h"
#include "BTypeDef.h"
#include "FuncBuilder.h"
#include "Utils.h"

#include <llvm/Support/StandardPasses.h>
#include <llvm/LLVMContext.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/PassManager.h>
#include <llvm/Module.h>
#include <llvm/Linker.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;

Linker *LLVMLinkerBuilder::linkModule(Module *mod) {
    if (linker) {
        string errMsg;
        linker->LinkInModule(mod, &errMsg);
        if (errMsg.length()) {
            std::cerr << "error linking " << mod->getModuleIdentifier() <<
                    " [" + errMsg + "]\n";
            mod->dump();
        }
    } else {
        if (rootBuilder)
            linker = LLVMLinkerBuilderPtr::cast(
                    rootBuilder.get())->linkModule(mod);
        else {
            linker = new Linker("crack",
                                "main-module",
                                getGlobalContext(),
                                Linker::Verbose // flags
                                );
        }
    }

    return linker;
}

// maintain a single list of modules throughout the compile in the root builder
LLVMLinkerBuilder::moduleListType
    *LLVMLinkerBuilder::addModule(ModuleDef *mod) {

    if (moduleList) {
        moduleList->push_back(mod);
    } else {
        if (rootBuilder)
           moduleList = LLVMLinkerBuilderPtr::cast(
                   rootBuilder.get())->addModule(mod);
        else {
            moduleList = new moduleListType();
        }
    }

    return moduleList;
}


void *LLVMLinkerBuilder::getFuncAddr(llvm::Function *func) {
    assert("LLVMLinkerBuilder::getFuncAddr called");
}

void LLVMLinkerBuilder::run() {

    assert(!rootBuilder && "run must be called from root builder");

    // final linked IR
    Module *finalir = linker->getModule();

    // LTO optimizations
    if (optimizeLevel) {
        PassManager passMan;
        createStandardLTOPasses(&passMan,
                                true, // internalize
                                true, // inline
                                debugMode // verify each
                                );
        passMan.run(*finalir);
    }

    //nativeCompile(finalir, options);

}

BuilderPtr LLVMLinkerBuilder::createChildBuilder() {
    LLVMLinkerBuilder *result = new LLVMLinkerBuilder();
    result->rootBuilder = rootBuilder ? rootBuilder : this;
    result->llvmVoidPtrType = llvmVoidPtrType;
    result->dumpMode = dumpMode;
    result->debugMode = debugMode;
    return result;
}

ModuleDefPtr LLVMLinkerBuilder::createModule(Context &context,
                                       const string &name) {

    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    module = new llvm::Module(name, lctx);

    if (debugMode) {
        debugInfo = new DebugInfo(module, name);
    }

    llvm::Constant *c =
        module->getOrInsertFunction(name+":main", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    block = BasicBlock::Create(lctx, "initCheck", func);
    builder.SetInsertPoint(block);

    // insert point is now at the begining of the :main function for
    // this module. this will run the top level code for the module
    // however, we consult a module level global to ensure that we only
    // run the top level code once, since this module may be imported from
    // more than one place
    GlobalVariable *moduleInit =
            new GlobalVariable(*module,
                               Type::getInt1Ty(lctx),
                               false, // constant
                               GlobalValue::InternalLinkage,
                               Constant::getIntegerValue(
                                       Type::getInt1Ty(lctx),APInt(1,0,false)),
                               name+":initialized"
                               );

    // emit code that checks the global and returns immediately if it
    // has been set to 1
    BasicBlock *alreadyInitBlock = BasicBlock::Create(lctx, "alreadyInit", func);
    assert(!mainInsert);
    block = mainInsert = BasicBlock::Create(lctx, "topLevel", func);
    Value* currentInitVal = builder.CreateLoad(moduleInit);
    builder.CreateCondBr(currentInitVal, alreadyInitBlock, mainInsert);

    // already init, return
    builder.SetInsertPoint(alreadyInitBlock);
    builder.CreateRetVoid();

    // do init, set global to 1, continue top level..
    builder.SetInsertPoint(mainInsert);
    builder.CreateStore(Constant::getIntegerValue(
                          Type::getInt1Ty(lctx),APInt(1,1,false)),
                        moduleInit);

    // name some structs in this module
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    module->addTypeName(".struct.Class", classType->rep);
    BTypeDef *vtableBaseType = BTypeDefPtr::arcast(
                                  context.construct->vtableBaseType);
    module->addTypeName(".struct.vtableBase", vtableBaseType->rep);

    // all of the "extern" primitive functions have to be created in each of
    // the modules - we can not directly reference across modules.

    BTypeDef *int32Type = BTypeDefPtr::arcast(context.construct->int32Type);
    BTypeDef *int64Type = BTypeDefPtr::arcast(context.construct->int64Type);
    BTypeDef *uint64Type = BTypeDefPtr::arcast(context.construct->uint64Type);
    BTypeDef *intType = BTypeDefPtr::arcast(context.construct->intType);
    BTypeDef *voidType = BTypeDefPtr::arcast(context.construct->int32Type);
    BTypeDef *float32Type = BTypeDefPtr::arcast(context.construct->float32Type);
    BTypeDef *byteptrType =
        BTypeDefPtr::arcast(context.construct->byteptrType);
    BTypeDef *voidptrType =
        BTypeDefPtr::arcast(context.construct->voidptrType);

    // create "int puts(String)"
    {
        FuncBuilder f(context, FuncDef::noFlags, int32Type, "puts",
                      1
                      );
        f.addArg("text", byteptrType);
        f.setSymbolName("puts");
        f.finish();
    }

    // create "void printint(int32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printint", 1);
        f.addArg("val", int32Type);
        f.setSymbolName("printint");
        f.finish();
    }

    // create "void printint64(int64)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printint64", 1);
        f.addArg("val", int64Type);
        f.setSymbolName("printint64");
        f.finish();
    }

    // create "void printuint64(int64)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printuint64", 1);
        f.addArg("val", uint64Type);
        f.setSymbolName("printuint64");
        f.finish();
    }

    // create "void printfloat(float32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printfloat", 1);
        f.addArg("val", float32Type);
        f.setSymbolName("printfloat");
        f.finish();
    }

    // create "void *calloc(uint size)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidptrType, "calloc", 2);
        f.addArg("size", intType);
        f.addArg("size", intType);
        f.setSymbolName("calloc");
        f.finish();
        callocFunc = f.funcDef->getRep(*this);
    }

    // create "array[byteptr] __getArgv()"
    {
        TypeDefPtr array = context.ns->lookUp("array");
        assert(array.get() && "array not defined in context");
        TypeDef::TypeVecObjPtr types = new TypeDef::TypeVecObj();
        types->push_back(context.construct->byteptrType.get());
        TypeDefPtr arrayOfByteptr =
            array->getSpecialization(context, types.get());
        FuncBuilder f(context, FuncDef::noFlags,
                      BTypeDefPtr::arcast(arrayOfByteptr),
                      "__getArgv",
                      0
                      );
        f.setSymbolName("__getArgv");
        f.finish();
    }

    // create "int __getArgc()"
    {
        FuncBuilder f(context, FuncDef::noFlags, intType, "__getArgc", 0);
        f.setSymbolName("__getArgc");
        f.finish();
    }

    // add to module list
    ModuleDef *newModule = new BModuleDef(name, context.ns.get());
    addModule(newModule);

    return newModule;
}

void LLVMLinkerBuilder::closeModule(Context &context, ModuleDef *moduleDef) {

    assert(module);
    builder.CreateRetVoid();

    // emit the cleanup function for this module
    // we will emit calls to these (for all modules) during run() in the finalir
    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
        module->getOrInsertFunction(moduleDef->name+":cleanup",
                                    Type::getVoidTy(lctx), NULL);
    Function *dfunc = llvm::cast<llvm::Function>(c);
    dfunc->setCallingConv(llvm::CallingConv::C);
    block = BasicBlock::Create(lctx, "", dfunc);
    builder.SetInsertPoint(block);
    closeAllCleanupsStatic(context);
    builder.CreateRetVoid();

    linkModule(module);

    if (debugInfo)
        delete debugInfo;

}


// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMJitBuilder.h"
#include "BJitModuleDef.h"
#include "DebugInfo.h"
#include "BTypeDef.h"
#include "model/Context.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "BBuilderContextData.h"
#include "debug/DebugTools.h"
#include "Cacher.h"

#include <llvm/LLVMContext.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>  // link in the JIT
#include <llvm/Module.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Intrinsics.h>
#include <llvm/Support/Debug.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;


void LLVMJitBuilder::engineBindModule(BModuleDef *moduleDef) {
    // note, this->module and moduleDef->rep should be ==
    bindJitModule(moduleDef->rep);
    if (options->dumpMode)
        dump();
}

void LLVMJitBuilder::engineFinishModule(BModuleDef *moduleDef) {
    // note, this->module and moduleDef->rep should be ==

    // XXX right now, only checking for > 0, later perhaps we can
    // run specific optimizations at different levels
    if (options->optimizeLevel) {
        // optimize
        llvm::PassManager passMan;

        // Set up the optimizer pipeline.  Start with registering info about how
        // the target lays out data structures.
        passMan.add(new llvm::TargetData(*execEng->getTargetData()));
        // Promote allocas to registers.
        passMan.add(createPromoteMemoryToRegisterPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        passMan.add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        passMan.add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        passMan.add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        passMan.add(llvm::createCFGSimplificationPass());

        passMan.run(*moduleDef->rep);
    }
    Function *delFunc = module->getFunction(":cleanup");
    if (delFunc) {
        moduleDef->cleanup = reinterpret_cast<void (*)()>(
                                execEng->getPointerToFunction(delFunc)
                             );
    }

    // mark the module as finished
    moduleDef->rep->getOrInsertNamedMetadata("crack_finished");
}

void LLVMJitBuilder::fixClassInstRep(BTypeDef *type) {
    type->getClassInstRep(module, execEng);
}

BModuleDef *LLVMJitBuilder::instantiateModule(model::Context &context,
                                              const std::string &name,
                                              llvm::Module *owner
                                              ) {
    return new BJitModuleDef(name, context.ns.get(), owner, 0);
}

ExecutionEngine *LLVMJitBuilder::bindJitModule(Module *mod) {
    if (execEng) {
        execEng->addModule(mod);
    } else {
        if (rootBuilder)
            execEng = LLVMJitBuilderPtr::cast(rootBuilder.get())->bindJitModule(mod);
        else {

            llvm::JITEmitDebugInfo = true;
            llvm::JITExceptionHandling = true;

            // XXX only available in debug builds of llvm?
            //if (options->verbosity > 3)
            //    llvm::DebugFlag = true;

            // we have to specify all of the arguments for this so we can turn
            // off "allocate globals with code."  In addition to being
            // deprecated in the docs for this function, this option causes
            // seg-faults when we allocate globals under certain conditions.
            InitializeNativeTarget();
            execEng = ExecutionEngine::create(mod,
                                              false, // force interpreter
                                              0, // error string
                                              CodeGenOpt::Default, // opt lvl
                                              false // alloc globals with code
                                              );

        }
    }

    return execEng;
}

void LLVMJitBuilder::addGlobalFuncMapping(Function* pointer,
                                          Function* real) {
    // if the module containing the original function has been finished, just
    // add the global mapping.
    if (real->getParent()->getNamedMetadata("crack_finished")) {
        void *realAddr = execEng->getPointerToFunction(real);
        execEng->addGlobalMapping(pointer, realAddr);
    } else {
        // push this on the list of externals - we used to assign a global mapping
        // for these right here, but that only works if we're guaranteed that an
        // imported module is closed before any of its functions are used by the
        // importer, and that is no longer the case after generics and ephemeral
        // modules.
        externals.push_back(pair<Function *, Function *>(pointer, real));
    }
}

void LLVMJitBuilder::addGlobalFuncMapping(Function* pointer,
                                          void* real) {
    execEng->addGlobalMapping(pointer, real);
}

void LLVMJitBuilder::addGlobalVarMapping(GlobalValue* pointer,
                                         GlobalValue* real) {
    execEng->addGlobalMapping(pointer, execEng->getPointerToGlobal(real));
}

void *LLVMJitBuilder::getFuncAddr(llvm::Function *func) {
    return execEng->getPointerToFunction(func);
}

void LLVMJitBuilder::run() {
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    fptr();
}

BuilderPtr LLVMJitBuilder::createChildBuilder() {
    LLVMJitBuilder *result = new LLVMJitBuilder();
    result->rootBuilder = rootBuilder ? rootBuilder : this;
    result->llvmVoidPtrType = llvmVoidPtrType;
    result->options = options;
    return result;
}

ModuleDefPtr LLVMJitBuilder::createModule(Context &context,
                                          const string &name,
                                          const string &path,
                                          ModuleDef *owner
                                          ) {

    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    createLLVMModule(name);

    if (options->debugMode) {
        debugInfo = new DebugInfo(module, name);
    }

    // create a context data object
    BBuilderContextData::get(&context);

    string mainFuncName = name + ":main";
    llvm::Constant *c =
        module->getOrInsertFunction(mainFuncName, Type::getVoidTy(lctx),
                                    NULL
                                    );
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    createFuncStartBlocks(mainFuncName);

    createModuleCommon(context);

    bindJitModule(module);

    bModDef =
        new BJitModuleDef(name, context.ns.get(), module,
                          owner ? BJitModuleDefPtr::acast(owner) : 0
                          );

    bModDef->path = getSourcePath(path);

    return bModDef;
}

void LLVMJitBuilder::innerCloseModule(Context &context, ModuleDef *moduleDef) {
    // if there was a top-level throw, we could already have a terminator.
    // Generate a return instruction if not.
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateRetVoid();

    // emit the cleanup function

    // since the cleanups have to be emitted against the module context, clear
    // the unwind blocks so we generate them for the del function.
    clearCachedCleanups(context);

    Function *mainFunc = func;
    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
        module->getOrInsertFunction(":cleanup", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    // create a new exStruct variable for this context
    createFuncStartBlocks(":cleanup");
    createSpecialVar(context.ns.get(), getExStructType(), ":exStruct");

    closeAllCleanupsStatic(context);
    builder.CreateRetVoid();

    // restore the main function
    func = mainFunc;

    // work around for an LLVM bug: When doing one of its internal exception
    // handling passes, LLVM can insert llvm.eh.exception() intrinsics with
    // calls to an llvm.eh.exception() function that are not part of the
    // module.  So this loop replaces all such calls with the correct instance
    // of the function.
    Function *ehEx = getDeclaration(module, Intrinsic::eh_exception);
    for (Module::iterator func = module->begin(); func != module->end();
         ++func
         )
        for (Function::iterator block = func->begin(); block != func->end();
             ++block
             )
            for (BasicBlock::iterator inst = block->begin();
                 inst != block->end();
                 ++inst
                 ) {
                IntrinsicInst *intrInst;
                if ((intrInst = dyn_cast<IntrinsicInst>(inst)) &&
                    intrInst->getIntrinsicID() == Intrinsic::eh_exception &&
                    intrInst->getCalledFunction() != ehEx
                    )
                    intrInst->setCalledFunction(ehEx);
            }

// XXX in the future, only verify if we're debugging
//    if (debugInfo)
        verifyModule(*module, llvm::PrintMessageAction);

    // let jit or linker finish module before run/link
    engineFinishModule(BModuleDefPtr::cast(moduleDef));

    // store primitive functions from an extension
    if (moduleDef->fromExtension) {
        for (map<Function *, void *>::iterator iter = primFuncs.begin();
             iter != primFuncs.end();
             ++iter)
            addGlobalFuncMapping(iter->first, iter->second);
    }

    if (debugInfo)
        delete debugInfo;

    // resolve all externals
    for (int i = 0; i < externals.size(); ++i) {
        void *realAddr = execEng->getPointerToFunction(externals[i].second);
        execEng->addGlobalMapping(externals[i].first, realAddr);
    }
    externals.clear();

    // build the debug tables
    Module::FunctionListType &funcList = module->getFunctionList();
    for (Module::FunctionListType::iterator funcIter = funcList.begin();
         funcIter != funcList.end();
         ++funcIter
         ) {
        string name = funcIter->getName();
        if (!funcIter->isDeclaration())
            crack::debug::registerDebugInfo(
                execEng->getPointerToGlobal(funcIter),
                name,
                "",   // file name
                0     // line number
            );
    }

    doRunOrDump(context);

}
void LLVMJitBuilder::doRunOrDump(Context &context) {

    // dump or run the module depending on the mode.
    if (rootBuilder->options->statsMode)
        context.construct->stats->switchState(ConstructStats::run);

    if (options->dumpMode)
        dump();

    if (!options->dumpMode || !context.construct->compileTimeConstruct)
        run();

    if (rootBuilder->options->statsMode)
        context.construct->stats->switchState(ConstructStats::build);

}

void LLVMJitBuilder::closeModule(Context &context, ModuleDef *moduleDef) {
    assert(module);
    BJitModuleDefPtr::acast(moduleDef)->closeOrDefer(context, this);
}

void LLVMJitBuilder::dump() {
    PassManager passMan;
    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
    passMan.run(*module);
}

void LLVMJitBuilder::registerDef(Context &context, VarDef *varDef) {

    // here we keep track of which external functions and globals came from
    // which module, so we can do a mapping in cacheMode
    if (!options->cacheMode)
        return;

    // make sure cacheMap is initialized
    // a single copy of the map exists in the rootBuilder
    if (!cacheMap) {
        if (rootBuilder) {
            LLVMJitBuilder *rb = LLVMJitBuilderPtr::cast(rootBuilder.get());
            if (rb->cacheMap)
                cacheMap = rb->cacheMap;
            else
                cacheMap = rb->cacheMap = new CacheMapType();
        }
        else {
            assert(0 && "non rootBuilder registerDef called");
        }
    }

    // get rep from either a BFuncDef or varDef->impl global, then use that as
    // value to cacheMap
    BGlobalVarDefImpl *bgbl;
    BFuncDef *fd;
    if (varDef->impl && (bgbl = dynamic_cast<BGlobalVarDefImpl*>(varDef->impl.get()))) {
        // global        
        cacheMap->insert(CacheMapType::value_type(varDef->getFullName(),bgbl->rep));
    }
    else if (fd = dynamic_cast<BFuncDef*>(varDef)) {
        // funcdef
        cacheMap->insert(CacheMapType::value_type(varDef->getFullName(),fd->rep));
        }
    else {
        //assert(0 && "registerDef: unknown varDef type");
        // this happens in a call from parser (not cacher) on e.g. classes,
        // which we don't care about here
        return;
    }

}

model::ModuleDefPtr LLVMJitBuilder::materializeModule(model::Context &context,
                                                   const std::string &canonicalName,
                                                   const std::string &path,
                                                   model::ModuleDef *owner) {

    Cacher c(context, options.get());
    BModuleDef *bmod = c.maybeLoadFromCache(canonicalName, path);

    if (bmod) {

        // we materialized a module from bitcode cache
        // find the main function
        module = bmod->rep;

        // XXX move this to Cacher::getEntryFunction() to hide metadata impl
        NamedMDNode *node = module->getNamedMetadata("crack_entry_func");
        assert(node && "no crack_entry_func");
        MDNode *funcNode = node->getOperand(0);
        assert(funcNode && "malformed crack_entry_func");
        func = dyn_cast<Function>(funcNode->getOperand(0));
        assert(func && "entry function not LLVM Function!");

        engineBindModule(bmod);

        // poor man's link - global mappings
        // here we loop through all the externs of the
        // loaded bitcode and try to resolve them from other modules already
        // loaded into the JIT. Note that externs from extensions are not
        // mapped, as these are handled by the JIT without our help (by
        // searching for the symbol in the current process, which works because
        // when the extension was imported, we've already dlopen'd it)
        Function *ext_f;
        GlobalVariable *ext_g;
        GlobalValue *gval;
        MDString *sym;
        // XXX move this to Cacher::getExterns() to hide metadata impl
        NamedMDNode *externs = module->getNamedMetadata("crack_externs");
        assert(externs && "no crack_externs node");
        MDNode *symNode = externs->getOperand(0);
        for (int i = 0; i < symNode->getNumOperands(); ++i) {

            sym = dyn_cast<MDString>(symNode->getOperand(i));
            assert(sym && "malformed crack_externs");

            CacheMapType::const_iterator cmi = cacheMap->find(sym->getString().str());
            assert(cmi != cacheMap->end() && "external not found");

            gval = cmi->second;

            Function *decl = module->getFunction(sym->getString());
            if (decl) {
                assert(decl->isDeclaration() &&
                       "declared extern function wasn't a decl");

                // find the function with this symbol name by searching through
                // the modules that have already been added to the JIT
                // we always expect it because 1) imports for this module have
                // already been loaded and 2) the imports would have missed if
                // their source digest didn't match (i.e. symbols changed) causing
                // us to miss as well before now
                ext_f = dyn_cast<Function>(gval);
                assert(ext_f && "extern not found in JIT");

                // materializing will ensure the function is codegen'd
                if (ext_f->isMaterializable()) {
                    if (ext_f->Materialize()) {
                        cerr << "materialize fail: " << ext_f->getNameStr()
                             << endl;
                        return NULL;
                    }
                }

                // after potentially materializing, ext_f should be a def
                assert(!ext_f->isDeclaration() && "ext_f: got a decl not a def");

                void *realAddr = execEng->getPointerToFunction(ext_f);
                assert(realAddr && "unable to resolve ext_f");
                execEng->addGlobalMapping(decl, realAddr);
            }
            else {
                // map globals
                GlobalVariable *gbl = module->getGlobalVariable(sym->getString());
                assert(gbl && "declared extern was not function or global");
                assert(gbl->isDeclaration() && "declared global wasn't a decl");
                // now find the defining module
                ext_g = dyn_cast<GlobalVariable>(gval);
                assert(ext_g && "extern not found in JIT");
                void* realAddr = execEng->getPointerToGlobal(ext_g);
                assert(realAddr && "unable to resolve global");
                execEng->addGlobalMapping(gbl, realAddr);
            }

        }

        doRunOrDump(context);

    }

    return bmod;

}

void LLVMJitBuilder::cacheModule(model::Context &context,
                                 model::ModuleDefPtr mod) {

    // encode main function location in bitcode metadata
    vector<Value *> dList;
    NamedMDNode *node;

    node = module->getOrInsertNamedMetadata("crack_entry_func");
    dList.push_back(func);
    node->addOperand(MDNode::get(getGlobalContext(), dList));

    Cacher c(context, options.get(), BModuleDefPtr::rcast(mod));
    c.saveToCache();

}

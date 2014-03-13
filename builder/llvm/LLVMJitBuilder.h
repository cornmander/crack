// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_LLVMJitBuilder_h_
#define _builder_LLVMJitBuilder_h_

#include "LLVMBuilder.h"

#include "spug/Tracer.h"
#include "ModuleMerger.h"

namespace llvm {
    class ExecutionEngine;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BJitModuleDef);
SPUG_RCPTR(BModuleDef);
SPUG_RCPTR(LLVMJitBuilder);

class LLVMJitBuilder : public LLVMBuilder {
    private:
        llvm::ExecutionEngine *execEng;

        llvm::ExecutionEngine *bindJitModule(llvm::Module *mp);

        // symbols that were imported from a shared library (we don't want to 
        // try to resolve these).
        std::set<std::string> shlibSyms;
        std::set<std::string> &getShlibSyms() {
            if (rootBuilder)
                return LLVMJitBuilderPtr::rcast(rootBuilder)->shlibSyms;
            else
                return shlibSyms;
        }
        
        // Definitions created in the module that are not part of its defs.
        std::vector<model::VarDefPtr> orphanedDefs;
        
        ModuleMerger *moduleMerger;
        ModuleMerger *getModuleMerger();
                
        virtual void run();

        virtual void dump();

        void doRunOrDump(model::Context &context);

        void setupCleanup(BModuleDef *moduleDef);

    protected:
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*);
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*);
        virtual void recordShlibSym(const std::string &name);

        virtual void engineBindModule(BModuleDef *moduleDef);

        // Contains most of the meat of engineFinishModule.
        void innerFinishModule(model::Context &context,
                               BModuleDef *moduleDef
                               );
        
        // Merge the module into the global main module, replace all of the 
        // old pointers into the original module (including the builder's) and 
        // delete it.
        void mergeModule(model::ModuleDef *moduleDef);

        virtual void engineFinishModule(model::Context &context,
                                        BModuleDef *moduleDef
                                        );
        virtual model::ModuleDefPtr innerCreateModule(model::Context &context,
                                                      const std::string &name,
                                                      model::ModuleDef *owner
                                                      );
        virtual void fixClassInstRep(BTypeDef *type);
        virtual BModuleDef *instantiateModule(model::Context &context,
                                              const std::string &name,
                                              llvm::Module *owner
                                              );

    public:
        virtual void addGlobalVarMapping(llvm::GlobalValue *decl,
                                         llvm::GlobalValue *externalDef
                                         );

        LLVMJitBuilder(void) : 
            execEng(0), 
            moduleMerger(0) {
        }
        ~LLVMJitBuilder();

        virtual void *getFuncAddr(llvm::Function *func);
        virtual void recordOrphanedDef(model::VarDef *def);

        virtual model::FuncDefPtr
            createExternFunc(model::Context &context,
                             model::FuncDef::Flags flags,
                             const std::string &name,
                             model::TypeDef *returnType,
                             model::TypeDef *receiverType,
                             const std::vector<model::ArgDefPtr> &args,
                             void *cfunc,
                             const char *symbolName=0
                             );

        virtual BuilderPtr createChildBuilder();

        void innerCloseModule(model::Context &context, 
                              model::ModuleDef *module
                              );
        
        // Merge all of the modules in the list into one and register all of 
        // the globals in it.  This lets us deal with cyclics, which have to 
        // be jitted as a single module.
        void mergeAndRegister(const std::vector<BJitModuleDefPtr> &modules);

        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *module
                                 );

        virtual bool isExec() { return true; }

        virtual void finishBuild(model::Context &context) {}

        virtual void registerDef(model::Context &context,
                                 model::VarDef *varDef
                                 );

        /**
         * Builds debug tables and registers all global symbols with the cache 
         * map.
         */
        void registerGlobals();

        virtual model::ModuleDefPtr materializeModule(
            model::Context &context,
            const std::string &canonicalName,
            model::ModuleDef *owner
        );
        virtual model::ModuleDefPtr registerPrimFuncs(model::Context &context);
        virtual llvm::ExecutionEngine *getExecEng();
};

} } // namespace

#endif

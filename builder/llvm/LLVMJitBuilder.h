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

namespace llvm {
    class ExecutionEngine;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BModuleDef);
SPUG_RCPTR(LLVMJitBuilder);

class LLVMJitBuilder : public LLVMBuilder {
    private:
        
        class Resolver {
            private:
                // the cache map tracks symbols that we've already resolved.
                typedef std::map<std::string, llvm::GlobalValue *> CacheMap;
                CacheMap cacheMap;
        
                // the fixup map maps unresolved externals to the modules that 
                // they are unresolved in.  When an entry in the fixup map is 
                // resolved, the cycle groups for all of the modules are 
                // coalesced.
                typedef std::set<llvm::Module *> ModuleSet;
                typedef std::map<std::string, ModuleSet> FixupMap;
                FixupMap fixupMap;
                
                // The unresolved map is the inverse of the fixup map - it 
                // maps modules to their unresolved externals.  When the 
                // unresolved map for a module is empty we check to see if the 
                // unresolved maps for all of the modules in its group are 
                // empty.  If they are, we run the linker on the set.
                typedef std::set<std::string> SymbolSet;
                typedef std::map<llvm::Module *, SymbolSet> UnresolvedMap;
                UnresolvedMap unresolvedMap;

                // the cycle group map keeps track of which modules are in a 
                // cycle together.  Every module in the same cycle group is 
                // mapped to the module set containing all modules in the 
                // group.
                typedef std::map<llvm::Module *, ModuleSet *> CycleMap;
                CycleMap cycleMap;
                
                // The source map keeps track of the ModuleDef object that a
                // deferred low-level module is associated with so that we can 
                // fix its reference when we run it through the linker.
                typedef std::map<llvm::Module *, BModuleDefPtr> SourceMap;
                SourceMap sourceMap;
                
                // The deferred set keeps track of symbols that have been 
                // resolved in modules that have unresolved externals and the 
                // modules that they are defined in.
                typedef std::map<std::string, llvm::Module *> 
                    DeferredMap;
                DeferredMap deferred;

                static bool trace;
                static spug::Tracer tracer;

                // add the global to 'deferred' and resolve it in fixups.
                // Returns true if the introduction of the global clears all 
                // unresolved externals for all of the modules it is 
                // unresolved in.  In this case, the caller should check for 
                // resolution of all modules participating in this cycle.
                bool deferGlobal(LLVMJitBuilder *builder, 
                                 llvm::GlobalValue *globalVal
                                 );
                
                // Merge all of the cycle groups that the two modules are in 
                // into a single cycle group referenced by both.
                void mergeCycleGroups(llvm::Module *a, llvm::Module *b);
                
                // This gets called on a module when we've discovered that all
                // the symbols in the module have been resolved in the course 
                // of resolving fixups.
                void linkCyclicGroup(LLVMJitBuilder *builder, 
                                     llvm::Module *module
                                     );

                // Resolve the fixups for a specific global value.  Returns
                // true if there are no remaining unresolved external in 
                // the modules that it is resolved in and the calling code 
                // should do a resolution check on all of the modules in the 
                // cycle.
                bool resolveFixups(LLVMJitBuilder *builder,
                                   llvm::GlobalValue *globalVal,
                                   const std::string &name
                                   );
            
            public:
                void registerGlobal(LLVMJitBuilder *builder,
                                    llvm::GlobalValue *globalVal
                                    );
                
                // Resolves the symbol if it's in the cache, otherwise adds a 
                // fix-up for it.  Returns true if ths symbol can be resolved, 
                // false if it is currently unresolvable.
                bool resolve(llvm::ExecutionEngine *execEng,
                             llvm::GlobalValue *globalVal
                             );

                // Defer all of the globals from 'module'.  If we end up 
                // getting rid of everything in the fixup map, register all of
                // the addresses in the cache map and debug info.
                void defer(LLVMJitBuilder *builder, 
                           BModuleDef *modDef
                           );
                
                /** See LLVMBuilder. */
                void checkForUnresolvedExternals();
                
                /** Returns true if the module has unresolved externals. */
                bool isUnresolved(llvm::Module *module);
        };

        llvm::ExecutionEngine *execEng;

        llvm::ExecutionEngine *bindJitModule(llvm::Module *mp);
        
        std::vector< std::pair<llvm::Function *, llvm::Function *> > externals;
        
        // symbols that were imported from a shared library (we don't want to 
        // try to resolve these).
        std::set<std::string> shlibSyms;
        std::set<std::string> &getShlibSyms() {
            if (rootBuilder)
                return LLVMJitBuilderPtr::rcast(rootBuilder)->shlibSyms;
            else
                return shlibSyms;
        }
        
        Resolver *resolver;

        virtual void run();

        virtual void dump();

        void doRunOrDump(model::Context &context);

        void ensureResolver();

        void setupCleanup(BModuleDef *moduleDef);

        /**
         * Builds debug tables and registers all global symbols with the cache 
         * map.
         */
        void registerGlobals();
    protected:
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*);
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*);
        virtual void recordShlibSym(const std::string &name);

        virtual void engineBindModule(BModuleDef *moduleDef);
        virtual void engineFinishModule(model::Context &context,
                                        BModuleDef *moduleDef
                                        );
        virtual void registerHiddenFunc(model::Context &context,
                                        BFuncDef *func
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

        LLVMJitBuilder(void) : execEng(0), resolver(0) { }
        ~LLVMJitBuilder();

        virtual void checkForUnresolvedExternals();
        virtual void *getFuncAddr(llvm::Function *func);


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

        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *module
                                 );

        virtual bool isExec() { return true; }

        virtual void finishBuild(model::Context &context) {}

        virtual void registerDef(model::Context &context,
                                 model::VarDef *varDef
                                 );

        virtual model::TypeDefPtr materializeType(model::Context &context, 
                                                  const std::string &name
                                                  );

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

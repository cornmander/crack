
#ifndef _builder_LLVMBUilder_h_
#define _builder_LLVMBUilder_h_

#include <map>

#include "Builder.h"

#include <llvm/Support/IRBuilder.h>

namespace llvm {
    class Module;
    class ModuleProvider;
    class Function;
    class BasicBlock;
    class Type;
    class Value;
    class Function;
    class ExecutionEngine;
};

namespace builder {

SPUG_RCPTR(LLVMBuilder);

class LLVMBuilder : public Builder {
    private:

        llvm::Function *func;
        
        llvm::ExecutionEngine *execEng;
        
        // emit all cleanups for context and all parent contextts up to the 
        // level of the function
        void emitFunctionCleanups(model::Context &context);
        
        // stores primitive function pointers
        std::map<llvm::Function *, void *> primFuncs;
        
        // keeps track of the Function object for the FuncDef in the builder's 
        // module.
        typedef std::map<model::FuncDef *, llvm::Function *> ModFuncMap;
        ModFuncMap moduleFuncs;
        
        // keeps track of the GlobalVariable object for the VarDef in the 
        // builder's module.
        typedef std::map<model::VarDefImpl *, llvm::GlobalVariable *> ModVarMap;
        ModVarMap moduleVars;

        LLVMBuilderPtr rootBuilder;
        
        llvm::ExecutionEngine *bindModule(llvm::ModuleProvider *mp);

    public:
        // currently experimenting with making these public to give objects in 
        // LLVMBuilder.cc's anonymous internal namespace access to them.  It 
        // seems to be cutting down on the amount of code necessary to do this.
        llvm::Module *module;
        llvm::Type *llvmVoidPtrType;
        llvm::IRBuilder<> builder;
        llvm::Value *lastValue;
        llvm::BasicBlock *block;

        void setModFunc(model::FuncDef *funcDef, llvm::Function *func) {
            moduleFuncs[funcDef] = func;
        }

        llvm::Function *getModFunc(model::FuncDef *funcDef);

        void setModVar(model::VarDefImpl *varDef, llvm::GlobalVariable *var) {
            moduleVars[varDef] = var;
        }

        llvm::GlobalVariable *getModVar(model::VarDefImpl *varDef);
        
        LLVMBuilder();

        virtual BuilderPtr createChildBuilder();

        virtual model::ResultExprPtr emitFuncCall(
            model::Context &context, 
            model::FuncCall *funcCall
        );
        
        virtual model::ResultExprPtr emitStrConst(model::Context &context,
                                                  model::StrConst *strConst
                                                  );

        virtual model::ResultExprPtr emitIntConst(model::Context &context,
                                                  model::IntConst *val
                                                  );

        virtual model::ResultExprPtr emitNull(model::Context &context,
                                              model::NullConst *nullExpr
                                              );

        virtual model::ResultExprPtr emitAlloc(model::Context &context, 
                                               model::AllocExpr *allocExpr
                                               );

        virtual void emitTest(model::Context &context,
                              model::Expr *expr
                              );

        virtual model::BranchpointPtr emitIf(model::Context &context,
                                             model::Expr *cond);
        
        virtual model::BranchpointPtr
            emitElse(model::Context &context,
                     model::Branchpoint *pos,
                     bool terminal
                     );
        
        virtual void emitEndIf(model::Context &context,
                               model::Branchpoint *pos,
                               bool terminal
                               );

        virtual model::BranchpointPtr 
            emitBeginWhile(model::Context &context, 
                           model::Expr *cond);

        virtual void emitEndWhile(model::Context &context,
                                  model::Branchpoint *pos);

        virtual model::FuncDefPtr
            emitBeginFunc(model::Context &context,
                          model::FuncDef::Flags flags,
                          const std::string &name,
                          model::TypeDef *returnType,
                          const std::vector<model::ArgDefPtr> &args);
        
        virtual void emitEndFunc(model::Context &context,
                                 model::FuncDef *funcDef);

        virtual model::FuncDefPtr
            createExternFunc(model::Context &context,
                             model::FuncDef::Flags flags,
                             const std::string &name,
                             model::TypeDef *returnType,
                             const std::vector<model::ArgDefPtr> &args,
                             void *cfunc
                             );

        virtual model::TypeDefPtr
            emitBeginClass(model::Context &context,
                           const std::string &name,
                           const std::vector<model::TypeDefPtr> &bases);

        virtual void emitEndClass(model::Context &context);

        virtual void emitReturn(model::Context &context,
                                model::Expr *expr);

        virtual model::VarDefPtr emitVarDef(model::Context &container,
                                            model::TypeDef *type,
                                            const std::string &name,
                                            model::Expr *initializer,
                                            bool staticScope
                                            );

        // for definitions, we're going to just let the builder be a factory 
        // so that it can tie whatever special information it wants to the 
        // definition.
        
        virtual model::FuncCallPtr
            createFuncCall(model::FuncDef *func);
        virtual model::ArgDefPtr createArgDef(model::TypeDef *type,
                                              const std::string &name
                                              );
        virtual model::VarRefPtr createVarRef(model::VarDef *varDef);
        virtual model::VarRefPtr
            createFieldRef(model::Expr *aggregate,
                           model::VarDef *varDef
                           );
        virtual model::ResultExprPtr emitFieldAssign(model::Context &context,
                                                     model::AssignExpr *assign
                                                     );

        virtual void emitNarrower(model::TypeDef &curType,
                                  model::TypeDef &parent,
                                  int index
                                  );
        virtual void createModule(model::Context &context,
                                  const std::string &name);
        virtual void closeModule();
        virtual model::CleanupFramePtr
            createCleanupFrame(model::Context &context);
        virtual void closeAllCleanups(model::Context &context);
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val);
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  long val);
        virtual void registerPrimFuncs(model::Context &context);
        virtual void loadSharedLibrary(const std::string &name,
                                       const std::vector<std::string> &symbols,
                                       model::Context &context
                                       );
        virtual model::VarDefPtr createImport(model::Context &context, 
                                              model::VarDef *varDef
                                              );
        
        virtual void run();
        virtual void dump();
        
        // internal functions used by our VarDefImpl to generate the 
        // appropriate variable references.
        void emitMemVarRef(model::Context &context, llvm::Value *val);
        void emitArgVarRef(model::Context &context, llvm::Value *val);
};

} // namespace builder
#endif


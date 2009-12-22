
#ifndef _builder_LLVMBUilder_h_
#define _builder_LLVMBUilder_h_

#include "Builder.h"

#include <llvm/Support/IRBuilder.h>

namespace llvm {
    class Module;
    class Function;
    class BasicBlock;
    class Type;
    class Value;
    class Function;
    class ExecutionEngine;
};

namespace builder {

class LLVMBuilder : public Builder {
    private:

        llvm::Function *func;
        
        llvm::ExecutionEngine *execEng;

    public:
        // currently experimenting with making these public to give objects in 
        // LLVMBuilder.cc's anonymous internal namespace access to them.  It 
        // seems to be cutting down on the amount of code necessary to do this.
        llvm::Module *module;
        llvm::Type *llvmVoidPtrType;
        llvm::IRBuilder<> builder;
        llvm::Value *lastValue;
        llvm::BasicBlock *block;

        LLVMBuilder();

        virtual void emitFuncCall(model::Context &context, 
                                  model::FuncDef *func,
                                  model::Expr *receiver,
                                  const model::FuncCall::ExprVec &args
                                  );
        
        virtual void emitStrConst(model::Context &context,
                                  model::StrConst *strConst
                                  );
        
        virtual void emitIntConst(model::Context &context,
                                  const model::IntConst &val);

        virtual void emitNull(model::Context &context,
                              const model::TypeDef &type
                              );

        virtual void emitAlloc(model::Context &context, model::TypeDef *type);

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
        virtual void emitFieldAssign(model::Context &context,
                                     model::Expr *aggregate,
                                     model::VarDef *varDef,
                                     model::Expr *val
                                     );

        virtual void emitNarrower(model::TypeDef &curType,
                                  model::TypeDef &parent,
                                  int index
                                  );
        virtual void createModule(const char *name);
        virtual void closeModule();
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val);
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  long val);
        virtual void registerPrimFuncs(model::Context &context);
        
        virtual void run();
        virtual void dump();
        
        // internal functions used by our VarDefImpl to generate the 
        // appropriate variable references.
        void emitMemVarRef(model::Context &context, llvm::Value *val);
        void emitArgVarRef(model::Context &context, llvm::Value *val);
};

} // namespace builder
#endif


// Copyright 2009 Google Inc.

#ifndef _model_FuncDef_h_
#define _model_FuncDef_h_

#include <vector>
#include "TypeDef.h"
#include "VarDef.h"

namespace model {

SPUG_RCPTR(ArgDef);
SPUG_RCPTR(Expr);

SPUG_RCPTR(FuncDef);

class FuncDef : public VarDef {
    public:
        enum Flags {
            noFlags =0,  // so we can specify this
            method = 1,  // function is a method (has a receiver)
            virtualized = 2, // function is virtual
        } flags;
        
        typedef std::vector<ArgDefPtr> ArgVec;
        ArgVec args;
        TypeDefPtr returnType;

        // for a virtual function, this is the path to the base class 
        // where the vtable is defined
        TypeDef::AncestorPath pathToFirstDeclaration;

        FuncDef(Flags flags, const std::string &name, size_t argCount);
        
        /**
         * Returns true if 'args' matches the types of the functions 
         * arguments.
         * 
         * @param newValues the set of converted values.  This is only
         *        constructed if 'convert' is true.
         * @param convert if true, attempt to construct the value if it does 
         *        not exist.
         */
        virtual bool matches(Context &context, 
                             const std::vector<ExprPtr> &vals,
                             std::vector<ExprPtr> &newVals,
                             bool convert
                             );
        
        /**
         * Returns true if the arg list matches the functions args.
         */
        bool matches(const ArgVec &args);
        
        /**
         * Returns true if the function can be overriden.
         */
        bool isOverridable() const;
        
        virtual bool hasInstSlot();

        /**
         * Returns the "receiver type."  For a non-virtual function, this 
         * is simply the type that the function was declared in.  For a 
         * virtual function, it is the type of the base class in which the 
         * function was first declared.
         */
        TypeDef *getReceiverType() const;
        
        /**
         * Returns the "this" type of the function.  This is always either 
         * the receiver type or a specialization of it.
         */
        TypeDef *getThisType() const;
        
        
        virtual
        void dump(std::ostream &out, const std::string &prefix = "") const;
};

inline FuncDef::Flags operator |(FuncDef::Flags a, FuncDef::Flags b) {
    return static_cast<FuncDef::Flags>(static_cast<int>(a) | 
                                       static_cast<int>(b));
}
        
} // namespace model

#endif

// Copyright 2009 Google Inc.

#ifndef _model_Expr_h_
#define _model_Expr_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(ResultExpr);

SPUG_RCPTR(Expr);

// Base class for everything representing an expression.
class Expr : public spug::RCBase {
    public:
        TypeDefPtr type;
        
        Expr(TypeDef *type);
        
        ~Expr();

        /**
         * Emit the expression in the given context.
         * 
         * Returns a result expression to be used for object lifecycle
         * management.
         */
        virtual ResultExprPtr emit(Context &context) = 0;

        /**
         * Emit the expression for use in a conditional context.
         * This defaults to calling Builder::emitTest().  Builder-derived 
         * classes should override in cases where there is a more appropriate 
         * path.
         */
        virtual void emitCond(Context &context);
        
        /**
         * Returns an expression that converts the expression to the specified 
         * type, null if it cannot be converted.
         */
        virtual ExprPtr convert(Context &context, TypeDef *type);

        /** Returns true if the expression is productive (see /notes.txt). */
        virtual bool isProductive() const;
        
        /**
         * Returns true if the expression changes type to adapt to the 
         * requirements of the context (integer and float constants).
         */
        virtual bool isAdaptive() const;
        
        /**
         * Write a representation of the expression to the stream.
         */
        virtual void writeTo(std::ostream &out) const = 0;
        
        /**
         * Do const folding on the expression.  Returns either the original 
         * expression object or a folded constant.
         * The base class version of this always returns null.  Derived 
         * classes may implement it for operations that support const folding.
         */
        virtual ExprPtr foldConstants();
        
        void dump() const;
};

inline std::ostream &operator <<(std::ostream &out, const Expr &expr) {
    expr.writeTo(out);
    return out;
}

} // namespace model

#endif

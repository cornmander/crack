
#ifndef _model_Expr_h_
#define _model_Expr_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(Expr);

// Base class for everything representing an expression.
class Expr : public spug::RCBase {
    public:
        TypeDefPtr type;
        
        Expr(TypeDef *type) : type(type) {}

        /** Emit the expression in the given context. */
        virtual void emit(Context &context) = 0;        
        
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
        virtual ExprPtr convert(Context &context, const TypeDef &type);
};

} // namespace model

#endif

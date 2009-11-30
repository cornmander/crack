
#ifndef _model_TypeDef_h
#define _model_TypeDef_h

#include <spug/RCPtr.h>

#include "VarDef.h"

namespace model {

SPUG_RCPTR(Context);
SPUG_RCPTR(Expr);
SPUG_RCPTR(VarDef);

// a type.
class TypeDef : public VarDef {
    public:
        
        // the type's context - contains all of the method/attribute 
        // definitions for the type.
        ContextPtr context;
        
        // the default initializer expression (XXX I'm not sure that we want 
        // to keep this, for now it's expedient to be able to do variable 
        // initialization without the whole "oper new" business)
        ExprPtr defaultInitializer;
        
        // XXX need a metatype
        TypeDef(const std::string &name) : VarDef(0, name) {}
        
        /** Emit a variable definition for the type. */
        VarDefPtr emitVarDef(Context &container, const std::string &name,
                             const ExprPtr &initializer
                             );
        
        /** 
         * Returns true if "other" satisfies the type - in other words, if 
         * "other" either equals "this" or is a subclass of "this".
         */
        bool matches(const TypeDef &other);

        /**
         * Emit code to "narrow" the type context to the specified 
         * target type (which must be a base class of this type).
         * 
         * This can be used by a Builder to force the generation of special 
         * code to refocus a field or method reference to a specific ancestor 
         * class.
         * 
         * The base implementation recursively calls itself if the expression 
         * type's context is not "target", then calls Builder::emitNarrower() 
         * to narrow down from the specific type.  It may be overriden to 
         * provide functionality better suited to the Builder.
         * 
         * @return true if target is a base type.
         */
        virtual bool emitNarrower(TypeDef &target);

};

} // namespace model


#endif

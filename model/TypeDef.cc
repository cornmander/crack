
#include "TypeDef.h"

#include "builder/Builder.h"
#include "Context.h"
#include "VarDef.h"

using namespace model;

VarDefPtr TypeDef::emitVarDef(Context &container, const std::string &name,
                               const ExprPtr &initializer
                               ) {
    return container.builder.emitVarDef(container, this, name, initializer);
}

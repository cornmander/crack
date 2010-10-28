// Copyright 2009 Google Inc.

#include "VarDef.h"

#include "AssignExpr.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "Expr.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace std;
using namespace model;

VarDef::VarDef(TypeDef *type, const std::string &name) :
    type(type),
    name(name),
    owner(0) {
}

VarDef::~VarDef() {}

ResultExprPtr VarDef::emitAssignment(Context &context, Expr *expr) {
    AssignExprPtr assign = new AssignExpr(0, this, expr);
    return impl->emitAssignment(context, assign.get());
}

bool VarDef::hasInstSlot() {
    return true;
}

std::string VarDef::getFullName() const {
    if (!fullName.empty())
        return fullName;
    if (owner && !owner->getNamespaceName().empty())
        fullName = owner->getNamespaceName()+"."+name;
    else
        fullName = name;
    return fullName;
}

void VarDef::dump(ostream &out, const string &prefix) const {
    out << prefix << type->getFullName() << " " << name << endl;
}

void VarDef::dump() const {
    dump(std::cerr, "");
}

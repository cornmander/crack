// Copyright 2009 Google Inc.

#ifndef _model_CompositeNamespace_h_
#define _model_CompositeNamespace_h_

#include <vector>
#include "Namespace.h"

namespace model {

SPUG_RCPTR(CompositeNamespace);

/** A virtual namespace that delegates to several other namespaces. */
class CompositeNamespace : public Namespace {
    private:
        std::vector<NamespacePtr> parents;

    public:
        CompositeNamespace(Namespace *parent0, Namespace *parent1);
        virtual NamespacePtr getParent(unsigned index);
        virtual void addDef(VarDef *def);
        virtual void removeDef(VarDef *def);
        virtual void addAlias(VarDef *def);
        virtual void addAlias(const std::string &name, VarDef *def);
        virtual void replaceDef(VarDef *def);
};

} // namespace model

#endif

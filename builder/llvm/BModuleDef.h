// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BModuleDef_h_
#define _builder_llvm_BModuleDef_h_

#include "model/ModuleDef.h"
#include <spug/RCPtr.h>
#include <string>

namespace model {
    class Context;
}

namespace llvm {
    class Module;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BModuleDef);

class BModuleDef : public model::ModuleDef {

public:
    // primitive cleanup function
    void (*cleanup)();
    llvm::Module *rep;

    BModuleDef(const std::string &canonicalName,
               model::Namespace *parent,
               llvm::Module *rep0) :
            ModuleDef(canonicalName, parent),
            cleanup(0), rep(rep0) {
    }

    void callDestructor() {
        if (cleanup)
            cleanup();
    }
};

} // end namespace builder::vmll
} // end namespace builder

#endif

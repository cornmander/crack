// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Cacher_h_
#define _builder_llvm_Cacher_h_

#include "model/Context.h"
#include <string>

namespace llvm {
    class Module;
    class MDNode;
    class Value;
}

namespace model {
    class VarDef;
}

namespace builder {

class BuilderOptions;

namespace mvll {

class BModuleDef;

class Cacher {

    static const std::string MD_VERSION;

    enum DefTypes {
        global = 0,
        function = 1
    };

    BModuleDef *modDef;
    model::Context &context;
    const builder::BuilderOptions *options;

protected:
    void addNamedStringNode(const std::string &key, const std::string &val);
    std::string getNamedStringNode(const std::string &key);

    llvm::MDNode *writeVarDef(model::VarDef *);
    llvm::MDNode *writeFuncDef(model::FuncDef *);

    void readFuncDef(const std::string &, llvm::Value *, llvm::MDNode *);

    void writeBitcode(const std::string &path);

    bool readImports();
    void readDefs();

    void writeMetadata();
    bool readMetadata();

public:

    Cacher(model::Context &c, builder::BuilderOptions* o, BModuleDef *m = NULL):
        modDef(m), context(c), options(o) { }

    BModuleDef *maybeLoadFromCache(const std::string &canonicalName,
                                   const std::string &path);
    void saveToCache();

};

} // end namespace builder::vmll
} // end namespace builder

#endif

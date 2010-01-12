
#include "Crack.h"

#include <sys/stat.h>
#include <fstream>
#include "model/Context.h"
#include "model/ModuleDef.h"
#include "parser/Parser.h"
#include "parser/ParseError.h"
#include "parser/Toker.h"
#include "builder/Builder.h"
#include "builder/LLVMBuilder.h"

using namespace std;
using namespace model;
using namespace parser;
using namespace builder;

Crack *Crack::theInstance = 0;

Crack &Crack::getInstance() {
    if (!theInstance)
        theInstance = new Crack();
    
    return *theInstance;
}

Crack::Crack() : 
    sourceLibPath(1), 
    rootBuilder(new LLVMBuilder()), 
    dump(false) {

    rootContext = new Context(*rootBuilder, Context::module, (Context *)0);

    // register the primitives    
    rootBuilder->registerPrimFuncs(*rootContext);
    
    // search for source files in the current directory
    sourceLibPath[0] = ".";
}

//Crack::~Crack() {}

std::string Crack::joinName(const std::string &base,
                            Crack::StringVecIter pathBegin,
                            Crack::StringVecIter pathEnd,
                            const std::string &ext
                            ) {
    // not worrying about performance, this only happens during module load.
    string result = base;
    
    // base off current directory if an empty path was specified.
    if (!result.size())
        result = ".";

    for (StringVecIter iter = pathBegin;
         iter != pathEnd;
         ++iter
         ) {
        result += "/" + *iter;
    }

    return result + ext;
}

bool Crack::isFile(const std::string &name) {
    struct stat st;
    if (stat(name.c_str(), &st))
        return false;
    
    // XXX should check symlinks, too
    return S_ISREG(st.st_mode);
}

bool Crack::isDir(const std::string &name) {
    struct stat st;
    if (stat(name.c_str(), &st))
        return false;
    
    // XXX should check symlinks, too
    return S_ISDIR(st.st_mode);
}

Crack::ModulePath Crack::searchPath(const Crack::StringVec &path,
                                    Crack::StringVecIter moduleNameBegin,
                                    Crack::StringVecIter moduleNameEnd,
                                    const std::string &extension
                                    ) {
    // try to find a matching file.
    for (StringVecIter pathIter = path.begin();
         pathIter != path.end();
         ++pathIter
         ) {
        string fullName = joinName(*pathIter, moduleNameBegin, moduleNameEnd,
                                   extension
                                   );
        if (isFile(fullName))
            return ModulePath(fullName, true, false);
    }
    
    // try to find a matching directory.
    string empty;
    for (StringVecIter pathIter = path.begin();
         pathIter != path.end();
         ++pathIter
         ) {
        string fullName = joinName(*pathIter, moduleNameBegin, moduleNameEnd,
                                   empty
                                   );
        if (isDir(fullName))
            return ModulePath(fullName, true, true);
    }
    
    return ModulePath(empty, false, false);
}

void Crack::parseModule(ModuleDef *module,
                        const std::string &path,
                        istream &src
                        ) {
    module->create();
    Toker toker(src, path.c_str());
    Parser parser(toker, module->moduleContext.get());
    parser.parse();
    module->close();
    if (dump)
        module->moduleContext->builder.dump();
    else
        module->moduleContext->builder.run();
}

ModuleDefPtr Crack::loadModule(Crack::StringVecIter moduleNameBegin,
                               Crack::StringVecIter moduleNameEnd,
                               string &canonicalName
                               ) {
    // create the dotted canonical name of the module
    for (StringVecIter iter = moduleNameBegin;
         iter != moduleNameEnd;
         ++iter
         )
        if (canonicalName.size())
            canonicalName += "." + *iter;

    // check to see if we have it in the cache
    ModuleMap::iterator iter = moduleCache.find(canonicalName);
    if (iter != moduleCache.end())
        return iter->second;

    // load the parent module
    StringVec::const_reverse_iterator rend(moduleNameEnd);
    ++rend;
    if (rend != StringVec::const_reverse_iterator(moduleNameBegin)) {
        StringVecIter last(rend.base());
        ModuleDefPtr parent = loadModule(moduleNameBegin, last, canonicalName);
    }
    
    // try to find the module on the source path
    ModulePath modPath = searchPath(sourceLibPath, moduleNameBegin, 
                                    moduleNameEnd,
                                    ".crk"
                                    );
    if (!modPath.found)
        return 0;

    // create a new builder, context and module
    BuilderPtr builder = rootBuilder->createChildBuilder();
    ContextPtr context =
        new Context(*builder, Context::module, rootContext.get());
    ModuleDefPtr modDef = new ModuleDef(canonicalName, context.get());
    if (!modPath.isDir) {
        ifstream src(modPath.path.c_str());
        parseModule(modDef.get(), modPath.path, src);
    }
    
    moduleCache[canonicalName] = modDef;
    return modDef;
}    

int Crack::runScript(std::istream &src, const std::string &name) {
    BuilderPtr builder = rootBuilder->createChildBuilder();
    ContextPtr context =
        new Context(*builder, Context::module, rootContext.get());

    // XXX using the name as the canonical name which is not right, need to 
    // produce a canonical name from the file name, e.g. "foo" -> "foo", 
    // "foo.crk" -> foo, "anything weird" -> "__main__" or something.
    ModuleDefPtr modDef = new ModuleDef(name, context.get());
    try {
        parseModule(modDef.get(), name, src);
    } catch (const ParseError &ex) {
        cerr << ex << endl;
    }
}

ModuleDefPtr Crack::loadModule(const vector<string> &moduleName,
                              string &canonicalName
                              ) {
    return getInstance().loadModule(moduleName.begin(), 
                                    moduleName.end(), 
                                    canonicalName
                                    );
}
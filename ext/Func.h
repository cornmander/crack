// Copyright 2010 Google Inc.

#ifndef _crack_ext_Func_h_
#define _crack_ext_Func_h_

#include <string>
#include <vector>

namespace crack { namespace ext {

struct Arg;
class Module;
class Type;

class Func {
    private:
        Module *module;
        Type *returnType;
        std::string name;
        void *funcPtr;
        std::vector<Arg *> args;
        bool finished;

    public:
        Func(Module *module, Type *returnType, std::string name, 
             void *funcPtr
             ) :
            module(module),
            returnType(returnType),
            name(name),
            funcPtr(funcPtr),
            finished(false) {
        }

        // add a new argument to the function.
        void addArg(Type *type, const std::string &name);

        // finish the definition of the function (this will be called 
        // automatically by Module::finish())
        void finish();
};


}} // namespace crack::ext

#endif


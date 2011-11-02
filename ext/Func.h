// Copyright 2010 Google Inc.

#ifndef _crack_ext_Func_h_
#define _crack_ext_Func_h_

#include <string>
#include <vector>

namespace model {
    class Context;
}

namespace crack { namespace ext {

struct Arg;
class Module;
class Type;

class Func {
    friend class Module;
    friend class Type;

    public:
        enum Flags {
            noFlags = 0,
            method = 1,
            virtualized = 2,
            variadic = 8,

            // mask for flags that map to FuncDef flags.
            funcDefFlags = 15,

            // flags that don't map to FuncDef flags.
            constructor = 1024
        };

    private:
        model::Context *context;
        Type *returnType;
        std::string name;
        std::string symbolName;
        void *funcPtr;
        std::vector<Arg *> args;

        std::string funcBody;
        
        // these must match the values in FuncDef::Flags
        Flags flags;
        bool finished;

        Func(model::Context *context, Type *returnType, std::string name, 
             void *funcPtr,
             Flags flags
             ) :
            context(context),
            returnType(returnType),
            name(name),
            funcPtr(funcPtr),
            flags(flags),
            finished(false) {
        }

        Func(model::Context *context, Type *returnType, std::string name,
             const std::string& body,
             Flags flags
             ) :
            context(context),
            returnType(returnType),
            name(name),
            funcPtr(0),
            funcBody(body),
            flags(flags),
            finished(false) {
        }

    public:

        // specify a symbol name for this function, which will be used in
        // low level IR to resolve calls to this function. if it's not
        // specified, at attempt is made to reverse it from the function
        // pointer
        void setSymbolName(const std::string &name) { symbolName = name; }

        // add a new argument to the function.
        void addArg(Type *type, const std::string &name);

        // sets whether the Func maps to a variadic function
        void setIsVariadic(bool isVariadic);

        // gets whether the Func maps to a variadic function
        bool isVariadic() const;

        // sets the function body; sets funcPtr to 0
        void setBody(const std::string& body);

        // returns the function body
        std::string body() const;

        // finish the definition of the function (this will be called 
        // automatically by Module::finish())
        void finish();
};

inline Func::Flags operator |(Func::Flags a, Func::Flags b) {
    return static_cast<Func::Flags>(static_cast<int>(a) |
                                    static_cast<int>(b)
                                    );
}

}} // namespace crack::ext

#endif


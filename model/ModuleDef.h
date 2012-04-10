// Copyright 2010 Google Inc.

#ifndef _model_ModuleDef_h_
#define _model_ModuleDef_h_

#include <vector>
#include "Namespace.h"
#include "VarDef.h"

namespace model {

SPUG_RCPTR(Context);

SPUG_RCPTR(ModuleDef);

/**
 * A module.
 * The context of a module is the parent module.
 */
class ModuleDef : public VarDef, public Namespace {
    public:
        typedef std::vector<std::string> StringVec;

        // the parent namespace.  This should be the root namespace where 
        // builtins are stored.
        NamespacePtr parent;

        // this is true if the module has been completely parsed and the
        // close() method has been called.
        bool finished;

        // true if this module was generated from an extension (as opposed
        // to crack source)
        bool fromExtension;

        // aliased symbols that other modules are allowed to import.
        std::map<std::string, bool> exports;

        // path to original source code on disk
        std::string sourcePath;

        ModuleDef(const std::string &name, Namespace *parent);

        /**
         * Close the module, executing it.
         */
        void close(Context &context);
        
        /**
         * Call the module destructor - cleans up all global variables.
         */
        virtual void callDestructor() = 0;
        
        virtual bool hasInstSlot();

        /**
         * Set namespace owner, and set our namespace name
         */
        virtual void setOwner(Namespace *o) {
            owner = o;
            canonicalName = o->getNamespaceName()+"."+name;
            fullName.clear();
        }

        /**
         * Record a dependency on another module.  See 
         * model::Context::recordDependency() for more info.
         * Derived classes should override if it's important.
         */
        virtual void recordDependency(ModuleDef *other) {}

        /**        
         * Returns true if the module matches the source file found along the 
         * given path.
         */
        bool matchesSource(const StringVec &libSearchPath);
        
        /**
         * Returns true if the module matches the specific source file.
         */
        virtual bool matchesSource(const std::string &sourcePath) = 0;

        virtual NamespacePtr getParent(unsigned index);
        virtual ModuleDefPtr getModule();
        
        /**
         * Parse a canonical module name, return it as a vector of name 
         * components.
         */
        static StringVec parseCanonicalName(const std::string &name);
};

} // namespace model

#endif

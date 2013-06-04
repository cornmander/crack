// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ModulePlaceholder_h_
#define _model_ModulePlaceholder_h_

#include "ModuleDef.h"

#include <set>
#include <vector>

// we need to include this because of TypeDef::TypeVecObj
#include "TypeDef.h"

namespace model {

class Context;
SPUG_RCPTR(OverloadDef);
SPUG_RCPTR(ModuleStub);

class ModuleStub : public ModuleDef {
    public:

        struct Callback {
            virtual void run() = 0;
        };

        typedef std::vector<Callback *> CallbackVec;
        CallbackVec callbacks;

        ModuleDefPtr replacement;
        bool replacedAll;

        // modules that depend on the stub and need to be fixed.
        std::set<ModuleDef *> dependents;

        ModuleStub(const std::string &name) :
            ModuleDef(name, 0),
            replacedAll(false) {
        }

        ~ModuleStub();

        virtual void callDestructor() {}
        virtual void runMain(builder::Builder &builder) {}

        /**
         * These functions get placeholders for symbols defined inside the
         * module.
         */
        /** @{ */
        TypeDefPtr getTypeStub(const std::string &name);
        OverloadDefPtr getOverloadStub(const std::string &name);
        VarDefPtr getVarStub(const std::string &name);
        /** @} */

        /**
         * Replace this stub in all modules in 'dependents'.
         */
        void replace(Context &context);

        virtual TypeDefPtr getType(const std::string &name);

        /**
         * Registers the callback to be called after the stub is replaced.
         * Ownership of the callback is transferred to the ModuleStub.
         */
        void registerCallback(Callback *callback);

        /**
         * Returns a stubbed type for a generic with stubbed parameters.
         * @param dependent The module that depends on the new stub.
         * @param stub The first stubbed parameter.
         * @param generic the original generic type.
         * @param types the generic's type parameters.
         */
        static TypeDefPtr createGenericStub(ModuleDef *dependent,
                                            TypeDef *stub,
                                            TypeDef *generic,
                                            TypeDef::TypeVecObj *types
                                            );
};

} // namespace model

#endif

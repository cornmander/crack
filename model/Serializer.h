// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Serializer_h_
#define _model_Serializer_h_

#include <map>
#include <string>

namespace model {

class ModuleDef;

class Serializer {
    private:
        std::ostream &dst;

        // mapping from a pointer to the object id associated with that
        // pointer.  This is part of the mechanism that allows us to serialize
        // an object that is used in multiple locations only the first time it
        // is used.
        typedef std::map<const void *, int> ObjMap;
        ObjMap objMap;
        int lastId;

    public:
        const ModuleDef *module;

        Serializer(std::ostream &dst) : dst(dst), lastId(0), module(0) {}

        /** Serialize an integer. */
        void write(unsigned int val);

        /** Serialize byte data (writes the length followed by the bytes) */
        void write(size_t length, const void *data);

        /** Convenience method for writing strings. */
        void write(const std::string &str) {
            write(str.size(), str.data());
        }

        /**
         * If we have "object" before, serialize its id with the "definition"
         * flag set to false and return false.  Otherwise serialize a new
         * identifier with a definition flag set to true and return true,
         * indicating that the caller should serialize the state of the object.
         */
        bool writeObject(const void *object);
};

}

#endif

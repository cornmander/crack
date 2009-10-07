
#ifndef _model_Def_h_
#define _model_Def_h_

#include <string>
#include <spug/RCBase.h>

namespace model {

SPUG_RCPTR(Def);

// Base class for definitions.
class Def : public spug::RCBase {
    public:
        std::string name;
        
        Def(const char *name) : name(name) {}
};

} // namespace model

#endif

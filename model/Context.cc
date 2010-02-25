// Copyright 2009 Google Inc.

#include "Context.h"

#include "builder/Builder.h"
#include "BuilderContextData.h"
#include "CleanupFrame.h"
#include "ArgDef.h"
#include "IntConst.h"
#include "OverloadDef.h"
#include "StrConst.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace std;

Context::GlobalData::GlobalData() : 
    objectType(0), stringType(0), staticStringType(0) {
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext
                 ) :
    builder(builder),
    scope(scope),
    complete(false),
    toplevel(false),
    emittingCleanups(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    globalData(parentContext ? parentContext->globalData : new GlobalData()),
    cleanupFrame(builder.createCleanupFrame(*this)) {
    
    if (parentContext)
        parents.push_back(parentContext);
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context::GlobalData *globalData
                 ) :
    builder(builder),
    scope(scope),
    complete(false),
    toplevel(false),
    emittingCleanups(false),
    returnType(TypeDefPtr(0)),
    globalData(globalData),
    cleanupFrame(builder.createCleanupFrame(*this)) {
}

Context::~Context() {}

ContextPtr Context::createSubContext(Scope newScope) {
    return new Context(builder, newScope, this);
}

ContextPtr Context::getClassContext() {
    if (scope == instance)
        return this;

    ContextPtr result;
    for (ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (result = (*iter)->getClassContext()) break;
    
    return result;
}

ContextPtr Context::getDefContext() {
    if (scope != composite)
        return this;
    
    ContextPtr result;
    for (ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (result = (*iter)->getDefContext()) break;
    
    return result;
}

void Context::createModule(const string &name) {
    builder.createModule(*this, name);
}

namespace {
    inline void allocOverload(OverloadDefPtr &overload, const string &name) {
        if (!overload)
            overload = new OverloadDef(name);
    }
    
    inline void mergeOverloads(OverloadDef::FuncList &aggregator,
                               const OverloadDef::FuncList &newSubset
                               ) {
        for (OverloadDef::FuncList::const_iterator iter = newSubset.begin();
             iter != newSubset.end();
             ++iter
             ) 
            aggregator.push_back(*iter);
    }
}

OverloadDefPtr Context::getOverload(const std::string &varName, 
                                    bool alwaysCreate
                                    ) {
    // see if the name exists in the current context
    OverloadDefPtr overloads = lookUp(varName, false);
    if (overloads)
        return overloads;

    // if we have to always create one of these, do so now.
    if (alwaysCreate)
        overloads = new OverloadDef(varName);
    
    // merge in the overloads from the parents
    if (parents.size())
        for (ContextVec::iterator iter = parents.begin();
             iter != parents.end();
             ++iter
             ) {
            // XXX somebody somewhere needs to check for collisions
            
            // get the parent overloads if there are any
            OverloadDefPtr parentOverloads = 
                (*iter)->getOverload(varName, false);
            
            // merge the parent overloads with this one (creating this one if 
            // necessary)
            if (parentOverloads) {
                if (!overloads)
                    overloads = new OverloadDef(varName);
                overloads->merge(*parentOverloads);
            }
        }

    if (overloads) {
        defs[varName] = overloads;
        overloads->context = this;
    }

    return overloads;
}
        

VarDefPtr Context::lookUp(const std::string &varName, bool recurse) {
    VarDefMap::iterator iter = defs.find(varName);
    if (iter != defs.end()) {
        return iter->second;
    } else if (recurse && parents.size()) {
        
        // try to find the definition in the parents
        VarDefPtr def;
        for (ContextVec::iterator parent_iter = parents.begin();
             parent_iter != parents.end();
             ++parent_iter
             )
            if (def = (*parent_iter)->lookUp(varName))
                break;
        
        // if we got an overload, we need to create an overload in this 
        // context.
        OverloadDef *overload = OverloadDefPtr::rcast(def);
        if (overload)
            return getOverload(varName);
        else
            return def;
    }

    return 0;
}

FuncDefPtr Context::lookUp(Context &context,
                           const std::string &varName,
                           vector<ExprPtr> &args
                           ) {
    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName);
    if (!var)
        return 0;

    // if "var" is a class definition, convert this to a lookup of the "oper 
    // new" function on the class.
    TypeDef *typeDef = TypeDefPtr::rcast(var);
    if (typeDef)
        return typeDef->context->lookUp(context, "oper new", args);

    // if this is an overload, get the function from it.
    OverloadDefPtr overload = OverloadDefPtr::rcast(var);
    if (!overload)
        return 0;
    return overload->getMatch(context, args);
}

FuncDefPtr Context::lookUpNoArgs(const std::string &name) {
    OverloadDefPtr overload = getOverload(name);
    if (!overload)
        return 0;

    // we can just check for a signature match here - cheaper and easier.
    FuncDef::ArgVec args;
    return overload->getSigMatch(args);
}

void Context::addDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");

    // if this is a function, create an overload definition
    FuncDef *funcDef;
    if (funcDef = FuncDefPtr::cast(def)) {
        OverloadDefPtr overloads = getOverload(def->name);
        overloads->addFunc(funcDef);
    } else {
        // not an overload.  Just add it.
        defs[def->name] = def;
    }
    def->context = this;
}

void Context::removeDef(VarDef *def) {
    assert(!OverloadDefPtr::cast(def));
    VarDefMap::iterator iter = defs.find(def->name);
    assert(iter != defs.end());
    defs.erase(iter);
}

void Context::addAlias(VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->context);
    defs[def->name] = def;
}

void Context::addAlias(const string &name, VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->context);
    defs[name] = def;
}

void Context::replaceDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");
    def->context = this;
    defs[def->name] = def;
}

ExprPtr Context::getStrConst(const std::string &value) {
    
    // look up the raw string constant
    StrConstPtr strConst;
    StrConstTable::iterator iter = globalData->strConstTable.find(value);
    if (iter != globalData->strConstTable.end()) {
        strConst = iter->second;
    } else {
        // create a new one
        strConst = builder.createStrConst(*this, value);
        globalData->strConstTable[value] = strConst;
    }
    
    // if we don't have a StaticString type yet, we're done.
    if (!globalData->staticStringType)
        return strConst;
    
    // create the "new" expression for the string.
    vector<ExprPtr> args;
    args.push_back(strConst);
    args.push_back(builder.createIntConst(*this, value.size(),
                                          globalData->uintType.get()
                                          )
                   );
    FuncDefPtr newFunc =
        globalData->staticStringType->context->lookUp(*this, "oper new", args);
    FuncCallPtr funcCall = builder.createFuncCall(newFunc.get());
    funcCall->args = args;
    return funcCall;    
}

CleanupFramePtr Context::createCleanupFrame() {
    CleanupFramePtr frame = builder.createCleanupFrame(*this);
    frame->parent = cleanupFrame;
    cleanupFrame = frame;
    return frame;
}

void Context::closeCleanupFrame() {
    CleanupFramePtr frame = cleanupFrame;
    cleanupFrame = frame->parent;
    frame->close();
}

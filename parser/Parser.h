// Copyright 2009 Google Inc.

#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <spug/Exception.h>

#include "Toker.h"

namespace model {
   SPUG_RCPTR(ArgDef);
   SPUG_RCPTR(Context);
   SPUG_RCPTR(Expr);
   SPUG_RCPTR(Initializers);
   SPUG_RCPTR(TypeDef);
   SPUG_RCPTR(VarDef);
};

namespace parser {

class Parser {

   private:

      Toker &toker;
      
      // the module context, and the current context.
      model::ContextPtr moduleCtx, context;
      
      /**
       * This class essentially lets us manage the context stack with the 
       * program's stack.  We push the context by creating an instance, and 
       * pop it by calling restore() or falling through to the destructor.
       */
      friend class ContextStackFrame;
      class ContextStackFrame {
         private:
            bool restored;
            Parser &parser;
            model::ContextPtr context;

         public:
            ContextStackFrame(Parser &parser,
                              model::Context *context
                              ) :
               restored(false),
               parser(parser),
               context(parser.context) {
               
               parser.context = context;
            }
            
            ~ContextStackFrame() {
               if (!restored)
                  restore();
            }
            
            void restore() {
               assert(!restored);
               parser.context = context;
               restored = true;
            }
            
            model::Context &parent() {
               assert(!restored);
               return *context;
            }
      };
      
      typedef std::map<std::string, unsigned> OpPrecMap;
      OpPrecMap opPrecMap;
      
      /**
       * Add a new definition to the current context or nearest definition 
       * context.
       */
      void addDef(model::VarDef *context);
      
      /**
       * Returns the precedence of the specified operator.
       */
      unsigned getPrecedence(const std::string &op);

      /** Special kind of error function used for unexpected tokens. */
      void unexpected(const Token &tok, const char *userMsg = 0);

      /** 
       * Get the next token and make sure it is of type 'type' otherwise 
       * unexpected(tok, error);
       */
      void expectToken(Token::Type type, const char *error);

      /**
       * Parse a single statement.
       * @param defsAllowed true if a definition may be provided instead of a 
       *    statement.  Statements in block contexts may be definitions, 
       *    simple statements in conditionals must not be.
       * @returns the context that this statement terminates to.  If non-null, 
       *    the statement is terminal in all contexts from the current context
       *    to the returned context (non-inclusive).
       */
      model::ContextPtr parseStatement(bool defsAllowed);

      /**
       * Parse a block - a sequence of statements in the same execution
       * context.  A block is "nested" if it is inside the implicit file scope
       * block and wrapped in curly brackets.
       * @returns the context that this statement terminates to.  See 
       *    parseStatement().
       */
      model::ContextPtr parseBlock(bool nested);

      /** Create a reference to the "this" variable, error if there is none. */
      model::ExprPtr makeThisRef(const Token &ident,
                                 const std::string &memberName
                                 );

      /** 
       * Create a variable reference expression, complete with an implicit
       * "this" if necessary.
       */
      model::ExprPtr createVarRef(model::Expr *receiver, const Token &ident);

      /**
       * Parse the rest of an explicit "oper" name.
       */
      std::string parseOperSpec();

      /**
       * Parse the kinds of things that can come after an identifier.
       *
       * @param container the aggregate that the identifier is scoped to, as 
       *   in "container.ident"  This can be null, in which case the 
       *   identifier is scoped to the local context.
       * @param ident The identifier's token.
       */
      model::ExprPtr parsePostIdent(model::Expr *container,
                                    const Token &ident
                                    );

      /**
       * Parse an interpolated string.
       * 
       * @param expr the receiver of the interpolated string operation.
       */
      model::ExprPtr parseIString(model::Expr *expr);

      /**
       * If the expression is a VarRef referencing a TypeDef, return the 
       * TypeDef.  Otherwise returns null.
       */
      static model::TypeDef *convertTypeRef(model::Expr *expr);

      /**
       * Parse an expression.
       * 
       * @param precedence The function will not parse an operator of lower 
       *    precedence than this parameter.  So if we're parsing the right 
       *    side of 'x * y', and the precedence of '*' is 10, and we encounter 
       *    the sequence 'a + b' ('+' has precedence of 8), we'll stop parsing 
       *    after the 'a'.
       */
      model::ExprPtr parseExpression(unsigned precedence = 0);
      void parseMethodArgs(std::vector<model::ExprPtr> &args, 
                           Token::Type terminator = Token::rparen
                           );
      
      /** 
       * Parse the "specializer" after a generic type name. 
       * @param tok the left bracket token of the generic specifier.
       */
      model::TypeDef *parseSpecializer(const Token &tok, 
                                       model::TypeDef *typeDef
                                       );
      
      
      model::ExprPtr parseConstructor(const Token &tok, model::TypeDef *type,
                                      Token::Type terminator
                                      );
      
      model::TypeDefPtr parseTypeSpec(const char *errorMsg = 
                                       " is not a type."
                                      );
      void parseModuleName(std::vector<std::string> &moduleName);
      void parseArgDefs(std::vector<model::ArgDefPtr> &args);

      /**
       * Parse the initializer list after an oper init.
       */
      void parseInitializers(model::Initializers *inits, model::Expr *receiver);

      enum FuncFlags {
         normal,           // normal function
         hasMemberInits,   // function with member/base class initializers
                           // ("oper new")
         hasMemberDels     // function with member/base class destructors
                           // ("oper del")
      };

      /**
       * Parse a function definition.
       * @param returnType function return type.
       * @param nameTok the last token parsed in the function name.
       * @param name the full (but unqualified) function name.
       * @param funcFlags flags defining special processing rules for the 
       *    function (whether initializers or destructors need to be parsed 
       *    and emitted).
       * @param expectedArgCount if > -1, this is the expected number of arguments. 
       * @returns the number of actual arguments.
       */
      int parseFuncDef(model::TypeDef *returnType, const Token &nameTok,
                       const std::string &name,
                       FuncFlags funcFlags,
                       int expectedArgCount
                       );

      /**
       * Parse a definition. Returns false if there was no definition. 
       * @param type the parsed type.
       */
      bool parseDef(model::TypeDef *type);
      
      // statements

      /*      
       * @returns the context that this statement terminates to.  See 
       *    parseStatement().
       */
      model::ContextPtr parseIfClause();

      /*      
       * @returns the context that this statement terminates to.  See 
       *    parseStatement().
       */
      model::ContextPtr parseIfStmt();

      void parseWhileStmt();
      void parseReturnStmt();
      void parseImportStmt();
      
      /**
       * Parse a function definition after an "oper" keyword.
       */
      void parsePostOper(model::TypeDef *returnType);
      
      model::TypeDefPtr parseClassDef();
      
      // error checking functions
      model::VarDefPtr checkForExistingDef(const Token &tok,
                                           const std::string &name,
                                           bool overloadOk = false);

   public:
      Parser(Toker &toker, model::Context *context);

      void parse();

      void parseClassBody();

      /** 
       * throws a ParseError, properly formatted with the location and
       * message text.
       */
      static void error(const Token &tok, const std::string &msg);
      
      /** Writes a warning message to standard error. */
      static void warn(const Token &tok, const std::string & msg);
};

} // namespace parser

#endif

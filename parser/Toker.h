// Copyright 2003 Michael A. Muller
// Portions Copyright 2009 Google Inc.

#ifndef TOKER_H
#define TOKER_H

#include <assert.h>
#include <list>
#include <string>
#include "Token.h"
#include "LocationMap.h"

namespace parser {

/** The tokenizer. */
class Toker {
   private:

      // the "put-back" list - where tokens are stored after they've been put
      // back
      std::list<Token> tokens;

      // source stream
      std::istream &src;
      
      // tracks the location
      LocationMap locationMap;

      // "fixes identifiers" by converting them to keywords if appropriate - 
      // if the identifier in 'raw' is a keyword, returns a keyword token, 
      // otherwise just returns the identifier token.
      Token fixIdent(const std::string &raw, const Location &loc);

      // reads the next token directly from the source stream
      Token readToken();

      // info for tracking the state of the tokenizer.
      enum { 
         st_none, 
         st_interpNone,
         st_ident, 
         st_slash,
         st_minus,
         st_plus,
         st_digram,
         st_comment,
         st_ccomment,
         st_ccomment2,
         st_string, 
         st_strEscapeChar,
         st_istrEscapeChar,
         st_strOctal,
         st_istrOctal,
         st_strHex,
         st_istrHex,
         st_number,
         st_intdot,
         st_period,
         st_zero,
         st_float,
         st_exponent,
         st_exponent2,
         st_exponent3,
         st_amp,
         st_istr,
         st_pipe,
         st_ltgt,
         st_postaug,
         st_hex,
         st_octal,
         st_binary,
         st_strint,
         st_rawStr,
         st_rawStrBody,
         st_rawStrEscape,
         st_strEscapedIndentedNewline,
         st_istrEscapedIndentedNewline,
         st_indentStr
      } state;
      
      // the putback queue
      enum { putbackSize = 2 };
      char putbackBuf[putbackSize];
      int putbackIndex;
      
      // stuff for dealing with indentation.
      
      // set to true if we are parsing an indented string
      bool indentedString;
      
      // indentLevel and minIndentLevel are only used for 
      // "escape-newline-whitespace" sequences to keep track of the smallest 
      // of them.
      int indentLevel, minIndentLevel;
      
      // size of a tab.
      enum { tabWidth = 8 };
      
      // get the next character from the stream.
      bool getChar(char &ch);

      // put back the character      
      void ungetChar(char ch);

      // initialize all of the indentation state variables for a string, 
      // initialize for an indented string if indented is true.      
      void initIndent(bool indented);
      
      // update minIndentLevel with the smallest indent level in the string.  
      // This ignores lines consisting entirely of whitespace and newlines at 
      // the end of the string
      void evaluateIndentation(const std::string &val);
      
      // reformat all indentation in the string, truncating all whitespace
      // prefixes by minIndentLevel.
      void fixIndentation(std::string &val);
      
      // reindent the buffer based on the accumulated indentation state.
      void reindent(std::string &val) {
         evaluateIndentation(val);
         fixIndentation(val);
      }

   public:

      /** constructs a tokenizer from the source stream */
      Toker(std::istream &src, const char *sourceName, int lineNumber = 1);

      /**
       * Returns the next token in the stream.
       */
      Token getToken();

      /**
       * Puts the token back onto the stream.  A subsequent getNext() will
       * return the token.
       */
      void putBack(const Token &token) {
	 tokens.push_back(token);
      }
      
      /**
       * Tells the toker to continue scanning an interpolating string that was 
       * interrupted by a $ sequence.
       */
      void continueIString() {
         assert(state == st_none && "continueIString in invalid state");
         state = st_istr;
      }
      
      /** Returns the tokenizer's location map. */
      LocationMap &getLocationMap() { return locationMap; }

};

} // namespace parser

#endif

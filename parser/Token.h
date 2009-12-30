
#ifndef TOKEN_H
#define TOKEN_H

#include "Location.h"

namespace parser {

/** Basic representation of a token. */
class Token {
   public:

      // the token types
      typedef enum { classKw, elseKw, ifKw, importKw, isKw, nullKw, returnKw, 
                     whileKw, assign, asterisk, bang, colon, comma, define, 
                     dot, end, eq, ge, gt, ident, integer, lcurly, le, lparen, 
                     lt, minus, ne, oper, percent, plus, rcurly, rparen, semi,
                     slash, string
		    } Type;

   private:

      // the token's state
      Type type;
      std::string data;

      // source location for the token
      Location loc;

   public:

      Token();

      Token(Type type, const std::string &data, const Location &loc);

      /** returns the token type */
      Type getType() const { return type; }

      /** returns the token raw data */
      const std::string &getData() const { return data; }

      /** Returns the source location for the token */
      const Location &getLocation() const { return loc; }

      /** dump a representation of the token to a stream */
      friend std::ostream &operator <<(std::ostream &out, const Token &tok) {
	 out << tok.loc << ":\"" << tok.data;
      }

      /** Methods to check the token type */
      /** @{ */

      bool isIf() const { return type == ifKw; }
      bool isElse() const { return type == elseKw; }
      bool isWhile() const { return type == whileKw; }
      bool isReturn() const { return type == returnKw; }
      bool isClass() const { return type == classKw; }
      bool isNull() const { return type == nullKw; }
      bool isIdent() const { return type == ident; }
      bool isString() const { return type == string; }
      bool isSemi() const { return type == semi; }
      bool isComma() const { return type == comma; }
      bool isColon() const { return type == colon; }
      bool isDefine() const { return type == define; }
      bool isDot() const { return type == dot; }
      bool isAssign() const { return type == assign; }
      bool isLParen() const { return type == lparen; }
      bool isRParen() const { return type == rparen; }
      bool isLCurly() const { return type == lcurly; }
      bool isRCurly() const { return type == rcurly; }
      bool isInteger() const { return type == integer; }
      bool isPlus() const { return type == plus; }
      bool isMinus() const { return type == minus; }
      bool isAsterisk() const { return type == asterisk; }
      bool isSlash() const { return type == slash; }
      bool isPercent() const { return type == percent; }
      bool isNot() const { return type == bang; }
      bool isGT() const { return type == gt; }
      bool isLT() const { return type == lt; }
      bool isEQ() const { return type == eq; }
      bool isNE() const { return type == ne; }
      bool isGE() const { return type == ge; }
      bool isLE() const { return type == ne; }
      bool isEnd() const { return type == end; }
      
      bool isBinOp() const {
         switch (type) {
            case plus:
            case minus:
            case asterisk:
            case slash:
            case percent:
            case eq:
            case ne:
            case lt:
            case gt:
            case le:
            case ge:
            case isKw:
               return true;
            default:
               return false;
         }
      }

      /** @} */

};

} // namespace parser

#endif

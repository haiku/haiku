//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file sniffer/Parser.h
	MIME sniffer rule parser declarations
*/
#ifndef _sk_sniffer_parser_h_
#define _sk_sniffer_parser_h_

#include <SupportDefs.h>
#include <sniffer/Err.h>
#include <sniffer/Range.h>
#include <sniffer/Rule.h>
#include <List.h>
#include <string>
#include <vector>

class BString;

//! MIME Sniffer related classes
namespace Sniffer {

class Rule;
class Expr;
class RPattern;
class Pattern;

//------------------------------------------------------------------------------
// The mighty parsing function ;-)
//------------------------------------------------------------------------------

status_t parse(const char *rule, Rule *result, BString *parseError = NULL);

//------------------------------------------------------------------------------
// Classes used internally by the parser
//------------------------------------------------------------------------------

//! Manages a stream of characters
/*! CharStream is used by the scanner portion of the parser, which is implemented
	in TokenStream::SetTo().
*/
class CharStream {
public:
	CharStream(const std::string &string);
	CharStream();
	~CharStream();
	
	status_t SetTo(const std::string &string);
	void Unset();
	status_t InitCheck() const;
	bool IsEmpty() const;
	ssize_t Pos() const;
	const std::string& String() const;
	
	char Get();
	void Unget();

private:
	std::string fString;
	ssize_t fPos;
//	ssize_t fLen;
	status_t fCStatus;
	
	CharStream(const CharStream &ref);
	CharStream& operator=(const CharStream &ref);
};

//! Types of tokens
typedef enum TokenType {
	EmptyToken,
	LeftParen,
	RightParen,
	LeftBracket,
	RightBracket,
	Colon,
	Divider,
	Ampersand,
	CharacterString,
	Integer,
	FloatingPoint
};

/*! \brief Returns a NULL-terminated string contating the
		   name of the given token type
*/
const char* tokenTypeToString(TokenType type);

//! Base token class returned by TokenStream
/*! Each token represents a single chunk of relevant information
    in a given rule. For example, the floating point number "1.2e-35",
    originally represented as a 7-character string, is added to the
    token stream as a single FloatToken object.
*/
class Token {
public:
	Token(TokenType type = EmptyToken, const ssize_t pos = -1);
	TokenType Type() const;
	virtual const std::string& String() const;
	virtual int32 Int() const;
	virtual double Float() const;
	ssize_t Pos() const;
	bool operator==(Token &ref) const;
protected:
	TokenType fType;
	ssize_t fPos;
};

//! String token class
/*! Single-quoted strings, double-quoted strings, unquoted strings, and
	hex literals are all converted to StringToken objects by the scanner
	and from then on treated uniformly.
*/
class StringToken : public Token {
public:
	StringToken(const std::string &str, const ssize_t pos);
	virtual const std::string& String() const;
protected:
	std::string fString;
};

//! Integer token class
/*! Signed or unsigned integer literals are coverted to IntToken objects,
    which may then be treated as either ints or floats (since a priority
    of "1" would be valid, but scanned as an int instead of a float).
*/
class IntToken : public Token {
public:
	IntToken(const int32 value, const ssize_t pos);
	virtual int32 Int() const;
	virtual double Float() const;
protected:
	int32 fValue;
};

//! Floating point token class
/*! Signed or unsigned, extended or non-extended notation floating point
    numbers are converted to FloatToken objects by the scanner.
*/
class FloatToken : public Token {
public:
	FloatToken(const double value, const ssize_t pos);
	virtual double Float() const;
protected:
	double fValue;
};

//! Manages a stream of Token objects
/*! Provides Get() and Unget() operations, some handy shortcut operations (Read()
    and CondRead()), and handles memory management with respect to all the
    Token objects in the stream (i.e. never delete a Token object returned by Get()).
    
    Also, the scanner portion of the parser is implemented in the TokenStream's
    SetTo() function.
*/
class TokenStream {
public:
	TokenStream(const std::string &string);
	TokenStream();
	~TokenStream();
	
	status_t SetTo(const std::string &string);
	void Unset();
	status_t InitCheck() const;
	
	const Token* Get();
	void Unget();
	
	void Read(TokenType type);
	bool CondRead(TokenType type);
	
	ssize_t Pos() const;
	ssize_t EndPos() const;
	
	bool IsEmpty() const;
	
private:
	void AddToken(TokenType type, ssize_t pos);
	void AddString(const std::string &str, ssize_t pos);
	void AddInt(const char *str, ssize_t pos);
	void AddFloat(const char *str, ssize_t pos);

	std::vector<Token*> fTokenList;
	int fPos;
	int fStrLen;
	status_t fCStatus;


	TokenStream(const TokenStream &ref);
	TokenStream& operator=(const TokenStream &ref);
};

//! Handles parsing a sniffer rule, yielding either a parsed rule or a descriptive error message.
class Parser {
public:
	Parser();
	~Parser();
	status_t Parse(const char *rule, Rule *result, BString *parseError = NULL);	
private:
	std::string ErrorMessage(Err *err, const char *rule);

	// Things that get done a lot :-)
	void ThrowEndOfStreamError();
	inline void ThrowOutOfMemError(ssize_t pos);
	void ThrowUnexpectedTokenError(TokenType expected, const Token *found);
	void ThrowUnexpectedTokenError(TokenType expected1, TokenType expected2, const Token *found);

	// Parsing functions
	void ParseRule(Rule *result);
	double ParsePriority();
	std::vector<Expr*>* ParseExprList();
	Expr* ParseExpr();
	Range ParseRange();
	Expr* ParsePatternList(Range range);
	Expr* ParseRPatternList();
	RPattern* ParseRPattern();
	Pattern* ParsePattern();
	
	TokenStream stream;
	
	Err *fOutOfMemErr;
};

}	// namespace Sniffer


#endif	// _sk_sniffer_parser_h_


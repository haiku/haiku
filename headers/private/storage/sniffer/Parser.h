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

class CharStream {
public:
	CharStream(const char *string = NULL);
	~CharStream();
	
	status_t SetTo(const char *string);
	void Unset();
	status_t InitCheck() const;
	bool IsEmpty() const;
	ssize_t Pos() const;
	const char *String() const;
	
	char Get();
	void Unget();

private:
	char *fString;
	ssize_t fPos;
	ssize_t fLen;
	status_t fCStatus;
	
	CharStream(const CharStream &ref);
	CharStream& operator=(const CharStream &ref);
};

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

const char* tokenTypeToString(TokenType type);

class Token {
public:
	Token(TokenType type = EmptyToken, const ssize_t pos = -1);
	virtual ~Token();
	TokenType Type() const;
	virtual const char* String() const;
	virtual int32 Int() const;
	virtual double Float() const;
	ssize_t Pos() const;
	bool operator==(Token &ref) const;
protected:
	TokenType fType;
	ssize_t fPos;
};

class StringToken : public Token {
public:
	StringToken(const char *string, const ssize_t pos);
	virtual ~StringToken();
	virtual const char* String() const;
protected:
	char *fString;
};

class IntToken : public Token {
public:
	IntToken(const int32 value, const ssize_t pos);
	virtual int32 Int() const;
	virtual double Float() const;
protected:
	int32 fValue;
};

class FloatToken : public Token {
public:
	FloatToken(const double value, const ssize_t pos);
	virtual double Float() const;
protected:
	double fValue;
};

class TokenStream {
public:
	TokenStream(const char *string = NULL);
	~TokenStream();
	
	status_t SetTo(const char *string);
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
	void AddString(const char *str, ssize_t pos);
	void AddInt(const char *str, ssize_t pos);
	void AddFloat(const char *str, ssize_t pos);

	std::vector<Token*> fTokenList;
	int fPos;
	int fStrLen;
	status_t fCStatus;


	TokenStream(const TokenStream &ref);
	TokenStream& operator=(const TokenStream &ref);
};

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


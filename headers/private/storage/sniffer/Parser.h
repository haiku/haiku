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
#include <sniffer/Range.h>
#include <sniffer/Rule.h>
#include <List.h>
#include <string>
#include <vector>


class BString;

namespace Sniffer {

class Rule;
class Expr;
class Range;
class RPattern;
class Pattern;
typedef std::vector<Expr*> ExprList;

//------------------------------------------------------------------------------
// The mighty parsing function ;-)
//------------------------------------------------------------------------------

status_t parse(const char *rule, Rule *result, BString *parseError = NULL);

//------------------------------------------------------------------------------
// Classes used internally by the parser
//------------------------------------------------------------------------------

class Err {
public:
	Err(const char *msg, const ssize_t pos);
	Err(const std::string &msg, const ssize_t pos);
	Err(const Err &ref);
	Err& operator=(const Err &ref);
	const char* Msg() const;
	ssize_t Pos() const;
	
private:
	void SetMsg(const char *msg);
	char *fMsg;
	ssize_t fPos;
};

class CharStream {
public:
	CharStream(const char *string = NULL);
	~CharStream();
	
	status_t SetTo(const char *string);
	void Unset();
	status_t InitCheck() const;
	bool IsEmpty() const;
	ssize_t Pos() const;
	
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
	bool operator==(Token &ref);
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
	
	Token* Get();
	void Unget(Token *token);
	
	bool IsEmpty();
	
private:
	void AddToken(TokenType type, ssize_t pos);
	void AddString(const char *str, ssize_t pos);
	void AddInt(const char *str, ssize_t pos);
	void AddFloat(const char *str, ssize_t pos);

	BList fTokenList;
	status_t fCStatus;

	TokenStream(const TokenStream &ref);
	TokenStream& operator=(const TokenStream &ref);
};

class Parser {
public:
	Parser();
	status_t Parse(const char *rule, Rule *result, BString *parseError = NULL);	
private:
	std::string ErrorMessage(Err *err, const char *rule);

	// Parsing functions
	void ParseRule(Rule *result);
	double ParsePriority();
	ExprList* ParseExprList();
	Expr* ParseExpr();
	Range ParseRange();
	Expr* ParsePatternList();
	Expr* ParseRPatternList();
	RPattern* ParseRPattern();
	Pattern* ParsePattern();
	
	TokenStream stream;
};

}	// namespace Sniffer


#endif	// _sk_sniffer_parser_h_


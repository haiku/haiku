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
#include <List.h>
#include <string>


class BString;

namespace Sniffer {
class Rule;

//------------------------------------------------------------------------------
// The mighty parsing function ;-)
//------------------------------------------------------------------------------

status_t parse(const char *rule, Rule *result, BString *parseError = NULL);

//------------------------------------------------------------------------------
// Classes used internally by the parser
//------------------------------------------------------------------------------

class Err {
public:
	Err(const char *msg);
	Err(const std::string &msg);
	Err(const Err &ref);
	Err& operator=(const Err &ref);
	const char* Msg() const;
	
private:
	void SetMsg(const char *msg);
	char *fMsg;
};

class CharStream {
public:
	CharStream(const char *string = NULL);
	~CharStream();
	
	status_t SetTo(const char *string);
	void Unset();
	status_t InitCheck() const;
	bool IsEmpty() const;
	
	char Get();
	void Unget();

private:
	char *fString;
	size_t fPos;
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
	Token(TokenType type = EmptyToken);
	virtual ~Token();
	TokenType Type() const;
	virtual const char* String() const;
	virtual int32 Int() const;
	virtual double Float() const;
	bool operator==(Token &ref);
protected:
	TokenType fType;
};

class StringToken : public Token {
public:
	StringToken(const char *string);
	virtual ~StringToken();
	virtual const char* String() const;
protected:
	char *fString;
};

class IntToken : public Token {
public:
	IntToken(const int32 value);
	virtual int32 Int() const;
	virtual double Float() const;
protected:
	int32 fValue;
};

class FloatToken : public Token {
public:
	FloatToken(const double value);
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
	void AddToken(TokenType type);
	void AddString(const char *str);
	void AddInt(const char *str);
	void AddFloat(const char *str);

	BList fTokenList;
	status_t fCStatus;

	TokenStream(const TokenStream &ref);
	TokenStream& operator=(const TokenStream &ref);
};

} // namespace Sniffer


#endif	// _sk_sniffer_parser_h_


//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//----------------------------------------------------------------------
/*!
	\file sniffer/Parser.cpp
	MIME sniffer rule parser implementation
*/

//#include <sniffer/Expr.h>
#include <sniffer/Parser.h>
//#include <sniffer/Pattern.h>
//#include <sniffer/PatternList.h>
//#include <sniffer/Range.h>
//#include <sniffer/RPatternList.h>
#include <sniffer/Rule.h>

#include <string>
#include <List.h>
#include <new.h>
#include <stdio.h>
#include <string.h>
#include <String.h>	// BString

using namespace Sniffer;

namespace Sniffer {

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

} // namespace Sniffer

class CharStream {
public:
	CharStream(const char *string = NULL);
	~CharStream();
	
	status_t SetTo(const char *string);
	void Unset();
	status_t InitCheck() const;
	bool IsEmpty();
	
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
	LeftParen,
	RightParen,
	LeftBracket,
	RightBracket,
	Colon,
	Divider,
	Ampersand,
	CharacterString
};

class Token {
public:
	Token(TokenType type);
	virtual ~Token();
	TokenType Type() const;
	virtual const char* String() const;
protected:
	TokenType fType;
};

class PString : public Token {
public:
	PString(const char *string);
	virtual ~PString();
	virtual const char* String() const;
protected:
	char *fString;
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
	
private:
	void AddToken(TokenType type);
	void AddString(const char *str);

	BList fTokenList;
	status_t fCStatus;

	TokenStream(const TokenStream &ref);
	TokenStream& operator=(const TokenStream &ref);
};

// Our global token stream object
TokenStream stream;

// Private parsing functions
/*
float parsePriority();
BList* parseExprList();
Expr* parseExpr();
Range parseRange();
Expr* parsePatternList();
Expr* parseRPatternList();
Pattern* parsePattern();
*/

// Miscellaneous helper functions
char escapeChar(char ch);
char hexToChar(char hi, char low);
char hexToChar(char hex);
char octalToChar(char octal);
char octalToChar(char hi, char low);
char octalToChar(char hi, char mid, char low);
bool isHexChar(char ch);
bool isWhiteSpace(char ch);
bool isOctalChar(char ch);

status_t
Sniffer::parse(const char *rule, Rule *result, BString *parseError = NULL) {
	try {
		if (!rule)
			throw Err("Sniffer pattern error: NULL pattern");
		if (!result)
			return B_BAD_VALUE;		
		if (stream.SetTo(rule) != B_OK)
			throw Err("Sniffer parser error: Unable to intialize token stream");
			
		float priority;
		BList* exprList;
		
//		priority = parsePriority();
		
	} catch (Err &err) {
		if (parseError) 
			parseError->SetTo((err.Msg() ? err.Msg() : "Sniffer parser rule: Unexpected error with no supplied error message"));
		return B_BAD_MIME_SNIFFER_RULE;
	}
}

//------------------------------------------------------------------------------
// Sniffer::Err
//------------------------------------------------------------------------------

Err::Err(const char *msg)
	: fMsg(NULL)
{
	SetMsg(msg);
}

Err::Err(const std::string &msg) {
	SetMsg(msg.c_str());
}

Err::Err(const Err &ref) {
	*this = ref;
}

Err&
Err::operator=(const Err &ref) {
	SetMsg(ref.Msg());
	return *this;
}

const char*
Err::Msg() const {
	return fMsg;
}

void
Err::SetMsg(const char *msg) {
	delete fMsg;
	if (msg == NULL)
		fMsg = NULL;
	else {
		fMsg = new(nothrow) char[strlen(msg)+1];
		if (fMsg)
			strcpy(fMsg, msg);
	}
}


//------------------------------------------------------------------------------
// CharStream
//------------------------------------------------------------------------------

CharStream::CharStream(const char *string)
	: fString(NULL)
	, fPos(0)
	, fLen(-1)
	, fCStatus(B_NO_INIT)		
{
	SetTo(string);
}

CharStream::~CharStream() {
	Unset();
}

status_t
CharStream::SetTo(const char *string) {
	Unset();
	if (string) {
		fString = new(nothrow) char[strlen(string)+1];
		if (!fString)
			fCStatus = B_NO_MEMORY;
		else {
			strcpy(fString, string);
			fLen = strlen(fString);
			fCStatus = B_OK;
		}
	}
	return fCStatus;
}

void
CharStream::Unset() {
	delete fString;
	fCStatus = B_NO_INIT;
	fPos = 0;
	fLen = -1;
}
	
status_t
CharStream::InitCheck() const {
	return fCStatus;
}

bool
CharStream::IsEmpty() {
	return fPos >= fLen;
}

char
CharStream::Get() {
	if (fCStatus != B_OK)
		throw Err("Sniffer parser error: CharStream::Get() called on uninitialized CharStream object");
	if (!IsEmpty()) 
		return fString[fPos++];
	else
		throw Err("Sniffer pattern error: unterminated rule");
}

void
CharStream::Unget() {
	if (fCStatus != B_OK)
		throw Err("Sniffer parser error: CharStream::Unget() called on uninitialized CharStream object");
	if (fPos > 0) 
		fPos--;
	else
		throw Err("Sniffer parser error: CharStream::Unget() called at beginning of character stream");
}

//------------------------------------------------------------------------------
// Token
//------------------------------------------------------------------------------

Token::Token(TokenType type)
	: fType(type)
{
}

Token::~Token() {
}

TokenType
Token::Type() const {
	return fType;
}

const char*
Token::String() const {
	throw Err("Sniffer scanner error: Token::String() called on non-string token");
}

//------------------------------------------------------------------------------
// PString
//------------------------------------------------------------------------------

PString::PString(const char *string)
	: Token(CharacterString)
	, fString(NULL)
{
	if (string) {
		fString = new(nothrow) char[strlen(string)+1];
		if (fString) 
			strcpy(fString, string);
	}
}

PString::~PString() {
	delete fString;
}

const char*
PString::String() const {
	return fString;
}

//------------------------------------------------------------------------------
// TokenStream
//------------------------------------------------------------------------------

TokenStream::TokenStream(const char *string = NULL)
	: fCStatus(B_NO_INIT)
{
	SetTo(string);
}

TokenStream::~TokenStream() {
	Unset();
}
	
status_t
TokenStream::SetTo(const char *string) {
	Unset();
	if (string) {
		CharStream stream(string);
		if (stream.InitCheck() != B_OK)
			throw Err("Sniffer scanner error: Unable to intialize character stream");
		
		typedef enum TokenStreamScannerState {
			tsssStart,
			tsssOneSingle,
			tsssSingleEscape,
			tsssOneDouble,
			tsssDoubleEscape,
			tsssOneZero,
			tsssZeroX,
			tsssOneHex,
			tsssTwoHex,
			tsssOneEscape,
			tsssOneOctal,
			tsssTwoOctal,
			tsssUnquoted,
			tsssUnquotedEscape,
		};
		
		TokenStreamScannerState state = tsssStart;		

		std::string charStr;	// Used to build up character strings
		char lastChar;			// For two char lookahead
		char lastLastChar;		// For three char lookahead
		while (!stream.IsEmpty()) {
			char ch = stream.Get();
			switch (state) {				
				case tsssStart:
					switch (ch) {
						case '\t':
						case '\n':
						case ' ':
							// Whitespace, so ignore it.
							break;

						case '"':
							charStr = "";
							state = tsssOneDouble;
							break;
							
						case '\'':
							charStr = "";
							state = tsssOneSingle;
							break;
							
						case '0':
							state = tsssOneZero;
							break;
													
						case '&':	AddToken(Ampersand);		break;
						case '(':	AddToken(LeftParen);		break;
						case ')':	AddToken(RightParen);		break;
						case ':':	AddToken(Colon);			break;
						case '[':	AddToken(LeftBracket);		break;
						
						case '\\':
							state = tsssOneEscape;
							break;							
						
						case ']':	AddToken(RightBracket);		break;
						case '|':	AddToken(Divider);			break;
						
						default:
							throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					}			
					break;
					
				case tsssOneSingle:
					switch (ch) {
						case '\\':
							state = tsssSingleEscape;
							break;							
						case '\'':
							AddString(charStr.c_str());
							state = tsssStart;
							break;				
						default:							
							charStr += ch;
							break;
					}
					break;
					
				case tsssSingleEscape:
					charStr += escapeChar(ch);
					state = tsssOneSingle;					
					break;
					
				case tsssOneDouble:
					switch (ch) {
						case '\\':
							state = tsssDoubleEscape;
							break;							
						case '"':
							AddString(charStr.c_str());
							state = tsssStart;
							break;				
						default:							
							charStr += ch;
							break;
					}
					break;

				case tsssDoubleEscape:
					charStr += escapeChar(ch);
					state = tsssOneDouble;					
					break;
					
				case tsssOneZero:
					if (ch == 'x')
						state = tsssZeroX;
					else
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssZeroX:
					if (isHexChar(ch)) {
						lastChar = ch;
						state = tsssOneHex;
					} else 
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssOneHex:
					if (isHexChar(ch)) {
						charStr += hexToChar(lastChar, ch);
						state = tsssTwoHex;
					} else 
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssTwoHex:
					if (isHexChar(ch)) {
						lastChar = ch;
						state = tsssOneHex;
					} else if (isWhiteSpace(ch)) {
						AddString(charStr.c_str());
						state = tsssStart;
					} else
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssOneEscape:
					if (isOctalChar(ch)) {
						lastChar = ch;
						state = tsssOneOctal;
					} else {
						charStr += escapeChar(ch);
						state = tsssUnquoted;
					}									
					break;
					
				case tsssOneOctal:
					if (isOctalChar(ch)) {
						lastLastChar = lastChar;
						lastChar = ch;
						state = tsssTwoOctal;
					} else {
						// First handle the octal
						charStr += octalToChar(lastChar);
						
						// Push the new char back on and let the tsssUnquoted
						// state handle it.
						stream.Unget();
						state = tsssUnquoted;
					}									
					break;

				case tsssTwoOctal:
					if (isOctalChar(ch)) {
						charStr += octalToChar(lastLastChar, lastChar, ch);
						state = tsssUnquoted;
					} else {
						// First handle the octal
						charStr += octalToChar(lastLastChar, lastChar);
						
						// Push the new char back on and let the tsssUnquoted
						// state handle it.
						stream.Unget();
						state = tsssUnquoted;
					}									
					break;

				case tsssUnquoted:
					if (ch == '\\')
						state = tsssUnquotedEscape;
					else if (isWhiteSpace(ch)) {
						AddString(charStr.c_str());
						state = tsssStart;
					} else if (ch == '\'' || ch == '"' || ch == '&') {
						throw Err(std::string("Sniffer scanner error: illegal unquoted character '") + ch + "'");
					} else {							
						charStr += ch;
					}
					break;
					
				case tsssUnquotedEscape:
					charStr += escapeChar(ch);
					state = tsssUnquoted;					
					break;
			
			}
		}
		if (state == tsssStart)	
			fCStatus = B_OK;
		else
			throw Err("Sniffer pattern error: unterminated rule");
	}
	return fCStatus;
}

void
TokenStream::Unset() {
	while (!fTokenList.IsEmpty()) 
		delete (Token*)fTokenList.RemoveItem((int32)0);
	fCStatus = B_NO_INIT;
}

status_t
TokenStream::InitCheck() const {
	return fCStatus;
}
	
Token*
TokenStream::Get() {
	
}

void
TokenStream::Unget(Token *token) {
}

void
TokenStream::AddToken(TokenType type) {
	Token *token = new Token(type);
	fTokenList.AddItem(token);
}

void
TokenStream::AddString(const char *str) {
	Token *token = new PString(str);
	fTokenList.AddItem(token);
}

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

char
escapeChar(char ch) {
	// Is there an easier way to do this? I'm not sure. :-)
	std::string format = std::string("\\") + ch;
	char str[3];	// Two should be enough but I'm paranoid :-)
	sprintf(str, format.c_str());
	return str[0];
}

// Converts 0x|hi|low| to a single char
char
hexToChar(char hi, char low) {
	return (hexToChar(hi) << 4)	| hexToChar(low);
}

// Converts 0x|ch| to a single char
char
hexToChar(char hex) {
	if ('0' <= hex && hex <= '9')
		return hex-'0';
	else if ('a' <= hex && hex <= 'f')
		return hex-'a'+10;
	else if ('A' <= hex && hex <= 'F')
		return hex-'a'+10;
	else
		throw Err(std::string("Sniffer parser error: invalid hex digit '") + hex + "' passed to hexToChar()");
}
		
char
octalToChar(char octal) {
	return octalToChar('0', '0', octal);
}

char
octalToChar(char hi, char low) {
	return octalToChar('0', hi, low);
}

char
octalToChar(char hi, char mid, char low) {
	if (isOctalChar(hi) && isOctalChar(mid) && isOctalChar(low)) {
		return ((hi-'0') << 6) | ((mid-'0') << 3) | (low-'0');
	} else
		throw Err(std::string("Sniffer parser error: invalid octal digit passed to hexToChar()"));		
}

bool
isHexChar(char ch) {
	return ('0' <= ch && ch <= '9')
	         || ('a' <= ch && ch <= 'f')
	           || ('A' <= ch && ch <= 'F');
}

bool
isWhiteSpace(char ch) {
	return ch == ' ' || ch == '\n' || ch == '\t';
}

bool
isOctalChar(char ch) {
	return ('0' <= ch && ch <= '7');
}



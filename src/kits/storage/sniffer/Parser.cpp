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

#include <new.h>
#include <stdio.h>
#include <stdlib.h>	// For atol(), atof()
#include <string.h>
#include <String.h>	// BString

using namespace Sniffer;

// Our global token stream object
TokenStream stream;

// Private parsing functions
/*
double parsePriority();
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
bool isDecimalChar(char ch);

status_t
Sniffer::parse(const char *rule, Rule *result, BString *parseError = NULL) {
	try {
		if (!rule)
			throw Err("Sniffer pattern error: NULL pattern");
		if (!result)
			return B_BAD_VALUE;		
		if (stream.SetTo(rule) != B_OK)
			throw Err("Sniffer parser error: Unable to intialize token stream");
			
		double priority;
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
CharStream::IsEmpty() const {
	return fPos >= fLen;
}

char
CharStream::Get() {
	if (fCStatus != B_OK)
		throw Err("Sniffer parser error: CharStream::Get() called on uninitialized CharStream object");
	if (fPos < fLen) 
		return fString[fPos++];
	else {
		fPos++;		// Increment fPos to keep Unget()s consistent
		return 0x3;	// Return End-Of-Text char
	}
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
//	if (type != EmptyToken)
//		cout << "New Token, fType == " << tokenTypeToString(fType) << endl;
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

int32
Token::Int() const {
	throw Err("Sniffer scanner error: Token::Int() called on non-integer token");
}

double
Token::Float() const {
	throw Err("Sniffer scanner error: Token::Float() called on non-float token");
}

bool
Token::operator==(Token &ref) {
	// Compare types, then data if necessary
	if (Type() == ref.Type()) {
		switch (Type()) {
			case CharacterString:
				return strcmp(String(), ref.String()) == 0;
			
			case Integer:
				return Int() == ref.Int();
				
			case FloatingPoint:
				return Float() == ref.Float();		
			
			default:
				return true;	
		}	
	} else
		return false;
}

//------------------------------------------------------------------------------
// StringToken
//------------------------------------------------------------------------------

StringToken::StringToken(const char *string)
	: Token(CharacterString)
	, fString(NULL)
{
	if (string) {
		fString = new(nothrow) char[strlen(string)+1];
		if (fString) 
			strcpy(fString, string);
	}
}

StringToken::~StringToken() {
	delete fString;
}

const char*
StringToken::String() const {
	return fString;
}

//------------------------------------------------------------------------------
// IntToken
//------------------------------------------------------------------------------

IntToken::IntToken(const int32 value)
	: Token(Integer)
	, fValue(value)
{
}

int32
IntToken::Int() const {
	return fValue;
}

double
IntToken::Float() const {
	return (double)fValue;
}

//------------------------------------------------------------------------------
// FloatToken
//------------------------------------------------------------------------------

FloatToken::FloatToken(const double value)
	: Token(FloatingPoint)
	, fValue(value)
{
}

double
FloatToken::Float() const {
	return fValue;
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
			tsssIntOrFloat,
			tsssFloat,
			tsssLonelyDecimalPoint,
			tsssLonelyMinusSign,
			tsssNegativeInt,
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
		bool keepLooping = true;
		while (keepLooping) {
			char ch = stream.Get();
			switch (state) {				
				case tsssStart:
					switch (ch) {
						case 0x3:	// End-Of-Text
							if (stream.IsEmpty())
								keepLooping = false;
							else
								throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
							break;							
					
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
							
						case '-':
							charStr = ch;
							state = tsssLonelyMinusSign;
							break;
							
						case '.':
							charStr = ch;
							state = tsssLonelyDecimalPoint;
							break;
							
						case '0':
							charStr = ch;
							state = tsssOneZero;
							break;
							
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							charStr = ch;
							state = tsssIntOrFloat;							
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
						case 0x3:
							if (stream.IsEmpty())
								throw Err(std::string("Sniffer scanner error: unterminated single-quoted string"));
							else
								charStr += ch;
							break;
						default:							
							charStr += ch;
							break;
					}
					break;
					
				case tsssSingleEscape:
					// Check for a true end-of-text marker
					if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: unterminated escape sequence in single-quoted string"));
					else {
						charStr += escapeChar(ch);
						state = tsssOneSingle;
					}
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
						case 0x3:
							if (stream.IsEmpty())
								throw Err(std::string("Sniffer scanner error: unterminated single-quoted string"));
							else
								charStr += ch;
							break;
						default:							
							charStr += ch;
							break;
					}
					break;

				case tsssDoubleEscape:
					// Check for a true end-of-text marker
					if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: unterminated escape sequence in single-quoted string"));
					else {
						charStr += escapeChar(ch);
						state = tsssOneDouble;
					}
					break;
					
				case tsssOneZero:
					if (ch == 'x') {
						charStr = "";	// Reinit, since we actually have a hex string
						state = tsssZeroX;
					} else if ('0' <= ch && ch <= '9') {
						charStr += ch;
						state = tsssIntOrFloat;
					} else if (ch == '.') {
						charStr += ch;
						state = tsssFloat;
					} else if (ch == 0x3 && stream.IsEmpty()) {
						// Terminate the number and then the loop
						AddInt(charStr.c_str());
						keepLooping = false;
					} else {
						// Terminate the number
						AddInt(charStr.c_str());
						
						// Push the last char back on and try again
						stream.Unget();
						state = tsssStart;
					}
					break;
					
				case tsssZeroX:
					if (isHexChar(ch)) {
						lastChar = ch;
						state = tsssOneHex;
					} else if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: incomplete hex code"));
					else 
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssOneHex:
					if (isHexChar(ch)) {
						charStr += hexToChar(lastChar, ch);
						state = tsssTwoHex;
					} else if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: incomplete hex code (the number of hex digits must be a multiple of two)"));
					else 
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssTwoHex:
					if (isHexChar(ch)) {
						lastChar = ch;
						state = tsssOneHex;
					} else if (isWhiteSpace(ch)) {
						AddString(charStr.c_str());
						state = tsssStart;
					} else if (ch == 0x3 && stream.IsEmpty()) {
						AddString(charStr.c_str());
						keepLooping = false;						
					} else
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssIntOrFloat:
					if (isDecimalChar(ch))
						charStr += ch;
					else if (ch == '.') {
						charStr += ch;
						state = tsssFloat;
					} else {
						// Terminate the number
						AddInt(charStr.c_str());
						
						// Push the last char back on and try again
						stream.Unget();
						state = tsssStart;						
					}
					break;
					
				case tsssFloat:
					if (isDecimalChar(ch))
						charStr += ch;
					else {
						// Terminate the number
						AddFloat(charStr.c_str());
						
						// Push the last char back on and try again
						stream.Unget();
						state = tsssStart;						
					}
					break;
					
				case tsssLonelyDecimalPoint:
					if (isDecimalChar(ch)) {
						charStr += ch;
						state = tsssFloat;
					} else if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: incomplete floating point number"));
					else
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;
					
				case tsssLonelyMinusSign:
					if (isDecimalChar(ch)) {
						charStr += ch;
						state = tsssNegativeInt;
					} else if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: incomplete negative integer"));
					else
						throw Err(std::string("Sniffer scanner error: unexpected character '") + ch + "'");
					break;

				case tsssNegativeInt:
					if (isDecimalChar(ch)) 
						charStr += ch;
					else if (ch == '.') 
						throw Err(std::string("Sniffer scanner error: negative floating point numbers are useless and thus illegal"));
					else {
						// Terminate the number
						AddInt(charStr.c_str());
						
						// Push the last char back on and try again
						stream.Unget();
						state = tsssStart;						
					}
					break;

				case tsssOneEscape:
					if (isOctalChar(ch)) {
						lastChar = ch;
						state = tsssOneOctal;
					} else {
						// Check for a true end-of-text marker
						if (ch == 0x3 && stream.IsEmpty())
							throw Err(std::string("Sniffer scanner error: unterminated escape sequence"));
						else {
							charStr += escapeChar(ch);
							state = tsssUnquoted;
						}
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
					} else if (ch == 0x3 && stream.IsEmpty()) {
						AddString(charStr.c_str());
						keepLooping = false;
					} else {							
						charStr += ch;
					}
					break;
					
				case tsssUnquotedEscape:
					// Check for a true end-of-text marker
					if (ch == 0x3 && stream.IsEmpty())
						throw Err(std::string("Sniffer scanner error: unterminated escape sequence in unquoted string"));
					else {
						charStr += escapeChar(ch);
						state = tsssUnquoted;
					}
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
	return (Token*)fTokenList.RemoveItem((int32)0);
}

void
TokenStream::Unget(Token *token) {
	fTokenList.AddItem(token, 0);
}

bool
TokenStream::IsEmpty() {
	return fCStatus != B_OK || fTokenList.IsEmpty();
}

void
TokenStream::AddToken(TokenType type) {
	Token *token = new Token(type);
	fTokenList.AddItem(token);
}

void
TokenStream::AddString(const char *str) {
	Token *token = new StringToken(str);
	fTokenList.AddItem(token);
}

void
TokenStream::AddInt(const char *str) {
	// Convert the string to an int
	int32 value = atol(str);	
	Token *token = new IntToken(value);
	fTokenList.AddItem(token);
}

void
TokenStream::AddFloat(const char *str) {
	// Convert the string to a float
	double value = atof(str);
	Token *token = new FloatToken(value);
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

bool
isDecimalChar(char ch) {
	return ('0' <= ch && ch <= '9');
}

const char*
Sniffer::tokenTypeToString(TokenType type) {
	switch (type) {
		case LeftParen:
			return "LeftParen";
			break;
		case RightParen:
			return "RightParen";
			break;
		case LeftBracket:
			return "LeftBracket";
			break;
		case RightBracket:
			return "RightBracket";
			break;
		case Colon:
			return "Colon";
			break;
		case Divider:
			return "Divider";
			break;
		case Ampersand:
			return "Ampersand";
			break;
		case CharacterString:
			return "CharacterString";
			break;
		case Integer:
			return "Integer";
			break;
		case FloatingPoint:
			return "FloatingPoint";
			break;
		default:
			return "UNKNOWN TOKEN TYPE";
			break;
	}
}


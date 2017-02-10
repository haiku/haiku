//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//----------------------------------------------------------------------
/*!
	\file sniffer/Parser.cpp
	MIME sniffer rule parser implementation
*/

#include <sniffer/Parser.h>
#include <sniffer/Pattern.h>
#include <sniffer/PatternList.h>
#include <sniffer/Range.h>
#include <sniffer/RPattern.h>
#include <sniffer/RPatternList.h>
#include <sniffer/Rule.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>	// For atol(), atof()
#include <string.h>
#include <String.h>	// BString

using namespace BPrivate::Storage::Sniffer;

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
bool isPunctuation(char ch);

//! Parses the given rule.
/*! The resulting parsed Rule structure is stored in \c rule, which
	must be pre-allocated. If parsing fails, a descriptive error message (meant
	to be viewed in a monospaced font) is placed in the pre-allocated \c BString
	pointed to by \c parseError (which may be \c NULL if you don't care about
	the error message).
	
	\param rule Pointer to a NULL-terminated string containing the sniffer
	            rule to be parsed
	\param result Pointer to a pre-allocated \c Rule object into which the result
	              of parsing is placed upon success.
	\param parseError Point to pre-allocated \c BString object into which
	                  a descriptive error message is stored upon failure.
	                  
	\return
	- B_OK: Success
	- B_BAD_MIME_SNIFFER_RULE: Failure
*/
status_t
BPrivate::Storage::Sniffer::parse(const char *rule, Rule *result, BString *parseError) {
	Parser parser;
	return parser.Parse(rule, result, parseError);
}

//------------------------------------------------------------------------------
// Token
//------------------------------------------------------------------------------

Token::Token(TokenType type, const ssize_t pos)
	: fType(type)
	, fPos(pos)
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

const std::string&
Token::String() const {
	throw new Err("Sniffer scanner error: Token::String() called on non-string token", fPos);
}

int32
Token::Int() const {
	throw new Err("Sniffer scanner error: Token::Int() called on non-integer token", fPos);
}

double
Token::Float() const {
	throw new Err("Sniffer scanner error: Token::Float() called on non-float token", fPos);
}

ssize_t
Token::Pos() const {
	return fPos;
}

bool
Token::operator==(Token &ref) const {
	// Compare types, then data if necessary
	if (Type() == ref.Type()) {
		switch (Type()) {
			case CharacterString:
//				printf(" str1 == '%s'\n", String());
//				printf(" str2 == '%s'\n", ref.String());
//				printf(" strcmp() == %d\n", strcmp(String(), ref.String()));
			{
				return String() == ref.String();				
				
/*				
				// strcmp() seems to choke on certain, non-normal ASCII chars
				// (i.e. chars outside the usual alphabets, but still valid
				// as far as ASCII is concerned), so we'll just compare the
				// strings by hand to be safe.
				const char *str1 = String();
				const char *str2 = ref.String();				
				int len1 = strlen(str1);
				int len2 = strlen(str2);
//				printf("len1 == %d\n", len1);
//				printf("len2 == %d\n", len2);
				if (len1 == len2) {
					for (int i = 0; i < len1; i++) {
//						printf("i == %d, str1[%d] == %x, str2[%d] == %x\n", i, i, str1[i], i, str2[i]);
						if (str1[i] != str2[i])
							return false;
					}
				}
				return true;
*/
			}
//				return strcmp(String(), ref.String()) == 0;
			
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

StringToken::StringToken(const std::string &str, const ssize_t pos)
	: Token(CharacterString, pos)
	, fString(str)
{
}

StringToken::~StringToken() {
}

const std::string&
StringToken::String() const {
	return fString;
}

//------------------------------------------------------------------------------
// IntToken
//------------------------------------------------------------------------------

IntToken::IntToken(const int32 value, const ssize_t pos)
	: Token(Integer, pos)
	, fValue(value)
{
}

IntToken::~IntToken() {
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

FloatToken::FloatToken(const double value, const ssize_t pos)
	: Token(FloatingPoint, pos)
	, fValue(value)
{
}

FloatToken::~FloatToken() {
}


double
FloatToken::Float() const {
	return fValue;
}

//------------------------------------------------------------------------------
// TokenStream
//------------------------------------------------------------------------------

TokenStream::TokenStream(const std::string &string)
	: fCStatus(B_NO_INIT)
	, fPos(-1)
	, fStrLen(-1)
{
	SetTo(string);
}

TokenStream::TokenStream()
	: fCStatus(B_NO_INIT)
	, fPos(-1)
	, fStrLen(-1)
{
}

TokenStream::~TokenStream() {
	Unset();
}
	
status_t
TokenStream::SetTo(const std::string &string) {
	Unset();
	fStrLen = string.length();
	CharStream stream(string);
	if (stream.InitCheck() != B_OK) 
		throw new Err("Sniffer scanner error: Unable to intialize character stream", -1);
	
	typedef enum TokenStreamScannerState {
		tsssStart,
		tsssOneSingle,
		tsssOneDouble,
		tsssOneZero,
		tsssZeroX,
		tsssOneHex,
		tsssTwoHex,
		tsssIntOrFloat,
		tsssFloat,
		tsssLonelyDecimalPoint,
		tsssLonelyMinusOrPlus,
		tsssLonelyFloatExtension,
		tsssLonelyFloatExtensionWithSign,
		tsssExtendedFloat,
		tsssUnquoted,
		tsssEscape,
		tsssEscapeX,
		tsssEscapeOneOctal,
		tsssEscapeTwoOctal,
		tsssEscapeOneHex,
	} TokenStreamScannerState;
	
	TokenStreamScannerState state = tsssStart;
	TokenStreamScannerState escapedState = tsssStart;
		// Used to remember which state to return to from an escape sequence

	std::string charStr = "";	// Used to build up character strings
	char lastChar = 0;			// For two char lookahead
	char lastLastChar = 0;		// For three char lookahead (have I mentioned I hate octal?)
	bool keepLooping = true;
	ssize_t startPos = 0;
	while (keepLooping) {
		ssize_t pos = stream.Pos();
		char ch = stream.Get();
		switch (state) {				
			case tsssStart:
				startPos = pos;
				switch (ch) {
					case 0x3:	// End-Of-Text
						if (stream.IsEmpty()) 
							keepLooping = false;
						else 
							throw new Err(std::string("Sniffer pattern error: invalid character '") + ch + "'", pos);
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
						
					case '+':	
					case '-':
						charStr = ch;
						lastChar = ch;
						state = tsssLonelyMinusOrPlus;
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
												
					case '&':	AddToken(Ampersand, pos);		break;
					case '(':	AddToken(LeftParen, pos);		break;
					case ')':	AddToken(RightParen, pos);		break;
					case ':':	AddToken(Colon, pos);			break;
					case '[':	AddToken(LeftBracket, pos);		break;
					
					case '\\':
						charStr = "";					// Clear our string
						state = tsssEscape;
						escapedState = tsssUnquoted;	// Unquoted strings begin with an escaped character
						break;							
					
					case ']':	AddToken(RightBracket, pos);		break;
					case '|':	AddToken(Divider, pos);			break;
					
					default:
						throw new Err(std::string("Sniffer pattern error: invalid character '") + ch + "'", pos);
				}			
				break;
				
			case tsssOneSingle:
				switch (ch) {
					case '\\':
						escapedState = state;		// Save our state
						state = tsssEscape;			// Handle the escape sequence
						break;							
					case '\'':
						AddString(charStr, startPos);
						state = tsssStart;
						break;
					case 0x3:
						if (stream.IsEmpty())
							throw new Err(std::string("Sniffer pattern error: unterminated single-quoted string"), pos);
						else
							charStr += ch;
						break;
					default:							
						charStr += ch;
						break;
				}
				break;
				
			case tsssOneDouble:
				switch (ch) {
					case '\\':
						escapedState = state;		// Save our state
						state = tsssEscape;			// Handle the escape sequence
						break;							
					case '"':
						AddString(charStr, startPos);
						state = tsssStart;
						break;				
					case 0x3:
						if (stream.IsEmpty())
							throw new Err(std::string("Sniffer pattern error: unterminated double-quoted string"), pos);
						else
							charStr += ch;
						break;
					default:							
						charStr += ch;
						break;
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
				} else if (ch == 'e' || ch == 'E') {
					charStr += ch;
					state = tsssLonelyFloatExtension;
				} else {
					// Terminate the number
					AddInt(charStr.c_str(), startPos);
					
					// Push the last char back on and try again
					stream.Unget();
					state = tsssStart;
				}
				break;
				
			case tsssZeroX:
				if (isHexChar(ch)) {
					lastChar = ch;
					state = tsssOneHex;
				} else
					throw new Err(std::string("Sniffer pattern error: incomplete hex code"), pos);
				break;
				
			case tsssOneHex:
				if (isHexChar(ch)) {
					try { 
						charStr += hexToChar(lastChar, ch);
					} catch (Err *err) {
						if (err)
							err->SetPos(pos);
						throw err;
					}
					state = tsssTwoHex;
				} else 
					throw new Err(std::string("Sniffer pattern error: bad hex literal"), pos);	// Same as R5
				break;
				
			case tsssTwoHex:
				if (isHexChar(ch)) {
					lastChar = ch;
					state = tsssOneHex;
				} else {
					AddString(charStr, startPos);
					stream.Unget();		// So punctuation gets handled properly
					state = tsssStart;
				}
				break;
				
			case tsssIntOrFloat:
				if (isDecimalChar(ch))
					charStr += ch;
				else if (ch == '.') {
					charStr += ch;
					state = tsssFloat;
				} else if (ch == 'e' || ch == 'E') {
					charStr += ch;
					state = tsssLonelyFloatExtension;
				} else {
					// Terminate the number
					AddInt(charStr.c_str(), startPos);
					
					// Push the last char back on and try again
					stream.Unget();
					state = tsssStart;						
				}
				break;
				
			case tsssFloat:
				if (isDecimalChar(ch))
					charStr += ch;
				else if (ch == 'e' || ch == 'E') {
					charStr += ch;
					state = tsssLonelyFloatExtension;
				} else {
					// Terminate the number
					AddFloat(charStr.c_str(), startPos);
					
					// Push the last char back on and try again
					stream.Unget();
					state = tsssStart;						
				}
				break;
				
			case tsssLonelyDecimalPoint:
				if (isDecimalChar(ch)) {
					charStr += ch;
					state = tsssFloat;
				} else
					throw new Err(std::string("Sniffer pattern error: incomplete floating point number"), pos);
				break;
				
			case tsssLonelyMinusOrPlus:
				if (isDecimalChar(ch)) {
					charStr += ch;
					state = tsssIntOrFloat;
				} else if (ch == '.') {
					charStr += ch;
					state = tsssLonelyDecimalPoint;					
				} else if (ch == 'i' && lastChar == '-') {
					AddToken(CaseInsensitiveFlag, startPos);
					state = tsssStart;				
				} else
					throw new Err(std::string("Sniffer pattern error: incomplete signed number or invalid flag"), pos);
				break;

			case tsssLonelyFloatExtension:
				if (ch == '+' || ch == '-') {
					charStr += ch;
					state = tsssLonelyFloatExtensionWithSign;
				} else if (isDecimalChar(ch)) {
					charStr += ch;
					state = tsssExtendedFloat;
				} else
					throw new Err(std::string("Sniffer pattern error: incomplete extended-notation floating point number"), pos);
				break;
				
			case tsssLonelyFloatExtensionWithSign:
				if (isDecimalChar(ch)) {
					charStr += ch;
					state = tsssExtendedFloat;
				} else
					throw new Err(std::string("Sniffer pattern error: incomplete extended-notation floating point number"), pos);
				break;
				
			case tsssExtendedFloat:
				if (isDecimalChar(ch)) {
					charStr += ch;
					state = tsssExtendedFloat;
				} else {
					// Terminate the number
					AddFloat(charStr.c_str(), startPos);
					
					// Push the last char back on and try again
					stream.Unget();
					state = tsssStart;						
				}
				break;

			case tsssUnquoted:
				if (ch == '\\') {
					escapedState = state;		// Save our state
					state = tsssEscape;			// Handle the escape sequence
				} else if (isWhiteSpace(ch) || isPunctuation(ch)) {
					AddString(charStr, startPos);
					stream.Unget();				// In case it's punctuation, let tsssStart handle it
					state = tsssStart;
				} else if (ch == 0x3 && stream.IsEmpty()) {
					AddString(charStr, startPos);
					keepLooping = false;
				} else {							
					charStr += ch;
				}
				break;
				
			case tsssEscape:
				if (isOctalChar(ch)) {
					lastChar = ch;
					state = tsssEscapeOneOctal;
				} else if (ch == 'x') {
					state = tsssEscapeX;
				} else {
					// Check for a true end-of-text marker
					if (ch == 0x3 && stream.IsEmpty())
						throw new Err(std::string("Sniffer pattern error: incomplete escape sequence"), pos);
					else {
						charStr += escapeChar(ch);
						state = escapedState;	// Return to the state we were in before the escape
					}
				}									
				break;
				
			case tsssEscapeX:
				if (isHexChar(ch)) {
					lastChar = ch;
					state = tsssEscapeOneHex;
				} else 
					throw new Err(std::string("Sniffer pattern error: incomplete escaped hex code"), pos);
				break;
				
			case tsssEscapeOneOctal:
				if (isOctalChar(ch)) {
					lastLastChar = lastChar;
					lastChar = ch;
					state = tsssEscapeTwoOctal;
				} else {
					// First handle the octal
					try {
						charStr += octalToChar(lastChar);
					} catch (Err *err) {
						if (err)
							err->SetPos(startPos);
						throw err;
					}
					
					// Push the new char back on and let the state we
					// were in when the escape sequence was hit handle it.
					stream.Unget();
					state = escapedState;
				}									
				break;

			case tsssEscapeTwoOctal:
				if (isOctalChar(ch)) {
					try {
						charStr += octalToChar(lastLastChar, lastChar, ch);
					} catch (Err *err) {
						if (err)
							err->SetPos(startPos);
						throw err;
					}
					state = escapedState;
				} else {
					// First handle the octal
					try {
						charStr += octalToChar(lastLastChar, lastChar);
					} catch (Err *err) {
						if (err)
							err->SetPos(startPos);
						throw err;
					}
					
					// Push the new char back on and let the state we
					// were in when the escape sequence was hit handle it.
					stream.Unget();
					state = escapedState;
				}									
				break;

			case tsssEscapeOneHex:
				if (isHexChar(ch)) {
					try {
						charStr += hexToChar(lastChar, ch);
					} catch (Err *err) {
						if (err)
							err->SetPos(pos);
						throw err;
					}
					state = escapedState;
				} else
					throw new Err(std::string("Sniffer pattern error: incomplete escaped hex code"), pos);
				break;					
				
		}
	}
	if (state == tsssStart)	{
		fCStatus = B_OK;
		fPos = 0;
	} else {
		throw new Err("Sniffer pattern error: unterminated rule", stream.Pos());
	}
	
	return fCStatus;
}

void
TokenStream::Unset() {
	std::vector<Token*>::iterator i;
	for (i = fTokenList.begin(); i != fTokenList.end(); i++)
		delete *i;
	fTokenList.clear();
	fCStatus = B_NO_INIT;
	fStrLen = -1;
}

status_t
TokenStream::InitCheck() const {
	return fCStatus;
}
	
//! Returns a pointer to the next token in the stream.
/*! The TokenStream object retains owner ship of the Token object returned by Get().
    If Get() is called at the end of the stream, a pointer to a Err object is thrown.
*/
const Token*
TokenStream::Get() {
	if (fCStatus != B_OK)
		throw new Err("Sniffer parser error: TokenStream::Get() called on uninitialized TokenStream object", -1);
	if (fPos < (ssize_t)fTokenList.size()) 
		return fTokenList[fPos++];
	else {
		throw new Err("Sniffer pattern error: unterminated rule", EndPos());
//		fPos++;			// Increment fPos to keep Unget()s consistent
//		return NULL;	// Return NULL to signal end of list
	}
}

//! Places token returned by the most recent call to Get() back on the head of the stream.
/*! If Unget() is called at the beginning of the stream, a pointer to a Err object is thrown.
*/
void
TokenStream::Unget() {
	if (fCStatus != B_OK)
		throw new Err("Sniffer parser error: TokenStream::Unget() called on uninitialized TokenStream object", -1);
	if (fPos > 0) 
		fPos--;
	else
		throw new Err("Sniffer parser error: TokenStream::Unget() called at beginning of token stream", -1);
}


/*! \brief Reads the next token in the stream and verifies it is of the given type,
	throwing a pointer to a Err object if it is not.
*/
void
TokenStream::Read(TokenType type) {
	const Token *t = Get();
	if (t->Type() != type) {
		throw new Err((std::string("Sniffer pattern error: expected ") + tokenTypeToString(type)
	                + ", found " + tokenTypeToString(t->Type())).c_str(), t->Pos());
	}		
}

//! Conditionally reads the next token in the stream.
/*! CondRead() peeks at the next token in the stream. If it is of the given type, the
	token is removed from the stream and \c true is returned. If it is not of the
	given type, false is returned and the token remains at the head of the stream.
*/
bool
TokenStream::CondRead(TokenType type) {
	const Token *t = Get();
	if (t->Type() == type) {
		return true;
	} else {
		Unget();
		return false;
	}
}

ssize_t
TokenStream::Pos() const {
	return fPos < (ssize_t)fTokenList.size() ? fTokenList[fPos]->Pos() : fStrLen;
}

ssize_t
TokenStream::EndPos() const {
	return fStrLen;
}

bool
TokenStream::IsEmpty() const {
	return fCStatus != B_OK || fPos >= (ssize_t)fTokenList.size();
}

void
TokenStream::AddToken(TokenType type, ssize_t pos) {
	Token *token = new Token(type, pos);
	fTokenList.push_back(token);
}

void
TokenStream::AddString(const std::string &str, ssize_t pos) {
	Token *token = new StringToken(str, pos);
	fTokenList.push_back(token);
}

void
TokenStream::AddInt(const char *str, ssize_t pos) {
	// Convert the string to an int
	int32 value = atol(str);	
	Token *token = new IntToken(value, pos);
	fTokenList.push_back(token);
}

void
TokenStream::AddFloat(const char *str, ssize_t pos) {
	// Convert the string to a float
	double value = atof(str);
	Token *token = new FloatToken(value, pos);
	fTokenList.push_back(token);
}

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

char
escapeChar(char ch) {
	// I've manually handled all the escape sequences I could come 
	// up with, and for anything else I just return the character
	// passed in. Hex escapes are handled elsewhere, so \x just
	// returns 'x'. Similarly, octals are handled elsewhere, so \0
	// through \9 just return '0' through '9'.
	switch (ch) {
		case 'a':
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			return '\v';	
		default:
			return ch;
	}
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
		return hex-'A'+10;
	else
		throw new Err(std::string("Sniffer parser error: invalid hex digit '") + hex + "' passed to hexToChar()", -1);
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
		// Check for octals >= decimal 256
		if ((hi-'0') <= 3)
			return ((hi-'0') << 6) | ((mid-'0') << 3) | (low-'0');
		else
			throw new Err("Sniffer pattern error: invalid octal literal (octals must be between octal 0 and octal 377 inclusive)", -1);
	} else
		throw new Err(std::string("Sniffer parser error: invalid octal digit passed to hexToChar()"), -1);		
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

bool
isPunctuation(char ch) {
	switch (ch) {
		case '&':	
		case '(':	
		case ')':	
		case ':':	
		case '[':	
		case ']':	
		case '|':
			return true;
		default:
			return false;
	}			
}

const char*
BPrivate::Storage::Sniffer::tokenTypeToString(TokenType type) {
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
		case CaseInsensitiveFlag:
			return "CaseInsensitiveFlag";
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

//------------------------------------------------------------------------------
// Parser
//------------------------------------------------------------------------------

Parser::Parser()
	: fOutOfMemErr(new(std::nothrow) Err("Sniffer parser error: out of memory", -1))
{
}

Parser::~Parser() {
	delete fOutOfMemErr;
}

status_t
Parser::Parse(const char *rule, Rule *result, BString *parseError) {
	try {
		if (!rule)
			throw new Err("Sniffer pattern error: NULL pattern", -1);
		if (!result)
			return B_BAD_VALUE;		
		if (stream.SetTo(rule) != B_OK)
			throw new Err("Sniffer parser error: Unable to intialize token stream", -1);
			
		ParseRule(result);
		
		return B_OK;
		
	} catch (Err *err) {
//		cout << "Caught error" << endl;
		if (parseError)
			parseError->SetTo(ErrorMessage(err, rule).c_str());
		delete err;
		return rule ? (status_t)B_BAD_MIME_SNIFFER_RULE : (status_t)B_BAD_VALUE;
	}
}

std::string
Parser::ErrorMessage(Err *err, const char *rule) {
	const char* msg = (err && err->Msg())
    	                ? err->Msg()
    	                  : "Sniffer parser error: Unexpected error with no supplied error message";
    ssize_t pos = err && (err->Pos() >= 0) ? err->Pos() : 0;
    std::string str = std::string(rule ? rule : "") + "\n";
    for (int i = 0; i < pos; i++)
    	str += " ";
    str += "^    ";
    str += msg;
    return str;
}

void
Parser::ParseRule(Rule *result) {
	if (!result)
		throw new Err("Sniffer parser error: NULL Rule object passed to Parser::ParseRule()", -1);

	// Priority
	double priority = ParsePriority();
	// Conjunction List	
	std::vector<DisjList*>* list = ParseConjList();
	
	result->SetTo(priority, list);	
}

double
Parser::ParsePriority() {
	const Token *t = stream.Get();
	if (t->Type() == FloatingPoint || t->Type() == Integer) {
		double result = t->Float();
		if (0.0 <= result && result <= 1.0)
			return result;
		else {
//			cout << "(priority == " << result << ")" << endl;
			throw new Err("Sniffer pattern error: invalid priority", t->Pos());
		}
	} else
		throw new Err("Sniffer pattern error: match level expected", t->Pos());	// Same as R5 
}

std::vector<DisjList*>*
Parser::ParseConjList() {
	std::vector<DisjList*> *list = new(std::nothrow) std::vector<DisjList*>;
	if (!list)
		ThrowOutOfMemError(stream.Pos());		
	try {
		// DisjList+
		int count = 0;
		while (true) {
			DisjList* expr = ParseDisjList();
			if (!expr)
				break;
			else {
				list->push_back(expr);
				count++;
			}
		}	
		if (count == 0)
			throw new Err("Sniffer pattern error: missing expression", -1);			
	} catch (...) {
		delete list;
		throw;
	}
	return list;
}

DisjList*
Parser::ParseDisjList() {
	// If we've run out of tokens right now, it's okay, but
	// we need to let ParseConjList() know what's up
	if (stream.IsEmpty())
		return NULL;

	// Peek ahead, then let the appropriate Parse*List()
	// functions handle things
	const Token *t1 = stream.Get();
	
	// PatternList | RangeList
	if (t1->Type() == LeftParen) {
		const Token *t2 = stream.Get();
		// Skip the case-insensitive flag, if there is one
		const Token *tokenOfInterest = (t2->Type() == CaseInsensitiveFlag) ? stream.Get() : t2;
		if (t2 != tokenOfInterest)
			stream.Unget();	// We called Get() three times
		stream.Unget();
		stream.Unget();
		// RangeList
		if (tokenOfInterest->Type() == LeftBracket) {
			return ParseRPatternList();
		// PatternList
		} else {
			return ParsePatternList(Range(0,0));
		}
	// Range, PatternList
	} else if (t1->Type() == LeftBracket) {
		stream.Unget();
		return ParsePatternList(ParseRange());
	} else {
		throw new Err("Sniffer pattern error: missing pattern", t1->Pos());	// Same as R5
	}

	// PatternList
	// RangeList
	// Range + PatternList
}

Range
Parser::ParseRange() {
	int32 start, end;
	// LeftBracket
	stream.Read(LeftBracket);
	// Integer
	{
		const Token *t = stream.Get();
		if (t->Type() == Integer) {
			start = t->Int();
			end = start;	// In case we aren't given an explicit end
		} else
			throw new Err("Sniffer pattern error: pattern offset expected", t->Pos());
	}
	// [Colon, Integer] RightBracket
	{
		const Token *t = stream.Get();
		// Colon, Integer, RightBracket
		if (t->Type() == Colon) {
			// Integer
			{
				const Token *t = stream.Get();
				if (t->Type() == Integer) {
					end = t->Int();
				} else
					ThrowUnexpectedTokenError(Integer, t);						
			}
			// RightBracket
			stream.Read(RightBracket);
		// !(Colon, Integer) RightBracket
		} else if (t->Type() == RightBracket) {
			// Nothing to do here...

		// Something else...
		} else
			ThrowUnexpectedTokenError(Colon, Integer, t);
	}
	Range range(start, end);
	if (range.InitCheck() == B_OK)
		return range;
	else 
		throw range.GetErr();
}

DisjList*
Parser::ParsePatternList(Range range) {
	PatternList *list = new(std::nothrow) PatternList(range);
	if (!list)
		ThrowOutOfMemError(stream.Pos());
	try {		
		// LeftParen
		stream.Read(LeftParen);
		// [Flag] Pattern, (Divider, [Flag] Pattern)*
		while (true) {
			// [Flag]
			if (stream.CondRead(CaseInsensitiveFlag))
				list->SetCaseInsensitive(true);		
			// Pattern
			list->Add(ParsePattern());
			// [Divider]
			if (!stream.CondRead(Divider))
				break;
		} 
		// RightParen
		const Token *t = stream.Get();
		if (t->Type() != RightParen)
			throw new Err("Sniffer pattern error: expecting '|', ')', or possibly '&'", t->Pos());
	} catch (...) {
		delete list;
		throw;
	}
	return list;
}

DisjList*
Parser::ParseRPatternList() {
	RPatternList *list = new(std::nothrow) RPatternList();
	if (!list)
		ThrowOutOfMemError(stream.Pos());
	try {
		// LeftParen
		stream.Read(LeftParen);
		// [Flag] RPattern, (Divider, [Flag] RPattern)*
		while (true) {
			// [Flag]
			if (stream.CondRead(CaseInsensitiveFlag))
				list->SetCaseInsensitive(true);		
			// RPattern
			list->Add(ParseRPattern());
			// [Divider]
			if (!stream.CondRead(Divider))
				break;
		} 
		// RightParen
		const Token *t = stream.Get();
		if (t->Type() != RightParen)
			throw new Err("Sniffer pattern error: expecting '|', ')', or possibly '&'", t->Pos());
	} catch (...) {
		delete list;
		throw;
	}
	return list;
}

RPattern*
Parser::ParseRPattern() {
	// Range
	Range range = ParseRange();
	// Pattern
	Pattern *pattern = ParsePattern();
	
	RPattern *result = new(std::nothrow) RPattern(range, pattern);
	if (result) {
		if (result->InitCheck() == B_OK)
			return result;
		else {
			Err *err = result->GetErr();
			delete result;
			throw err;
		}
	} else
		ThrowOutOfMemError(stream.Pos());
	return NULL;
}

Pattern*
Parser::ParsePattern() {
	std::string str;	
	// String
	{
		const Token *t = stream.Get();
		if (t->Type() == CharacterString) 
			str = t->String();		
		else
			throw new Err("Sniffer pattern error: missing pattern", t->Pos());
	}	
	// [Ampersand, String]
	if (stream.CondRead(Ampersand)) {
		// String (i.e. Mask)
		const Token *t = stream.Get();
		if (t->Type() == CharacterString) {
			Pattern *result = new(std::nothrow) Pattern(str, t->String());
			if (!result)
				ThrowOutOfMemError(t->Pos());
			if (result->InitCheck() == B_OK) {
				return result;
			} else {
				Err *err = result->GetErr();
				delete result;
				if (err) {
					err->SetPos(t->Pos());
				}
				throw err;
			}
		} else
			ThrowUnexpectedTokenError(CharacterString, t);
	} else {
		// No mask specified. 
		Pattern *result = new(std::nothrow) Pattern(str);
		if (result) {
			if (result->InitCheck() == B_OK)
				return result;
			else {
				Err *err = result->GetErr();
				delete result;
				throw err;
			}
		} else
			ThrowOutOfMemError(stream.Pos());
	}
	return NULL;
}

void
Parser::ThrowEndOfStreamError() {
	throw new Err("Sniffer pattern error: unterminated rule", stream.EndPos());
}

inline
void
Parser::ThrowOutOfMemError(ssize_t pos) {
	if (fOutOfMemErr)
		fOutOfMemErr->SetPos(pos);
	Err *err = fOutOfMemErr;
	fOutOfMemErr = NULL;
	throw err;	
}

void
Parser::ThrowUnexpectedTokenError(TokenType expected, const Token *found) {
	throw new Err((std::string("Sniffer pattern error: expected ") + tokenTypeToString(expected)
	                + ", found " + (found ? tokenTypeToString(found->Type()) : "NULL token")).c_str()
	                , (found ? found->Pos() : stream.EndPos()));	
}

void
Parser::ThrowUnexpectedTokenError(TokenType expected1, TokenType expected2, const Token *found) {
	throw new Err((std::string("Sniffer pattern error: expected ") + tokenTypeToString(expected1)
	                + " or " + tokenTypeToString(expected2) + ", found "
	                + (found ? tokenTypeToString(found->Type()) : "NULL token")).c_str()
	                , (found ? found->Pos() : stream.EndPos()));	
}




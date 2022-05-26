//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file sniffer/Parser.h
	MIME sniffer rule parser declarations
*/
#ifndef _SNIFFER_PARSER_H
#define _SNIFFER_PARSER_H

#include <SupportDefs.h>
#include <sniffer/CharStream.h>
#include <sniffer/Err.h>
#include <sniffer/Range.h>
#include <sniffer/Rule.h>
#include <List.h>
#include <string>
#include <vector>

class BString;

//! MIME Sniffer related classes
namespace BPrivate {
namespace Storage {
namespace Sniffer {

class Rule;
class DisjList;
class RPattern;
class Pattern;

//------------------------------------------------------------------------------
// The mighty parsing function ;-)
//------------------------------------------------------------------------------

status_t parse(const char *rule, Rule *result, BString *parseError = NULL);

//------------------------------------------------------------------------------
// Classes used internally by the parser
//------------------------------------------------------------------------------

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
	CaseInsensitiveFlag,
	CharacterString,
	Integer,
	FloatingPoint
} TokenType;

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
	virtual ~Token();
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
	virtual ~StringToken();
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
	virtual ~IntToken();
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
	virtual ~FloatToken();
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
	status_t fCStatus;
	int fPos;
	int fStrLen;


	TokenStream(const TokenStream &ref);
	TokenStream& operator=(const TokenStream &ref);
};

//! Handles parsing a sniffer rule, yielding either a parsed rule or a descriptive error message.
/*! A MIME sniffer rule is valid if it is well-formed with respect to the
	following grammar and fulfills some further conditions listed thereafter:
	
	<code>
	Rule			::= LWS Priority LWS ConjList LWS	
	ConjList		::= DisjList (LWS DisjList)*	
	DisjList		::= "(" LWS PatternList LWS ")"										
						| "(" LWS RPatternList LWS ")"									
						| Range LWS "(" LWS PatternList LWS ")"						
	RPatternList	::= [Flag LWS] RPattern (LWS "|" LWS [Flag LWS] RPattern)*	
	PatternList		::= [Flag LWS] Pattern (LWS "|" LWS [Flag LWS] Pattern)*
	
	RPattern		::= LWS Range LWS Pattern
	Pattern			::= PString [ LWS "&" LWS Mask ]
	Range			::=	"[" LWS SDecimal [LWS ":" LWS SDecimal] LWS "]"

	Priority		::= Float
	Mask			::= PString
	PString			::= HexLiteral | QuotedString | UnquotedString

	HexLiteral		::= "0x" HexPair HexPair*
	HexPair			::= HexChar HexChar
	
	QuotedString	::= SingleQuotedString | DoubleQuotedString
	SQuotedString	:= "'" SQChar+ "'"
	DQuotedString	:= '"' DQChar+ '"'

	UnquotedString	::= EscapedChar UChar*
	EscapedChar		::= OctalEscape | HexEscape | "\" Char
	OctalEscape		::= "\" [[OctHiChar] OctChar] OctChar
	HexEscape		::= "\x" HexPair

	Flag			::= "-i"

	SDecimal		::= [Sign] Decimal
	Decimal			::= DecChar DecChar*
	Float			::= Fixed [("E" | "e") SDecimal]
	Fixed			::= SDecimal ["." [Decimal]] | [Sign] "." Decimal
	Sign			::= "+" | "-"

	PunctuationChar	::= "(" | ")" | "[" | "]" | "|" | "&" | ":"
	OctHiChar		::= "0" | "1" | "2" | "3" 
	OctChar			::= OctHiChar | "4" | "5" | "6" | "7"
	DecChar			::= OctChar | "8" | "9"
	HexChar			::= DecChar | "a" | "b" | "c" | "d" | "e" | "f" | "A" | "B" | "C"
						| "D" | "E" | "F"

	Char			:: <any character>
	SQChar			::= <Char except "\", "'"> | EscapedChar
	DQChar			::= <Char except "\", '"'> | EscapedChar
	UChar			::= <Char except "\", LWSChar,  and PunctuationChar> | EscapedChar

	LWS				::= LWSChar*
	LWSChar			::= " " | TAB | LF
	</code>

	Conditions:
	- If a mask is specified for a pattern, this mask must have the same
	  length as the pattern string.
	- 0.0 <= Priority <= 1.0
	- 0 <= Range begin <= Range end
	
	Notes:
	- If a case-insensitive flag ("-i") appears in front of any Pattern or RPattern
	  in a DisjList, case-insensitivity is applied to the entire DisjList.

	Examples:
	- 1.0 ('ABCD')
	  The file must start with the string "ABCD". The priority of the rule
	  is 1.0 (maximal).
	- 0.8 [0:3] ('ABCD' | 'abcd')
	  The file must contain the string "ABCD" or "abcd" starting somewhere in
	  the first four bytes. The rule priority is 0.8.
	- 0.5 ([0:3] 'ABCD' | [0:3] 'abcd' | [13] 'EFGH')
	  The file must contain the string "ABCD" or "abcd" starting somewhere in
	  the first four bytes or the string "EFGH" at position 13. The rule
	  priority is 0.5.
	- 0.8 [0:3] ('ABCD' & 0xff00ffff | 'abcd' & 0xffff00ff)
	  The file must contain the string "A.CD" or "ab.d" (whereas "." is an
	  arbitrary character) starting somewhere in the first four bytes. The
	  rule priority is 0.8.
	- 0.3 [10] ('mnop') ('abc') [20] ('xyz')
	  The file must contain the string 'abc' at the beginning of the file,
	  the string 'mnop' starting at position 10, and the string 'xyz'
	  starting at position 20. The rule priority is 0.3.
	- 200e-3 (-i 'ab')
	  The file must contain the string 'ab', 'aB', 'Ab', or 'AB' at the
	  beginning of the file. The rule priority is 0.2.

	Real examples:
	- 0.20 ([0]"//" | [0]"/\*" | [0:32]"#include" | [0:32]"#ifndef"
	        | [0:32]"#ifdef")
	  text/x-source-code
	- 0.70 ("8BPS  \000\000\000\000" & 0xffffffff0000ffffffff )
	  image/x-photoshop
	- 0.40 [0:64]( -i "&lt;HTML" | "&lt;HEAD" | "&lt;TITLE" | "&lt;BODY"
			| "&lt;TABLE" | "&lt;!--" | "&lt;META" | "&lt;CENTER")
	  text/html
	  
*/
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
	std::vector<DisjList*>* ParseConjList();
	DisjList* ParseDisjList();
	Range ParseRange();
	DisjList* ParsePatternList(Range range);
	DisjList* ParseRPatternList();
	RPattern* ParseRPattern();
	Pattern* ParsePattern();
	
	TokenStream stream;
	
	Err *fOutOfMemErr;
};

};	// namespace Sniffer
};	// namespace Storage
};	// namespace BPrivate

#endif	// _SNIFFER_PARSER_H





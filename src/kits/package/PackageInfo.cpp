/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageInfo.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <File.h>
#include <Entry.h>
#include <Message.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageReader.h>
#include <package/hpkg/v1/PackageInfoContentHandler.h>
#include <package/hpkg/v1/PackageReader.h>
#include <package/PackageInfoContentHandler.h>


namespace BPackageKit {


namespace {


enum TokenType {
	TOKEN_WORD,
	TOKEN_QUOTED_STRING,
	TOKEN_OPERATOR_ASSIGN,
	TOKEN_OPERATOR_LESS,
	TOKEN_OPERATOR_LESS_EQUAL,
	TOKEN_OPERATOR_EQUAL,
	TOKEN_OPERATOR_NOT_EQUAL,
	TOKEN_OPERATOR_GREATER_EQUAL,
	TOKEN_OPERATOR_GREATER,
	TOKEN_OPEN_BRACE,
	TOKEN_CLOSE_BRACE,
	TOKEN_ITEM_SEPARATOR,
	//
	TOKEN_EOF,
};


struct ParseError {
	BString 	message;
	const char*	pos;

	ParseError(const BString& _message, const char* _pos)
		: message(_message), pos(_pos)
	{
	}
};


}	// anonymous namespace


// #pragma mark - Parser


/*
 * Parses a ".PackageInfo" file and fills a BPackageInfo object with the
 * package info elements found.
 */
class BPackageInfo::Parser {
public:
								Parser(ParseErrorListener* listener = NULL);

			status_t			Parse(const BString& packageInfoString,
									BPackageInfo* packageInfo);

			status_t			ParseVersion(const BString& versionString,
									bool revisionIsOptional,
									BPackageVersion& _version);

private:
			struct Token;
			struct ListElementParser;
	friend	struct ListElementParser;

			Token				_NextToken();
			void				_RewindTo(const Token& token);

			void				_ParseStringValue(BString* value,
									const char** _tokenPos = NULL);
			uint32				_ParseFlags();
			void				_ParseArchitectureValue(
									BPackageArchitecture* value);
			void				_ParseVersionValue(BPackageVersion* value,
									bool revisionIsOptional);
	static	void				_ParseVersionValue(Token& word,
									BPackageVersion* value,
									bool revisionIsOptional);
			void				_ParseList(ListElementParser& elementParser,
									bool allowSingleNonListElement);
			void				_ParseStringList(BStringList* value,
									bool allowQuotedStrings = true,
									bool convertToLowerCase = false);
			void				_ParseResolvableList(
									BObjectList<BPackageResolvable>* value);
			void				_ParseResolvableExprList(
									BObjectList<BPackageResolvableExpression>*
										value,
									BString* _basePackage = NULL);

			void				_Parse(BPackageInfo* packageInfo);

	static	bool				_IsAlphaNumUnderscore(const BString& string,
									const char* additionalChars,
									int32* _errorPos);
	static	bool				_IsAlphaNumUnderscore(const char* string,
									const char* additionalChars,
									int32* _errorPos);
	static	bool				_IsAlphaNumUnderscore(const char* start,
									const char* end,
									const char* additionalChars,
									int32* _errorPos);
	static	bool				_IsValidResolvableName(const char* string,
									int32* _errorPos);

private:
			ParseErrorListener*	fListener;
			const char*			fPos;
};


struct BPackageInfo::Parser::Token {
	TokenType	type;
	BString		text;
	const char*	pos;

	Token(TokenType _type, const char* _pos, int length = 0)
		: type(_type), pos(_pos)
	{
		if (length != 0) {
			text.SetTo(pos, length);

			if (type == TOKEN_QUOTED_STRING) {
				// unescape value of quoted string
				char* value = text.LockBuffer(length);
				if (value == NULL)
					return;
				int index = 0;
				int newIndex = 0;
				bool lastWasEscape = false;
				while (char c = value[index++]) {
					if (lastWasEscape) {
						lastWasEscape = false;
						// map \n to newline and \t to tab
						if (c == 'n')
							c = '\n';
						else if (c == 't')
							c = '\t';
					} else if (c == '\\') {
						lastWasEscape = true;
						continue;
					}
					value[newIndex++] = c;
				}
				value[newIndex] = '\0';
				text.UnlockBuffer(newIndex);
			}
		}
	}

	operator bool() const
	{
		return type != TOKEN_EOF;
	}
};


struct BPackageInfo::Parser::ListElementParser {
	virtual void operator()(const Token& token) = 0;
};


BPackageInfo::ParseErrorListener::~ParseErrorListener()
{
}


BPackageInfo::Parser::Parser(ParseErrorListener* listener)
	:
	fListener(listener),
	fPos(NULL)
{
}


status_t
BPackageInfo::Parser::Parse(const BString& packageInfoString,
	BPackageInfo* packageInfo)
{
	if (packageInfo == NULL)
		return B_BAD_VALUE;

	fPos = packageInfoString.String();

	try {
		_Parse(packageInfo);
	} catch (const ParseError& error) {
		if (fListener != NULL) {
			// map error position to line and column
			int line = 1;
			int column;
			int32 offset = error.pos - packageInfoString.String();
			int32 newlinePos = packageInfoString.FindLast('\n', offset - 1);
			if (newlinePos < 0)
				column = offset;
			else {
				column = offset - newlinePos;
				do {
					line++;
					newlinePos = packageInfoString.FindLast('\n',
						newlinePos - 1);
				} while (newlinePos >= 0);
			}
			fListener->OnError(error.message, line, column);
		}
		return B_BAD_DATA;
	} catch (const std::bad_alloc& e) {
		if (fListener != NULL)
			fListener->OnError("out of memory", 0, 0);
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
BPackageInfo::Parser::ParseVersion(const BString& versionString,
	bool revisionIsOptional, BPackageVersion& _version)
{
	fPos = versionString.String();

	try {
		Token token(TOKEN_WORD, fPos, versionString.Length());
		_ParseVersionValue(token, &_version, revisionIsOptional);
	} catch (const ParseError& error) {
		if (fListener != NULL) {
			int32 offset = error.pos - versionString.String();
			fListener->OnError(error.message, 1, offset);
		}
		return B_BAD_DATA;
	} catch (const std::bad_alloc& e) {
		if (fListener != NULL)
			fListener->OnError("out of memory", 0, 0);
		return B_NO_MEMORY;
	}

	return B_OK;
}


BPackageInfo::Parser::Token
BPackageInfo::Parser::_NextToken()
{
	// Eat any whitespace or comments. Also eat ';' -- they have the same
	// function as newlines. We remember the last encountered ';' or '\n' and
	// return it as a token afterwards.
	const char* itemSeparatorPos = NULL;
	bool inComment = false;
	while ((inComment && *fPos != '\0') || isspace(*fPos) || *fPos == ';'
		|| *fPos == '#') {
		if (*fPos == '#') {
			inComment = true;
		} else if (*fPos == '\n') {
			itemSeparatorPos = fPos;
			inComment = false;
		} else if (!inComment && *fPos == ';')
			itemSeparatorPos = fPos;
		fPos++;
	}

	if (itemSeparatorPos != NULL) {
		return Token(TOKEN_ITEM_SEPARATOR, itemSeparatorPos);
	}

	const char* tokenPos = fPos;
	switch (*fPos) {
		case '\0':
			return Token(TOKEN_EOF, fPos);

		case '{':
			fPos++;
			return Token(TOKEN_OPEN_BRACE, tokenPos);

		case '}':
			fPos++;
			return Token(TOKEN_CLOSE_BRACE, tokenPos);

		case '<':
			fPos++;
			if (*fPos == '=') {
				fPos++;
				return Token(TOKEN_OPERATOR_LESS_EQUAL, tokenPos, 2);
			}
			return Token(TOKEN_OPERATOR_LESS, tokenPos, 1);

		case '=':
			fPos++;
			if (*fPos == '=') {
				fPos++;
				return Token(TOKEN_OPERATOR_EQUAL, tokenPos, 2);
			}
			return Token(TOKEN_OPERATOR_ASSIGN, tokenPos, 1);

		case '!':
			if (fPos[1] == '=') {
				fPos += 2;
				return Token(TOKEN_OPERATOR_NOT_EQUAL, tokenPos, 2);
			}
			break;

		case '>':
			fPos++;
			if (*fPos == '=') {
				fPos++;
				return Token(TOKEN_OPERATOR_GREATER_EQUAL, tokenPos, 2);
			}
			return Token(TOKEN_OPERATOR_GREATER, tokenPos, 1);

		case '"':
		case '\'':
		{
			char quoteChar = *fPos;
			fPos++;
			const char* start = fPos;
			// anything until the next quote is part of the value
			bool lastWasEscape = false;
			while ((*fPos != quoteChar || lastWasEscape) && *fPos != '\0') {
				if (lastWasEscape)
					lastWasEscape = false;
				else if (*fPos == '\\')
					lastWasEscape = true;
				fPos++;
			}
			if (*fPos != quoteChar)
				throw ParseError("unterminated quoted-string", tokenPos);
			const char* end = fPos++;
			return Token(TOKEN_QUOTED_STRING, start, end - start);
		}

		default:
		{
			const char* start = fPos;
			while (isalnum(*fPos) || *fPos == '.' || *fPos == '-'
				|| *fPos == '_' || *fPos == ':' || *fPos == '+'
				|| *fPos == '~') {
				fPos++;
			}
			if (fPos == start)
				break;
			return Token(TOKEN_WORD, start, fPos - start);
		}
	}

	BString error = BString("unknown token '") << *fPos << "' encountered";
	throw ParseError(error.String(), fPos);
}


void
BPackageInfo::Parser::_RewindTo(const Token& token)
{
	fPos = token.pos;
}


void
BPackageInfo::Parser::_ParseStringValue(BString* value, const char** _tokenPos)
{
	Token string = _NextToken();
	if (string.type != TOKEN_QUOTED_STRING && string.type != TOKEN_WORD)
		throw ParseError("expected quoted-string or word", string.pos);

	*value = string.text;
	if (_tokenPos != NULL)
		*_tokenPos = string.pos;
}


void
BPackageInfo::Parser::_ParseArchitectureValue(BPackageArchitecture* value)
{
	Token arch = _NextToken();
	if (arch.type == TOKEN_WORD) {
		for (int i = 0; i < B_PACKAGE_ARCHITECTURE_ENUM_COUNT; ++i) {
			if (arch.text.ICompare(BPackageInfo::kArchitectureNames[i]) == 0) {
				*value = (BPackageArchitecture)i;
				return;
			}
		}
	}

	BString error("architecture must be one of: [");
	for (int i = 0; i < B_PACKAGE_ARCHITECTURE_ENUM_COUNT; ++i) {
		if (i > 0)
			error << ",";
		error << BPackageInfo::kArchitectureNames[i];
	}
	error << "]";
	throw ParseError(error, arch.pos);
}


void
BPackageInfo::Parser::_ParseVersionValue(BPackageVersion* value,
	bool revisionIsOptional)
{
	Token word = _NextToken();
	_ParseVersionValue(word, value, revisionIsOptional);
}


/*static*/ void
BPackageInfo::Parser::_ParseVersionValue(Token& word, BPackageVersion* value,
	bool revisionIsOptional)
{
	if (word.type != TOKEN_WORD)
		throw ParseError("expected word (a version)", word.pos);

	// get the revision number
	uint32 revision = 0;
	int32 dashPos = word.text.FindLast('-');
	if (dashPos >= 0) {
		char* end;
		long long number = strtoll(word.text.String() + dashPos + 1, &end,
			0);
		if (*end != '\0' || number < 0 || number > UINT_MAX) {
			throw ParseError("revision must be a number > 0 and < UINT_MAX",
				word.pos + dashPos + 1);
		}

		revision = (uint32)number;
		word.text.Truncate(dashPos);
	}

	if (revision == 0 && !revisionIsOptional) {
		throw ParseError("expected revision number (-<number> suffix)",
			word.pos + word.text.Length());
	}

	// get the pre-release string
	BString preRelease;
	int32 tildePos = word.text.FindLast('~');
	if (tildePos >= 0) {
		word.text.CopyInto(preRelease, tildePos + 1,
			word.text.Length() - tildePos - 1);
		word.text.Truncate(tildePos);

		if (preRelease.IsEmpty()) {
			throw ParseError("invalid empty pre-release string",
				word.pos + tildePos + 1);
		}

		int32 errorPos;
		if (!_IsAlphaNumUnderscore(preRelease, ".", &errorPos)) {
			throw ParseError("invalid character in pre-release string",
				word.pos + tildePos + 1 + errorPos);
		}
	}

	// get major, minor, and micro strings
	BString major;
	BString minor;
	BString micro;
	int32 firstDotPos = word.text.FindFirst('.');
	if (firstDotPos < 0)
		major = word.text;
	else {
		word.text.CopyInto(major, 0, firstDotPos);
		int32 secondDotPos = word.text.FindFirst('.', firstDotPos + 1);
		if (secondDotPos == firstDotPos + 1)
			throw ParseError("expected minor version", word.pos + secondDotPos);

		if (secondDotPos < 0) {
			word.text.CopyInto(minor, firstDotPos + 1, word.text.Length());
		} else {
			word.text.CopyInto(minor, firstDotPos + 1,
				secondDotPos - (firstDotPos + 1));
			word.text.CopyInto(micro, secondDotPos + 1, word.text.Length());

			int32 errorPos;
			if (!_IsAlphaNumUnderscore(micro, ".", &errorPos)) {
				throw ParseError("invalid character in micro version string",
					word.pos + secondDotPos + 1 + errorPos);
			}
		}

		int32 errorPos;
		if (!_IsAlphaNumUnderscore(minor, "", &errorPos)) {
			throw ParseError("invalid character in minor version string",
				word.pos + firstDotPos + 1 + errorPos);
		}
	}

	int32 errorPos;
	if (!_IsAlphaNumUnderscore(major, "", &errorPos)) {
		throw ParseError("invalid character in major version string",
			word.pos + errorPos);
	}

	value->SetTo(major, minor, micro, preRelease, revision);
}


void
BPackageInfo::Parser::_ParseList(ListElementParser& elementParser,
	bool allowSingleNonListElement)
{
	Token openBracket = _NextToken();
	if (openBracket.type != TOKEN_OPEN_BRACE) {
		if (!allowSingleNonListElement)
			throw ParseError("expected start of list ('{')", openBracket.pos);

		elementParser(openBracket);
		return;
	}

	while (true) {
		Token token = _NextToken();
		if (token.type == TOKEN_CLOSE_BRACE)
			return;

		if (token.type == TOKEN_ITEM_SEPARATOR)
			continue;

		elementParser(token);
	}
}


void
BPackageInfo::Parser::_ParseStringList(BStringList* value,
	bool allowQuotedStrings, bool convertToLowerCase)
{
	struct StringParser : public ListElementParser {
		BStringList* value;
		bool allowQuotedStrings;
		bool convertToLowerCase;

		StringParser(BStringList* value, bool allowQuotedStrings,
			bool convertToLowerCase)
			:
			value(value),
			allowQuotedStrings(allowQuotedStrings),
			convertToLowerCase(convertToLowerCase)
		{
		}

		virtual void operator()(const Token& token)
		{
			if (allowQuotedStrings) {
				if (token.type != TOKEN_QUOTED_STRING
					&& token.type != TOKEN_WORD) {
					throw ParseError("expected quoted-string or word",
						token.pos);
				}
			} else {
				if (token.type != TOKEN_WORD)
					throw ParseError("expected word", token.pos);
			}

			BString element(token.text);
			if (convertToLowerCase)
				element.ToLower();

			value->Add(element);
		}
	} stringParser(value, allowQuotedStrings, convertToLowerCase);

	_ParseList(stringParser, true);
}


uint32
BPackageInfo::Parser::_ParseFlags()
{
	struct FlagParser : public ListElementParser {
		uint32 flags;

		FlagParser()
			:
			flags(0)
		{
		}

		virtual void operator()(const Token& token)
		{
		if (token.type != TOKEN_WORD)
			throw ParseError("expected word (a flag)", token.pos);

		if (token.text.ICompare("approve_license") == 0)
			flags |= B_PACKAGE_FLAG_APPROVE_LICENSE;
		else if (token.text.ICompare("system_package") == 0)
			flags |= B_PACKAGE_FLAG_SYSTEM_PACKAGE;
		else {
				throw ParseError(
					"expected 'approve_license' or 'system_package'",
				token.pos);
		}
	}
	} flagParser;

	_ParseList(flagParser, true);

	return flagParser.flags;
}


void
BPackageInfo::Parser::_ParseResolvableList(
	BObjectList<BPackageResolvable>* value)
{
	struct ResolvableParser : public ListElementParser {
		Parser& parser;
		BObjectList<BPackageResolvable>* value;

		ResolvableParser(Parser& parser_,
			BObjectList<BPackageResolvable>* value_)
			:
			parser(parser_),
			value(value_)
		{
		}

		virtual void operator()(const Token& token)
		{
			if (token.type != TOKEN_WORD) {
				throw ParseError("expected word (a resolvable name)",
					token.pos);
			}

			int32 errorPos;
			if (!_IsValidResolvableName(token.text, &errorPos)) {
				throw ParseError("invalid character in resolvable name",
					token.pos + errorPos);
			}

			// parse version
			BPackageVersion version;
			Token op = parser._NextToken();
			if (op.type == TOKEN_OPERATOR_ASSIGN) {
				parser._ParseVersionValue(&version, true);
			} else if (op.type == TOKEN_ITEM_SEPARATOR
				|| op.type == TOKEN_CLOSE_BRACE) {
				parser._RewindTo(op);
			} else
				throw ParseError("expected '=', comma or '}'", op.pos);

			// parse compatible version
			BPackageVersion compatibleVersion;
			Token compatible = parser._NextToken();
			if (compatible.type == TOKEN_WORD
				&& (compatible.text == "compat"
					|| compatible.text == "compatible")) {
				op = parser._NextToken();
				if (op.type == TOKEN_OPERATOR_GREATER_EQUAL) {
					parser._ParseVersionValue(&compatibleVersion, true);
				} else
					parser._RewindTo(compatible);
			} else
				parser._RewindTo(compatible);

			value->AddItem(new BPackageResolvable(token.text, version,
				compatibleVersion));
		}
	} resolvableParser(*this, value);

	_ParseList(resolvableParser, false);
}


void
BPackageInfo::Parser::_ParseResolvableExprList(
	BObjectList<BPackageResolvableExpression>* value, BString* _basePackage)
{
	struct ResolvableExpressionParser : public ListElementParser {
		Parser& parser;
		BObjectList<BPackageResolvableExpression>* value;
		BString* basePackage;

		ResolvableExpressionParser(Parser& parser,
			BObjectList<BPackageResolvableExpression>* value,
			BString* basePackage)
			:
			parser(parser),
			value(value),
			basePackage(basePackage)
		{
		}

		virtual void operator()(const Token& token)
		{
			if (token.type != TOKEN_WORD) {
				throw ParseError("expected word (a resolvable name)",
					token.pos);
			}

			int32 errorPos;
			if (!_IsValidResolvableName(token.text, &errorPos)) {
				throw ParseError("invalid character in resolvable name",
					token.pos + errorPos);
			}

			BPackageVersion version;
			Token op = parser._NextToken();
			if (op.type == TOKEN_OPERATOR_LESS
				|| op.type == TOKEN_OPERATOR_LESS_EQUAL
				|| op.type == TOKEN_OPERATOR_EQUAL
				|| op.type == TOKEN_OPERATOR_NOT_EQUAL
				|| op.type == TOKEN_OPERATOR_GREATER_EQUAL
				|| op.type == TOKEN_OPERATOR_GREATER) {
				parser._ParseVersionValue(&version, true);

				if (basePackage != NULL) {
					Token base = parser._NextToken();
					if (base.type == TOKEN_WORD && base.text == "base") {
						if (!basePackage->IsEmpty()) {
						throw ParseError(
							"multiple packages marked as base package",
							token.pos);
						}

						*basePackage = token.text;
					} else
						parser._RewindTo(base);
				}
			} else if (op.type == TOKEN_ITEM_SEPARATOR
				|| op.type == TOKEN_CLOSE_BRACE) {
				parser._RewindTo(op);
			} else {
				throw ParseError(
					"expected '<', '<=', '==', '!=', '>=', '>', comma or '}'",
					op.pos);
			}

			BPackageResolvableOperator resolvableOperator
				= (BPackageResolvableOperator)(op.type - TOKEN_OPERATOR_LESS);

			value->AddItem(new BPackageResolvableExpression(token.text,
				resolvableOperator, version));
		}
	} resolvableExpressionParser(*this, value, _basePackage);

	_ParseList(resolvableExpressionParser, false);
}


void
BPackageInfo::Parser::_Parse(BPackageInfo* packageInfo)
{
	bool seen[B_PACKAGE_INFO_ENUM_COUNT];
	for (int i = 0; i < B_PACKAGE_INFO_ENUM_COUNT; ++i)
		seen[i] = false;

	const char* const* names = BPackageInfo::kElementNames;

	while (Token t = _NextToken()) {
		if (t.type == TOKEN_ITEM_SEPARATOR)
			continue;

		if (t.type != TOKEN_WORD)
			throw ParseError("expected word (a variable name)", t.pos);

		BPackageInfoAttributeID attribute = B_PACKAGE_INFO_ENUM_COUNT;
		for (int i = 0; i < B_PACKAGE_INFO_ENUM_COUNT; i++) {
			if (names[i] != NULL && t.text.ICompare(names[i]) == 0) {
				attribute = (BPackageInfoAttributeID)i;
				break;
			}
		}

		if (attribute == B_PACKAGE_INFO_ENUM_COUNT) {
			BString error = BString("unknown attribute \"") << t.text << '"';
			throw ParseError(error, t.pos);
		}

		if (seen[attribute]) {
			BString error = BString(names[attribute]) << " already seen!";
			throw ParseError(error, t.pos);
		}

		switch (attribute) {
			case B_PACKAGE_INFO_NAME:
			{
				BString name;
				const char* namePos;
				_ParseStringValue(&name, &namePos);

				int32 errorPos;
				if (!_IsValidResolvableName(name, &errorPos)) {
					throw ParseError("invalid character in package name",
						namePos + errorPos);
				}

				packageInfo->SetName(name);
				break;
			}

			case B_PACKAGE_INFO_SUMMARY:
			{
				BString summary;
				_ParseStringValue(&summary);
				if (summary.FindFirst('\n') >= 0)
					throw ParseError("the summary contains linebreaks", t.pos);
				packageInfo->SetSummary(summary);
				break;
			}

			case B_PACKAGE_INFO_DESCRIPTION:
				_ParseStringValue(&packageInfo->fDescription);
				break;

			case B_PACKAGE_INFO_VENDOR:
				_ParseStringValue(&packageInfo->fVendor);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				_ParseStringValue(&packageInfo->fPackager);
				break;

			case B_PACKAGE_INFO_BASE_PACKAGE:
				_ParseStringValue(&packageInfo->fBasePackage);
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				_ParseArchitectureValue(&packageInfo->fArchitecture);
				break;

			case B_PACKAGE_INFO_VERSION:
				_ParseVersionValue(&packageInfo->fVersion, false);
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				_ParseStringList(&packageInfo->fCopyrightList);
				break;

			case B_PACKAGE_INFO_LICENSES:
				_ParseStringList(&packageInfo->fLicenseList);
				break;

			case B_PACKAGE_INFO_URLS:
				_ParseStringList(&packageInfo->fURLList);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				_ParseStringList(&packageInfo->fSourceURLList);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				_ParseResolvableList(&packageInfo->fProvidesList);
				break;

			case B_PACKAGE_INFO_REQUIRES:
				packageInfo->fBasePackage.Truncate(0);
				_ParseResolvableExprList(&packageInfo->fRequiresList,
					&packageInfo->fBasePackage);
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				_ParseResolvableExprList(&packageInfo->fSupplementsList);
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				_ParseResolvableExprList(&packageInfo->fConflictsList);
				break;

			case B_PACKAGE_INFO_FRESHENS:
				_ParseResolvableExprList(&packageInfo->fFreshensList);
				break;

			case B_PACKAGE_INFO_REPLACES:
				_ParseStringList(&packageInfo->fReplacesList, false, true);
				break;

			case B_PACKAGE_INFO_FLAGS:
				packageInfo->SetFlags(_ParseFlags());
				break;

			default:
				// can never get here
				break;
		}

		seen[attribute] = true;
	}

	// everything up to and including 'provides' is mandatory
	for (int i = 0; i <= B_PACKAGE_INFO_PROVIDES; ++i) {
		if (!seen[i]) {
			BString error = BString(names[i]) << " is not being set anywhere!";
			throw ParseError(error, fPos);
		}
	}
}


/*static*/ inline bool
BPackageInfo::Parser::_IsAlphaNumUnderscore(const BString& string,
	const char* additionalChars, int32* _errorPos)
{
	return _IsAlphaNumUnderscore(string.String(),
		string.String() + string.Length(), additionalChars, _errorPos);
}


/*static*/ inline bool
BPackageInfo::Parser::_IsAlphaNumUnderscore(const char* string,
	const char* additionalChars, int32* _errorPos)
{
	return _IsAlphaNumUnderscore(string, string + strlen(string),
		additionalChars, _errorPos);
}


/*static*/ bool
BPackageInfo::Parser::_IsAlphaNumUnderscore(const char* start, const char* end,
	const char* additionalChars, int32* _errorPos)
{
	for (const char* c = start; c < end; c++) {
		if (!isalnum(*c) && *c != '_' && strchr(additionalChars, *c) == NULL) {
			if (_errorPos != NULL)
				*_errorPos = c - start;
			return false;
		}
	}

	return true;
}


/*static*/ bool
BPackageInfo::Parser::_IsValidResolvableName(const char* string,
	int32* _errorPos)
{
	for (const char* c = string; *c != '\0'; c++) {
		switch (*c) {
			case '-':
			case '/':
			case '<':
			case '>':
			case '=':
			case '!':
				break;
			default:
				if (!isspace(*c))
					continue;
				break;
		}

		if (_errorPos != NULL)
			*_errorPos = c - string;
		return false;
	}
	return true;
}


const char* const BPackageInfo::kElementNames[B_PACKAGE_INFO_ENUM_COUNT] = {
	"name",
	"summary",
	"description",
	"vendor",
	"packager",
	"architecture",
	"version",
	"copyrights",
	"licenses",
	"provides",
	"requires",
	"supplements",
	"conflicts",
	"freshens",
	"replaces",
	"flags",
	"urls",
	"source-urls",
	"checksum",		// not being parsed, computed externally
	NULL,			// install-path -- not settable via .PackageInfo
};


const char* const
BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_ENUM_COUNT] = {
	"any",
	"x86",
	"x86_gcc2",
	"source",
};


// #pragma mark - StringBuilder


struct BPackageInfo::StringBuilder {
	StringBuilder()
		:
		fData(),
		fError(B_OK),
		fBasePackage()
	{
	}

	status_t Error() const
	{
		return fError;
	}

	status_t GetString(BString& _string) const
	{
		if (fError != B_OK) {
			_string = BString();
			return fError;
		}

		_string.SetTo((const char*)fData.Buffer(), fData.BufferLength());
		return (size_t)_string.Length() == fData.BufferLength()
			? B_OK : B_NO_MEMORY;
	}

	template<typename Value>
	StringBuilder& Write(const char* attribute, Value value)
	{
		if (_IsValueEmpty(value))
			return *this;

		_Write(attribute);
		_Write('\t');
		_WriteValue(value);
		_Write('\n');
		return *this;
	}

	StringBuilder& WriteFlags(const char* attribute, uint32 flags)
	{
		if ((flags & B_PACKAGE_FLAG_APPROVE_LICENSE) == 0
			&& (flags & B_PACKAGE_FLAG_SYSTEM_PACKAGE) == 0) {
			return *this;
		}

		_Write(attribute);
		_Write('\t');

		if ((flags & B_PACKAGE_FLAG_APPROVE_LICENSE) == 0)
			_Write(" approve_license");
		if ((flags & B_PACKAGE_FLAG_SYSTEM_PACKAGE) == 0)
			_Write(" system_package");

		_Write('\n');
		return *this;
	}

	StringBuilder& BeginRequires(const BString& basePackage)
	{
		fBasePackage = basePackage;
		return *this;
	}

	StringBuilder& EndRequires()
	{
		fBasePackage.Truncate(0);
		return *this;
	}

private:
	void _WriteValue(const char* value)
	{
		_WriteMaybeQuoted(value);
	}

	void _WriteValue(const BPackageVersion& value)
	{
		if (fError != B_OK)
			return;

		if (value.InitCheck() != B_OK) {
			fError = B_BAD_VALUE;
			return;
		}

		_Write(value.ToString());
	}

	void _WriteValue(const BStringList& value)
	{
		int32 count = value.CountStrings();
		if (count == 1) {
			_WriteMaybeQuoted(value.StringAt(0));
		} else {
			_Write("{\n", 2);

			int32 count = value.CountStrings();
			for (int32 i = 0; i < count; i++) {
				_Write('\t');
				_WriteMaybeQuoted(value.StringAt(i));
				_Write('\n');
			}

			_Write('}');
		}
	}

	template<typename Value>
	void _WriteValue(const BObjectList<Value>& value)
	{
		// Note: The fBasePackage solution is disgusting, but any attempt of
		// encapsulating the stringification via templates seems to result in
		// an Internal Compiler Error with gcc 2.

		int32 count = value.CountItems();
		if (count == 1) {
			_Write(value.ItemAt(0)->ToString());
			if (!fBasePackage.IsEmpty()
				&& value.ItemAt(0)->Name() == fBasePackage) {
				_Write(" base");
			}
		} else {
			_Write("{\n", 2);

			int32 count = value.CountItems();
			for (int32 i = 0; i < count; i++) {
				_Write('\t');
				_Write(value.ItemAt(i)->ToString());
				if (!fBasePackage.IsEmpty()
					&& value.ItemAt(i)->Name() == fBasePackage) {
					_Write(" base");
				}
				_Write('\n');
			}

			_Write('}');
		}
	}

	static inline bool _IsValueEmpty(const char* value)
	{
		return value[0] == '\0';
	}

	static inline bool _IsValueEmpty(const BPackageVersion& value)
	{
		return false;
	}

	template<typename List>
	static inline bool _IsValueEmpty(const List& value)
	{
		return value.IsEmpty();
	}

	void _WriteMaybeQuoted(const char* data)
	{
		// check whether quoting is needed
		bool needsQuoting = false;
		bool needsEscaping = false;
		for (const char* it = data; *it != '\0'; it++) {
			if (isalnum(*it) || *it == '.' || *it == '-' || *it == '_'
				|| *it == ':' || *it == '+') {
				continue;
			}

			needsQuoting = true;

			if (*it == '\t' || *it == '\n' || *it == '"' || *it == '\\') {
				needsEscaping = true;
				break;
			}
		}

		if (!needsQuoting) {
			_Write(data);
			return;
		}

		// we need quoting
		_Write('"');

		// escape the string, if necessary
		if (needsEscaping) {
			const char* start = data;
			const char* end = data;
			while (*end != '\0') {
				char replacement[2];
				switch (*end) {
					case '\t':
						replacement[1] = 't';
						break;
					case '\n':
						replacement[1] = 'n';
						break;
					case '"':
					case '\\':
						replacement[1] = *end;
						break;
					default:
						end++;
						continue;
				}

				if (start < end)
					_Write(start, end - start);

				replacement[0] = '\\';
				_Write(replacement, 2);
			}
		} else
			_Write(data);

		_Write('"');
	}

	inline void _Write(char data)
	{
		_Write(&data, 1);
	}

	inline void _Write(const char* data)
	{
		_Write(data, strlen(data));
	}

	inline void _Write(const BString& data)
	{
		_Write(data, data.Length());
	}

	void _Write(const void* data, size_t size)
	{
		if (fError == B_OK) {
			ssize_t bytesWritten = fData.Write(data, size);
			if (bytesWritten < 0)
				fError = bytesWritten;
		}
	}

private:
	BMallocIO	fData;
	status_t	fError;
	BString		fBasePackage;
};


// #pragma mark - FieldName


struct BPackageInfo::FieldName {
	FieldName(const char* prefix, const char* suffix)
	{
		size_t prefixLength = strlen(prefix);
		size_t suffixLength = strlen(suffix);
		if (prefixLength + suffixLength >= sizeof(fFieldName)) {
			fFieldName[0] = '\0';
			return;
		}

		memcpy(fFieldName, prefix, prefixLength);
		memcpy(fFieldName + prefixLength, suffix, suffixLength);
		fFieldName[prefixLength + suffixLength] = '\0';
	}

	bool ReplaceSuffix(size_t prefixLength, const char* suffix)
	{
		size_t suffixLength = strlen(suffix);
		if (prefixLength + suffixLength >= sizeof(fFieldName)) {
			fFieldName[0] = '\0';
			return false;
		}

		memcpy(fFieldName + prefixLength, suffix, suffixLength);
		fFieldName[prefixLength + suffixLength] = '\0';
		return true;
	}

	bool IsValid() const
	{
		return fFieldName[0] != '\0';
	}

	operator const char*()
	{
		return fFieldName;
	}

private:
	char	fFieldName[64];
};


// #pragma mark - PackageFileLocation


struct BPackageInfo::PackageFileLocation {
	PackageFileLocation(const char* path)
		:
		fPath(path),
		fFD(-1)
	{
	}

	PackageFileLocation(int fd)
		:
		fPath(NULL),
		fFD(fd)
	{
	}

	const char* Path() const
	{
		return fPath;
	}

	int FD() const
	{
		return fFD;
	}

private:
	const char*	fPath;
	int			fFD;
};


// #pragma mark - BPackageInfo


BPackageInfo::BPackageInfo()
	:
	BArchivable(),
	fFlags(0),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fCopyrightList(5),
	fLicenseList(5),
	fURLList(5),
	fSourceURLList(5),
	fProvidesList(20, true),
	fRequiresList(20, true),
	fSupplementsList(20, true),
	fConflictsList(5, true),
	fFreshensList(5, true),
	fReplacesList(5)
{
}


BPackageInfo::BPackageInfo(BMessage* archive, status_t* _error)
	:
	BArchivable(archive),
	fFlags(0),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fCopyrightList(5),
	fLicenseList(5),
	fURLList(5),
	fSourceURLList(5),
	fProvidesList(20, true),
	fRequiresList(20, true),
	fSupplementsList(20, true),
	fConflictsList(5, true),
	fFreshensList(5, true),
	fReplacesList(5)
{
	status_t error;
	int32 architecture;
	if ((error = archive->FindString("name", &fName)) == B_OK
		&& (error = archive->FindString("summary", &fSummary)) == B_OK
		&& (error = archive->FindString("description", &fDescription)) == B_OK
		&& (error = archive->FindString("vendor", &fVendor)) == B_OK
		&& (error = archive->FindString("packager", &fPackager)) == B_OK
		&& (error = archive->FindString("basePackage", &fBasePackage)) == B_OK
		&& (error = archive->FindUInt32("flags", &fFlags)) == B_OK
		&& (error = archive->FindInt32("architecture", &architecture)) == B_OK
		&& (error = _ExtractVersion(archive, "version", 0, fVersion)) == B_OK
		&& (error = _ExtractStringList(archive, "copyrights", fCopyrightList))
			== B_OK
		&& (error = _ExtractStringList(archive, "licenses", fLicenseList))
			== B_OK
		&& (error = _ExtractStringList(archive, "urls", fURLList)) == B_OK
		&& (error = _ExtractStringList(archive, "source-urls", fSourceURLList))
			== B_OK
		&& (error = _ExtractResolvables(archive, "provides", fProvidesList))
			== B_OK
		&& (error = _ExtractResolvableExpressions(archive, "requires",
			fRequiresList)) == B_OK
		&& (error = _ExtractResolvableExpressions(archive, "supplements",
			fSupplementsList)) == B_OK
		&& (error = _ExtractResolvableExpressions(archive, "conflicts",
			fConflictsList)) == B_OK
		&& (error = _ExtractResolvableExpressions(archive, "freshens",
			fFreshensList)) == B_OK
		&& (error = _ExtractStringList(archive, "replaces", fReplacesList))
			== B_OK
		&& (error = archive->FindString("checksum", &fChecksum)) == B_OK
		&& (error = archive->FindString("install-path", &fInstallPath)) == B_OK) {
		if (architecture >= 0
			|| architecture <= B_PACKAGE_ARCHITECTURE_ENUM_COUNT) {
			fArchitecture = (BPackageArchitecture)architecture;
		} else
			error = B_BAD_DATA;
	}

	if (_error != NULL)
		*_error = error;
}


BPackageInfo::~BPackageInfo()
{
}


status_t
BPackageInfo::ReadFromConfigFile(const BEntry& packageInfoEntry,
	ParseErrorListener* listener)
{
	status_t result = packageInfoEntry.InitCheck();
	if (result != B_OK)
		return result;

	BFile file(&packageInfoEntry, B_READ_ONLY);
	if ((result = file.InitCheck()) != B_OK)
		return result;

	return ReadFromConfigFile(file, listener);
}


status_t
BPackageInfo::ReadFromConfigFile(BFile& packageInfoFile,
	ParseErrorListener* listener)
{
	off_t size;
	status_t result = packageInfoFile.GetSize(&size);
	if (result != B_OK)
		return result;

	BString packageInfoString;
	char* buffer = packageInfoString.LockBuffer(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if ((result = packageInfoFile.Read(buffer, size)) < size) {
		packageInfoString.UnlockBuffer(0);
		return result >= 0 ? B_IO_ERROR : result;
	}

	buffer[size] = '\0';
	packageInfoString.UnlockBuffer(size);

	return ReadFromConfigString(packageInfoString, listener);
}


status_t
BPackageInfo::ReadFromConfigString(const BString& packageInfoString,
	ParseErrorListener* listener)
{
	Clear();

	Parser parser(listener);
	return parser.Parse(packageInfoString, this);
}


status_t
BPackageInfo::ReadFromPackageFile(const char* path)
{
	return _ReadFromPackageFile(PackageFileLocation(path));
}


status_t
BPackageInfo::ReadFromPackageFile(int fd)
{
	return _ReadFromPackageFile(PackageFileLocation(fd));
}


status_t
BPackageInfo::InitCheck() const
{
	if (fName.Length() == 0 || fSummary.Length() == 0
		|| fDescription.Length() == 0 || fVendor.Length() == 0
		|| fPackager.Length() == 0
		|| fArchitecture == B_PACKAGE_ARCHITECTURE_ENUM_COUNT
		|| fVersion.InitCheck() != B_OK
		|| fCopyrightList.IsEmpty() || fLicenseList.IsEmpty()
		|| fProvidesList.IsEmpty())
		return B_NO_INIT;

	return B_OK;
}


const BString&
BPackageInfo::Name() const
{
	return fName;
}


const BString&
BPackageInfo::Summary() const
{
	return fSummary;
}


const BString&
BPackageInfo::Description() const
{
	return fDescription;
}


const BString&
BPackageInfo::Vendor() const
{
	return fVendor;
}


const BString&
BPackageInfo::Packager() const
{
	return fPackager;
}


const BString&
BPackageInfo::BasePackage() const
{
	return fBasePackage;
}


const BString&
BPackageInfo::Checksum() const
{
	return fChecksum;
}


const BString&
BPackageInfo::InstallPath() const
{
	return fInstallPath;
}


uint32
BPackageInfo::Flags() const
{
	return fFlags;
}


BPackageArchitecture
BPackageInfo::Architecture() const
{
	return fArchitecture;
}


const BPackageVersion&
BPackageInfo::Version() const
{
	return fVersion;
}


const BStringList&
BPackageInfo::CopyrightList() const
{
	return fCopyrightList;
}


const BStringList&
BPackageInfo::LicenseList() const
{
	return fLicenseList;
}


const BStringList&
BPackageInfo::URLList() const
{
	return fURLList;
}


const BStringList&
BPackageInfo::SourceURLList() const
{
	return fSourceURLList;
}


const BObjectList<BPackageResolvable>&
BPackageInfo::ProvidesList() const
{
	return fProvidesList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::RequiresList() const
{
	return fRequiresList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::SupplementsList() const
{
	return fSupplementsList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::ConflictsList() const
{
	return fConflictsList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::FreshensList() const
{
	return fFreshensList;
}


const BStringList&
BPackageInfo::ReplacesList() const
{
	return fReplacesList;
}


BString
BPackageInfo::CanonicalFileName() const
{
	if (InitCheck() != B_OK)
		return BString();

	return BString().SetToFormat("%s-%s-%s.hpkg", fName.String(),
		fVersion.ToString().String(), kArchitectureNames[fArchitecture]);
}


void
BPackageInfo::SetName(const BString& name)
{
	fName = name;
	fName.ToLower();
}


void
BPackageInfo::SetSummary(const BString& summary)
{
	fSummary = summary;
}


void
BPackageInfo::SetDescription(const BString& description)
{
	fDescription = description;
}


void
BPackageInfo::SetVendor(const BString& vendor)
{
	fVendor = vendor;
}


void
BPackageInfo::SetPackager(const BString& packager)
{
	fPackager = packager;
}


void
BPackageInfo::SetBasePackage(const BString& basePackage)
{
	fBasePackage = basePackage;
}


void
BPackageInfo::SetChecksum(const BString& checksum)
{
	fChecksum = checksum;
}


void
BPackageInfo::SetInstallPath(const BString& installPath)
{
	fInstallPath = installPath;
}


void
BPackageInfo::SetVersion(const BPackageVersion& version)
{
	fVersion = version;
}


void
BPackageInfo::SetFlags(uint32 flags)
{
	fFlags = flags;
}


void
BPackageInfo::SetArchitecture(BPackageArchitecture architecture)
{
	fArchitecture = architecture;
}


void
BPackageInfo::ClearCopyrightList()
{
	fCopyrightList.MakeEmpty();
}


status_t
BPackageInfo::AddCopyright(const BString& copyright)
{
	return fCopyrightList.Add(copyright) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearLicenseList()
{
	fLicenseList.MakeEmpty();
}


status_t
BPackageInfo::AddLicense(const BString& license)
{
	return fLicenseList.Add(license) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearURLList()
{
	fURLList.MakeEmpty();
}


status_t
BPackageInfo::AddURL(const BString& url)
{
	return fURLList.Add(url) ? B_OK : B_NO_MEMORY;
}


void
BPackageInfo::ClearSourceURLList()
{
	fSourceURLList.MakeEmpty();
}


status_t
BPackageInfo::AddSourceURL(const BString& url)
{
	return fSourceURLList.Add(url) ? B_OK : B_NO_MEMORY;
}


void
BPackageInfo::ClearProvidesList()
{
	fProvidesList.MakeEmpty();
}


status_t
BPackageInfo::AddProvides(const BPackageResolvable& provides)
{
	BPackageResolvable* newProvides
		= new (std::nothrow) BPackageResolvable(provides);
	if (newProvides == NULL)
		return B_NO_MEMORY;

	return fProvidesList.AddItem(newProvides) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearRequiresList()
{
	fRequiresList.MakeEmpty();
}


status_t
BPackageInfo::AddRequires(const BPackageResolvableExpression& requires)
{
	BPackageResolvableExpression* newRequires
		= new (std::nothrow) BPackageResolvableExpression(requires);
	if (newRequires == NULL)
		return B_NO_MEMORY;

	return fRequiresList.AddItem(newRequires) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearSupplementsList()
{
	fSupplementsList.MakeEmpty();
}


status_t
BPackageInfo::AddSupplements(const BPackageResolvableExpression& supplements)
{
	BPackageResolvableExpression* newSupplements
		= new (std::nothrow) BPackageResolvableExpression(supplements);
	if (newSupplements == NULL)
		return B_NO_MEMORY;

	return fSupplementsList.AddItem(newSupplements) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearConflictsList()
{
	fConflictsList.MakeEmpty();
}


status_t
BPackageInfo::AddConflicts(const BPackageResolvableExpression& conflicts)
{
	BPackageResolvableExpression* newConflicts
		= new (std::nothrow) BPackageResolvableExpression(conflicts);
	if (newConflicts == NULL)
		return B_NO_MEMORY;

	return fConflictsList.AddItem(newConflicts) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearFreshensList()
{
	fFreshensList.MakeEmpty();
}


status_t
BPackageInfo::AddFreshens(const BPackageResolvableExpression& freshens)
{
	BPackageResolvableExpression* newFreshens
		= new (std::nothrow) BPackageResolvableExpression(freshens);
	if (newFreshens == NULL)
		return B_NO_MEMORY;

	return fFreshensList.AddItem(newFreshens) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearReplacesList()
{
	fReplacesList.MakeEmpty();
}


status_t
BPackageInfo::AddReplaces(const BString& replaces)
{
	return fReplacesList.Add(BString(replaces).ToLower()) ? B_OK : B_ERROR;
}


void
BPackageInfo::Clear()
{
	fName.Truncate(0);
	fSummary.Truncate(0);
	fDescription.Truncate(0);
	fVendor.Truncate(0);
	fPackager.Truncate(0);
	fBasePackage.Truncate(0);
	fChecksum.Truncate(0);
	fInstallPath.Truncate(0);
	fFlags = 0;
	fArchitecture = B_PACKAGE_ARCHITECTURE_ENUM_COUNT;
	fVersion.Clear();
	fCopyrightList.MakeEmpty();
	fLicenseList.MakeEmpty();
	fURLList.MakeEmpty();
	fSourceURLList.MakeEmpty();
	fRequiresList.MakeEmpty();
	fProvidesList.MakeEmpty();
	fSupplementsList.MakeEmpty();
	fConflictsList.MakeEmpty();
	fFreshensList.MakeEmpty();
	fReplacesList.MakeEmpty();
}


status_t
BPackageInfo::Archive(BMessage* archive, bool deep) const
{
	status_t error = BArchivable::Archive(archive, deep);
	if (error != B_OK)
		return error;

	if ((error = archive->AddString("name", fName)) != B_OK
		|| (error = archive->AddString("summary", fSummary)) != B_OK
		|| (error = archive->AddString("description", fDescription)) != B_OK
		|| (error = archive->AddString("vendor", fVendor)) != B_OK
		|| (error = archive->AddString("packager", fPackager)) != B_OK
		|| (error = archive->AddString("basePackage", fBasePackage)) != B_OK
		|| (error = archive->AddUInt32("flags", fFlags)) != B_OK
		|| (error = archive->AddInt32("architecture", fArchitecture)) != B_OK
		|| (error = _AddVersion(archive, "version", fVersion)) != B_OK
		|| (error = archive->AddStrings("copyrights", fCopyrightList))
			!= B_OK
		|| (error = archive->AddStrings("licenses", fLicenseList)) != B_OK
		|| (error = archive->AddStrings("urls", fURLList)) != B_OK
		|| (error = archive->AddStrings("source-urls", fSourceURLList))
			!= B_OK
		|| (error = _AddResolvables(archive, "provides", fProvidesList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "requires",
			fRequiresList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "supplements",
			fSupplementsList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "conflicts",
			fConflictsList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "freshens",
			fFreshensList)) != B_OK
		|| (error = archive->AddStrings("replaces", fReplacesList)) != B_OK
		|| (error = archive->AddString("checksum", fChecksum)) != B_OK
		|| (error = archive->AddString("install-path", fInstallPath)) != B_OK) {
		return error;
	}

	return B_OK;
}


/*static*/ BArchivable*
BPackageInfo::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BPackageInfo"))
		return new(std::nothrow) BPackageInfo(archive);
	return NULL;
}


status_t
BPackageInfo::GetConfigString(BString& _string) const
{
	return StringBuilder()
		.Write("name", fName)
		.Write("version", fVersion)
		.Write("summary", fSummary)
		.Write("description", fDescription)
		.Write("vendor", fVendor)
		.Write("packager", fPackager)
		.Write("architecture", kArchitectureNames[fArchitecture])
		.Write("copyrights", fCopyrightList)
		.Write("licenses", fLicenseList)
		.Write("urls", fURLList)
		.Write("source-urls", fSourceURLList)
		.Write("provides", fProvidesList)
		.BeginRequires(fBasePackage)
			.Write("requires", fRequiresList)
		.EndRequires()
		.Write("supplements", fSupplementsList)
		.Write("conflicts", fConflictsList)
		.Write("freshens", fFreshensList)
		.Write("replaces", fReplacesList)
		.WriteFlags("flags", fFlags)
		.Write("checksum", fChecksum)
		.GetString(_string);
	// Note: fInstallPath can not be specified via .PackageInfo.
}


BString
BPackageInfo::ToString() const
{
	BString string;
	GetConfigString(string);
	return string;
}


/*static*/ status_t
BPackageInfo::GetArchitectureByName(const BString& name,
	BPackageArchitecture& _architecture)
{
	for (int i = 0; i < B_PACKAGE_ARCHITECTURE_ENUM_COUNT; ++i) {
		if (name.ICompare(kArchitectureNames[i]) == 0) {
			_architecture = (BPackageArchitecture)i;
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}


/*static*/ status_t
BPackageInfo::ParseVersionString(const BString& string, bool revisionIsOptional,
	BPackageVersion& _version, ParseErrorListener* listener)
{
	return Parser(listener).ParseVersion(string, revisionIsOptional, _version);
}


status_t
BPackageInfo::_ReadFromPackageFile(const PackageFileLocation& fileLocation)
{
	BHPKG::BNoErrorOutput errorOutput;

	// try current package file format version
	{
		BHPKG::BPackageReader packageReader(&errorOutput);
		status_t error = fileLocation.Path() != NULL
			? packageReader.Init(fileLocation.Path())
			: packageReader.Init(fileLocation.FD(), false);
		if (error == B_OK) {
			BPackageInfoContentHandler handler(*this);
			return packageReader.ParseContent(&handler);
		}

		if (error != B_MISMATCHED_VALUES)
			return error;
	}

	// try package file format version 1
	BHPKG::V1::BPackageReader packageReader(&errorOutput);
	status_t error = fileLocation.Path() != NULL
		? packageReader.Init(fileLocation.Path())
		: packageReader.Init(fileLocation.FD(), false);
	if (error != B_OK)
		return error;

	BHPKG::V1::BPackageInfoContentHandler handler(*this);
	return packageReader.ParseContent(&handler);
}


/*static*/ status_t
BPackageInfo::_AddVersion(BMessage* archive, const char* field,
	const BPackageVersion& version)
{
	// Storing BPackageVersion::ToString() would be nice, but the corresponding
	// constructor only works for valid versions and we might want to store
	// invalid versions as well.

	// major
	size_t fieldLength = strlen(field);
	FieldName fieldName(field, ":major");
	if (!fieldName.IsValid())
		return B_BAD_VALUE;

	status_t error = archive->AddString(fieldName, version.Major());
	if (error != B_OK)
		return error;

	// minor
	if (!fieldName.ReplaceSuffix(fieldLength, ":minor"))
		return B_BAD_VALUE;

	error = archive->AddString(fieldName, version.Minor());
	if (error != B_OK)
		return error;

	// micro
	if (!fieldName.ReplaceSuffix(fieldLength, ":micro"))
		return B_BAD_VALUE;

	error = archive->AddString(fieldName, version.Micro());
	if (error != B_OK)
		return error;

	// pre-release
	if (!fieldName.ReplaceSuffix(fieldLength, ":pre"))
		return B_BAD_VALUE;

	error = archive->AddString(fieldName, version.PreRelease());
	if (error != B_OK)
		return error;

	// revision
	if (!fieldName.ReplaceSuffix(fieldLength, ":revision"))
		return B_BAD_VALUE;

	return archive->AddUInt32(fieldName, version.Revision());
}


/*static*/ status_t
BPackageInfo::_AddResolvables(BMessage* archive, const char* field,
	const ResolvableList& resolvables)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName typeField(field, ":type");
	FieldName versionField(field, ":version");
	FieldName compatibleVersionField(field, ":compat");

	if (!nameField.IsValid() || !typeField.IsValid() || !versionField.IsValid()
		|| !compatibleVersionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// add fields
	int32 count = resolvables.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BPackageResolvable* resolvable = resolvables.ItemAt(i);
		status_t error;
		if ((error = archive->AddString(nameField, resolvable->Name())) != B_OK
			|| (error = _AddVersion(archive, versionField,
				resolvable->Version())) != B_OK
			|| (error = _AddVersion(archive, compatibleVersionField,
				resolvable->CompatibleVersion())) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_AddResolvableExpressions(BMessage* archive, const char* field,
	const ResolvableExpressionList& expressions)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName operatorField(field, ":operator");
	FieldName versionField(field, ":version");

	if (!nameField.IsValid() || !operatorField.IsValid()
		|| !versionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// add fields
	int32 count = expressions.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BPackageResolvableExpression* expression = expressions.ItemAt(i);
		status_t error;
		if ((error = archive->AddString(nameField, expression->Name())) != B_OK
			|| (error = archive->AddInt32(operatorField,
				expression->Operator())) != B_OK
			|| (error = _AddVersion(archive, versionField,
				expression->Version())) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractVersion(BMessage* archive, const char* field, int32 index,
	BPackageVersion& _version)
{
	// major
	size_t fieldLength = strlen(field);
	FieldName fieldName(field, ":major");
	if (!fieldName.IsValid())
		return B_BAD_VALUE;

	BString major;
	status_t error = archive->FindString(fieldName, index, &major);
	if (error != B_OK)
		return error;

	// minor
	if (!fieldName.ReplaceSuffix(fieldLength, ":minor"))
		return B_BAD_VALUE;

	BString minor;
	error = archive->FindString(fieldName, index, &minor);
	if (error != B_OK)
		return error;

	// micro
	if (!fieldName.ReplaceSuffix(fieldLength, ":micro"))
		return B_BAD_VALUE;

	BString micro;
	error = archive->FindString(fieldName, index, &micro);
	if (error != B_OK)
		return error;

	// pre-release
	if (!fieldName.ReplaceSuffix(fieldLength, ":pre"))
		return B_BAD_VALUE;

	BString preRelease;
	error = archive->FindString(fieldName, index, &preRelease);
	if (error != B_OK)
		return error;

	// revision
	if (!fieldName.ReplaceSuffix(fieldLength, ":revision"))
		return B_BAD_VALUE;

	uint32 revision;
	error = archive->FindUInt32(fieldName, index, &revision);
	if (error != B_OK)
		return error;

	_version.SetTo(major, minor, micro, preRelease, revision);
	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractStringList(BMessage* archive, const char* field,
	BStringList& _list)
{
	status_t error = archive->FindStrings(field, &_list);
	return error == B_NAME_NOT_FOUND ? B_OK : error;
		// If the field doesn't exist, that's OK.
}


/*static*/ status_t
BPackageInfo::_ExtractResolvables(BMessage* archive, const char* field,
	ResolvableList& _resolvables)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName typeField(field, ":type");
	FieldName versionField(field, ":version");
	FieldName compatibleVersionField(field, ":compat");

	if (!nameField.IsValid() || !typeField.IsValid() || !versionField.IsValid()
		|| !compatibleVersionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(nameField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString name;
		status_t error = archive->FindString(nameField, i, &name);
		if (error != B_OK)
			return error;

		BPackageVersion version;
		error = _ExtractVersion(archive, versionField, i, version);
		if (error != B_OK)
			return error;

		BPackageVersion compatibleVersion;
		error = _ExtractVersion(archive, compatibleVersionField, i,
			compatibleVersion);
		if (error != B_OK)
			return error;

		BPackageResolvable* resolvable = new(std::nothrow) BPackageResolvable(
			name, version, compatibleVersion);
		if (resolvable == NULL || !_resolvables.AddItem(resolvable)) {
			delete resolvable;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractResolvableExpressions(BMessage* archive,
	const char* field, ResolvableExpressionList& _expressions)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName operatorField(field, ":operator");
	FieldName versionField(field, ":version");

	if (!nameField.IsValid() || !operatorField.IsValid()
		|| !versionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(nameField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString name;
		status_t error = archive->FindString(nameField, i, &name);
		if (error != B_OK)
			return error;

		int32 operatorType;
		error = archive->FindInt32(operatorField, i, &operatorType);
		if (error != B_OK)
			return error;
		if (operatorType < 0
			|| operatorType > B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT) {
			return B_BAD_DATA;
		}

		BPackageVersion version;
		error = _ExtractVersion(archive, versionField, i, version);
		if (error != B_OK)
			return error;

		BPackageResolvableExpression* expression
			= new(std::nothrow) BPackageResolvableExpression(name,
				(BPackageResolvableOperator)operatorType, version);
		if (expression == NULL || !_expressions.AddItem(expression)) {
			delete expression;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


}	// namespace BPackageKit

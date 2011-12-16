/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageInfo.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <File.h>
#include <Entry.h>


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


/*
 * Parses a ".PackageInfo" file and fills a BPackageInfo object with the
 * package info elements found.
 */
class BPackageInfo::Parser {
public:
								Parser(ParseErrorListener* listener = NULL);

			status_t			Parse(const BString& packageInfoString,
									BPackageInfo* packageInfo);

private:
			struct Token;
			struct ListElementParser;
	friend	struct ListElementParser;

			Token				_NextToken();
			void				_RewindTo(const Token& token);

			void				_ParseStringValue(BString* value);
			uint32				_ParseFlags();
			void				_ParseArchitectureValue(
									BPackageArchitecture* value);
			void				_ParseVersionValue(BPackageVersion* value,
									bool releaseIsOptional);
			void				_ParseList(ListElementParser& elementParser,
									bool allowSingleNonListElement);
			void				_ParseStringList(BStringList* value,
									bool allowQuotedStrings = true,
									bool convertToLowerCase = false);
			void				_ParseResolvableList(
									BObjectList<BPackageResolvable>* value);
			void				_ParseResolvableExprList(
									BObjectList<BPackageResolvableExpression>*
										value);

			void				_Parse(BPackageInfo* packageInfo);

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
				|| *fPos == '_' || *fPos == ':' || *fPos == '+') {
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
BPackageInfo::Parser::_ParseStringValue(BString* value)
{
	Token string = _NextToken();
	if (string.type != TOKEN_QUOTED_STRING && string.type != TOKEN_WORD)
		throw ParseError("expected quoted-string or word", string.pos);

	*value = string.text;
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
	bool releaseIsOptional)
{
	Token word = _NextToken();
	if (word.type != TOKEN_WORD)
		throw ParseError("expected word (a version)", word.pos);

	// get the release number
	uint8 release = 0;
	int32 lastDashPos = word.text.FindLast('-');
	if (lastDashPos >= 0) {
		// Might be either the release number or, if that is optional, a
		// pre-release. The former always is a number, the latter starts with a
		// non-digit.
		if (isdigit(word.text[lastDashPos + 1])) {
			int number = atoi(word.text.String() + lastDashPos + 1);
			if (number <= 0 || number > 99) {
				throw ParseError("release number must be from 1-99",
					word.pos + word.text.Length());
			}
			release = number;
			word.text.Truncate(lastDashPos);
			lastDashPos = word.text.FindLast('-');
		}
	}

	if (release == 0 && !releaseIsOptional) {
		throw ParseError("expected release number (-<number> suffix)",
			word.pos + word.text.Length());
	}

	// get the pre-release string
	BString preRelease;
	if (lastDashPos >= 0) {
		if (isdigit(word.text[lastDashPos + 1])) {
			throw ParseError("pre-release number must not start with a digit",
				word.pos + word.text.Length());
		}

		word.text.CopyInto(preRelease, lastDashPos + 1, word.text.Length());
		word.text.Truncate(lastDashPos);
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

		if (secondDotPos < 0)
			word.text.CopyInto(minor, firstDotPos + 1, word.text.Length());
		else {
			word.text.CopyInto(minor, firstDotPos + 1,
				secondDotPos - (firstDotPos + 1));
			word.text.CopyInto(micro, secondDotPos + 1, word.text.Length());
		}
	}

	value->SetTo(major, minor, micro, preRelease, release);
}


void
BPackageInfo::Parser::_ParseList(ListElementParser& elementParser,
	bool allowSingleNonListElement)
{
	Token openBracket = _NextToken();
	if (openBracket.type != TOKEN_OPEN_BRACE) {
		if (!allowSingleNonListElement)
			throw ParseError("expected start of list ('[')", openBracket.pos);

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

			BPackageResolvableType type = B_PACKAGE_RESOLVABLE_TYPE_DEFAULT;
			int32 colonPos = token.text.FindFirst(':');
			if (colonPos >= 0) {
				BString typeName(token.text, colonPos);
				for (int i = 0; i < B_PACKAGE_RESOLVABLE_TYPE_ENUM_COUNT; ++i) {
					if (typeName.ICompare(BPackageResolvable::kTypeNames[i])
							== 0) {
						type = (BPackageResolvableType)i;
						break;
					}
				}
				if (type == B_PACKAGE_RESOLVABLE_TYPE_DEFAULT) {
					BString error("resolvable type (<type>:) must be one of [");
					for (int i = 1; i < B_PACKAGE_RESOLVABLE_TYPE_ENUM_COUNT;
							++i) {
						if (i > 1)
							error << ",";
						error << BPackageResolvable::kTypeNames[i];
					}
					error << "]";
					throw ParseError(error, token.pos);
				}
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
				throw ParseError("expected '=', comma or ']'", op.pos);

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

			value->AddItem(new BPackageResolvable(token.text, type, version,
				compatibleVersion));
		}
	} resolvableParser(*this, value);

	_ParseList(resolvableParser, false);
}


void
BPackageInfo::Parser::_ParseResolvableExprList(
	BObjectList<BPackageResolvableExpression>* value)
{
	struct ResolvableExpressionParser : public ListElementParser {
		Parser& parser;
		BObjectList<BPackageResolvableExpression>* value;

		ResolvableExpressionParser(Parser& parser_,
			BObjectList<BPackageResolvableExpression>* value_)
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

		BPackageVersion version;
			Token op = parser._NextToken();
		if (op.type == TOKEN_OPERATOR_LESS
			|| op.type == TOKEN_OPERATOR_LESS_EQUAL
			|| op.type == TOKEN_OPERATOR_EQUAL
			|| op.type == TOKEN_OPERATOR_NOT_EQUAL
			|| op.type == TOKEN_OPERATOR_GREATER_EQUAL
			|| op.type == TOKEN_OPERATOR_GREATER) {
				parser._ParseVersionValue(&version, true);
		} else if (op.type == TOKEN_ITEM_SEPARATOR
			|| op.type == TOKEN_CLOSE_BRACE) {
				parser._RewindTo(op);
		} else {
			throw ParseError(
				"expected '<', '<=', '==', '!=', '>=', '>', comma or ']'",
				op.pos);
		}

		BPackageResolvableOperator resolvableOperator
			= (BPackageResolvableOperator)(op.type - TOKEN_OPERATOR_LESS);

			value->AddItem(new BPackageResolvableExpression(token.text,
			resolvableOperator, version));
	}
	} resolvableExpressionParser(*this, value);

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
				_ParseStringValue(&name);
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
				_ParseResolvableExprList(&packageInfo->fRequiresList);
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


const char* BPackageInfo::kElementNames[B_PACKAGE_INFO_ENUM_COUNT] = {
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


const char*
BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_ENUM_COUNT] = {
	"any",
	"x86",
	"x86_gcc2",
};


BPackageInfo::BPackageInfo()
	:
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

}	// namespace BPackageKit

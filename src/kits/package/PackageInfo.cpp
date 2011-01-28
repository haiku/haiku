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
#include <String.h>

#include <NaturalCompare.h>


using BPrivate::NaturalCompare;


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
	TOKEN_OPEN_BRACKET,
	TOKEN_CLOSE_BRACKET,
	TOKEN_COMMA,
	TOKEN_COLON,
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

			Token				_NextToken();
			void				_RewindTo(const Token& token);

			void				_ParseStringValue(BString* value);
			void				_ParseArchitectureValue(
									BPackageArchitecture* value);
			void				_ParseVersionValue(BPackageVersion* value,
									bool releaseIsOptional);
			void				_ParseStringList(BObjectList<BString>* value);
			void				_ParseProvisionList(
									BObjectList<BPackageProvision>* value);
			void				_ParseRequirementList(
									BObjectList<BPackageRequirement>* value);

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
	// eat any whitespace or comments
	bool inComment = false;
	while ((inComment && *fPos != '\0') || isspace(*fPos) || *fPos == '#') {
		if (*fPos == '#')
			inComment = true;
		else if (*fPos == '\n')
			inComment = false;
		fPos++;
	}

	const char* tokenPos = fPos;
	switch (*fPos) {
		case '\0':
			return Token(TOKEN_EOF, fPos);

		case ':':
			fPos++;
			return Token(TOKEN_COLON, tokenPos);

		case ',':
			fPos++;
			return Token(TOKEN_COMMA, tokenPos);

		case '[':
			fPos++;
			return Token(TOKEN_OPEN_BRACKET, tokenPos);

		case ']':
			fPos++;
			return Token(TOKEN_CLOSE_BRACKET, tokenPos);

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
		{
			fPos++;
			const char* start = fPos;
			// anything until the next quote is part of the value
			bool lastWasEscape = false;
			while ((*fPos != '"' || lastWasEscape) && *fPos != '\0') {
				if (lastWasEscape)
					lastWasEscape = false;
				else if (*fPos == '\\')
					lastWasEscape = true;
				fPos++;
			}
			if (*fPos != '"')
				throw ParseError("unterminated quoted-string", tokenPos);
			const char* end = fPos++;
			return Token(TOKEN_QUOTED_STRING, start, end - start);
		}

		default:
		{
			const char* start = fPos;
			while (isalnum(*fPos) || *fPos == '.' || *fPos == '-'
				|| *fPos == '_') {
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

	uint8 release = 0;
	int32 lastDashPos = word.text.FindLast('-');
	if (lastDashPos < 0) {
		if (!releaseIsOptional) {
			throw ParseError("expected release number (-<number> suffix)",
				word.pos + word.text.Length());
		}
	} else {
		int number = atoi(word.text.String() + lastDashPos + 1);
		if (number <= 0 || number > 99) {
			throw ParseError("release number must be from 1-99",
				word.pos + word.text.Length());
		}
		release = number;
		word.text.Truncate(lastDashPos);
	}

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
				secondDotPos - firstDotPos + 1);
			word.text.CopyInto(micro, secondDotPos + 1, word.text.Length());
		}
	}

	value->SetTo(major, minor, micro, release);
}


void
BPackageInfo::Parser::_ParseStringList(BObjectList<BString>* value)
{
	Token openBracket = _NextToken();
	if (openBracket.type != TOKEN_OPEN_BRACKET)
		throw ParseError("expected start of list ('[')", openBracket.pos);

	bool needComma = false;
	while (true) {
		Token token = _NextToken();
		if (token.type == TOKEN_CLOSE_BRACKET)
			return;

		if (needComma) {
			if (token.type != TOKEN_COMMA)
				throw ParseError("expected comma", token.pos);
			token = _NextToken();
		} else
			needComma = true;

		if (token.type != TOKEN_QUOTED_STRING && token.type != TOKEN_WORD)
			throw ParseError("expected quoted-string or word", token.pos);

		value->AddItem(new BString(token.text));
	}
}


void
BPackageInfo::Parser::_ParseProvisionList(BObjectList<BPackageProvision>* value)
{
	Token openBracket = _NextToken();
	if (openBracket.type != TOKEN_OPEN_BRACKET)
		throw ParseError("expected start of list ('[')", openBracket.pos);

	bool needComma = false;
	while (true) {
		Token name = _NextToken();
		if (name.type == TOKEN_CLOSE_BRACKET)
			return;

		if (needComma) {
			if (name.type != TOKEN_COMMA)
				throw ParseError("expected comma", name.pos);
			name = _NextToken();
		} else
			needComma = true;

		if (name.type != TOKEN_WORD)
			throw ParseError("expected word (a provision name)", name.pos);

		BPackageVersion version;
		Token op = _NextToken();
		if (op.type == TOKEN_OPERATOR_ASSIGN)
			_ParseVersionValue(&version, true);
		else if (op.type == TOKEN_COMMA || op.type == TOKEN_CLOSE_BRACKET)
			_RewindTo(op);
		else
			throw ParseError("expected '=', comma or ']'", op.pos);

		value->AddItem(new BPackageProvision(name.text, version));
	}
}


void
BPackageInfo::Parser::_ParseRequirementList(
	BObjectList<BPackageRequirement>* value)
{
	Token openBracket = _NextToken();
	if (openBracket.type != TOKEN_OPEN_BRACKET)
		throw ParseError("expected start of list ('[')", openBracket.pos);

	bool needComma = false;
	while (true) {
		Token name = _NextToken();
		if (name.type == TOKEN_CLOSE_BRACKET)
			return;

		if (needComma) {
			if (name.type != TOKEN_COMMA)
				throw ParseError("expected comma", name.pos);
			name = _NextToken();
		} else
			needComma = true;

		if (name.type != TOKEN_WORD)
			throw ParseError("expected word (a requirement name)", name.pos);

		BPackageVersion version;
		Token op = _NextToken();
		if (op.type == TOKEN_OPERATOR_LESS
			|| op.type == TOKEN_OPERATOR_LESS_EQUAL
			|| op.type == TOKEN_OPERATOR_EQUAL
			|| op.type == TOKEN_OPERATOR_GREATER_EQUAL
			|| op.type == TOKEN_OPERATOR_GREATER)
			_ParseVersionValue(&version, true);
		else if (op.type == TOKEN_COMMA || op.type == TOKEN_CLOSE_BRACKET)
			_RewindTo(op);
		else {
			throw ParseError(
				"expected '<', '<=', '==', '>=', '>', comma or ']'", op.pos);
		}

		value->AddItem(new BPackageRequirement(name.text, op.text, version));
	}
}


void
BPackageInfo::Parser::_Parse(BPackageInfo* packageInfo)
{
	bool seen[B_PACKAGE_INFO_ENUM_COUNT];
	for (int i = 0; i < B_PACKAGE_INFO_ENUM_COUNT; ++i)
		seen[i] = false;

	const char* const* names = BPackageInfo::kElementNames;

	while (Token t = _NextToken()) {
		if (t.type != TOKEN_WORD)
			throw ParseError("expected word (a variable name)", t.pos);

		Token opAssign = _NextToken();
		if (opAssign.type != TOKEN_OPERATOR_ASSIGN) {
			throw ParseError("expected assignment operator ('=')",
				opAssign.pos);
		}

		if (t.text.ICompare(names[B_PACKAGE_INFO_NAME]) == 0) {
			if (seen[B_PACKAGE_INFO_NAME]) {
				BString error = BString(names[B_PACKAGE_INFO_NAME])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BString name;
			_ParseStringValue(&name);
			packageInfo->SetName(name);
			seen[B_PACKAGE_INFO_NAME] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_SUMMARY]) == 0) {
			if (seen[B_PACKAGE_INFO_SUMMARY]) {
				BString error = BString(names[B_PACKAGE_INFO_SUMMARY])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BString summary;
			_ParseStringValue(&summary);
			packageInfo->SetSummary(summary);
			seen[B_PACKAGE_INFO_SUMMARY] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_DESCRIPTION]) == 0) {
			if (seen[B_PACKAGE_INFO_DESCRIPTION]) {
				BString error = BString(names[B_PACKAGE_INFO_DESCRIPTION])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BString description;
			_ParseStringValue(&description);
			packageInfo->SetDescription(description);
			seen[B_PACKAGE_INFO_DESCRIPTION] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_VENDOR]) == 0) {
			if (seen[B_PACKAGE_INFO_VENDOR]) {
				BString error = BString(names[B_PACKAGE_INFO_VENDOR])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BString vendor;
			_ParseStringValue(&vendor);
			packageInfo->SetVendor(vendor);
			seen[B_PACKAGE_INFO_VENDOR] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_PACKAGER]) == 0) {
			if (seen[B_PACKAGE_INFO_PACKAGER]) {
				BString error = BString(names[B_PACKAGE_INFO_PACKAGER])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BString packager;
			_ParseStringValue(&packager);
			packageInfo->SetPackager(packager);
			seen[B_PACKAGE_INFO_PACKAGER] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_ARCHITECTURE]) == 0) {
			if (seen[B_PACKAGE_INFO_ARCHITECTURE]) {
				BString error = BString(names[B_PACKAGE_INFO_ARCHITECTURE])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BPackageArchitecture architecture;
			_ParseArchitectureValue(&architecture);
			packageInfo->SetArchitecture(architecture);
			seen[B_PACKAGE_INFO_ARCHITECTURE] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_VERSION]) == 0) {
			if (seen[B_PACKAGE_INFO_VERSION]) {
				BString error = BString(names[B_PACKAGE_INFO_VERSION])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BPackageVersion version;
			_ParseVersionValue(&version, false);
			packageInfo->SetVersion(version);
			seen[B_PACKAGE_INFO_VERSION] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_COPYRIGHTS]) == 0) {
			if (seen[B_PACKAGE_INFO_COPYRIGHTS]) {
				BString error = BString(names[B_PACKAGE_INFO_COPYRIGHTS])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BObjectList<BString> copyrights;
			_ParseStringList(&copyrights);
			int count = copyrights.CountItems();
			for (int i = 0; i < count; ++i)
				packageInfo->AddCopyright(*(copyrights.ItemAt(i)));
			seen[B_PACKAGE_INFO_COPYRIGHTS] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_LICENSES]) == 0) {
			if (seen[B_PACKAGE_INFO_LICENSES]) {
				BString error = BString(names[B_PACKAGE_INFO_LICENSES])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BObjectList<BString> licenses;
			_ParseStringList(&licenses);
			int count = licenses.CountItems();
			for (int i = 0; i < count; ++i)
				packageInfo->AddLicense(*(licenses.ItemAt(i)));
			seen[B_PACKAGE_INFO_LICENSES] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_PROVIDES]) == 0) {
			if (seen[B_PACKAGE_INFO_PROVIDES]) {
				BString error = BString(names[B_PACKAGE_INFO_PROVIDES])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BObjectList<BPackageProvision> provisions;
			_ParseProvisionList(&provisions);
			int count = provisions.CountItems();
			for (int i = 0; i < count; ++i)
				packageInfo->AddProvision(*(provisions.ItemAt(i)));
			seen[B_PACKAGE_INFO_PROVIDES] = true;
		} else if (t.text.ICompare(names[B_PACKAGE_INFO_REQUIRES]) == 0) {
			if (seen[B_PACKAGE_INFO_REQUIRES]) {
				BString error = BString(names[B_PACKAGE_INFO_REQUIRES])
					<< " already seen!";
				throw ParseError(error, t.pos);
			}

			BObjectList<BPackageRequirement> requirements;
			_ParseRequirementList(&requirements);
			int count = requirements.CountItems();
			for (int i = 0; i < count; ++i)
				packageInfo->AddRequirement(*(requirements.ItemAt(i)));
			seen[B_PACKAGE_INFO_REQUIRES] = true;
		}
	}

	for (int i = 0; i < B_PACKAGE_INFO_ENUM_COUNT; ++i) {
		if (!seen[i]) {
			BString error = BString(names[i]) << " is not being set anywhere!";
			throw ParseError(error, fPos);
		}
	}
}


BPackageVersion::BPackageVersion()
{
}


BPackageVersion::BPackageVersion(const BString& major, const BString& minor,
	const BString& micro, uint8 release)
	:
	fMajor(major),
	fMinor(minor),
	fMicro(micro),
	fRelease(release)
{
}


status_t
BPackageVersion::InitCheck() const
{
	return fMajor.Length() > 0 ? B_OK : B_NO_INIT;
}


int
BPackageVersion::Compare(const BPackageVersion& other) const
{
	int majorDiff = NaturalCompare(fMajor.String(), other.fMajor.String());
	if (majorDiff != 0)
		return majorDiff;

	int minorDiff = NaturalCompare(fMinor.String(), other.fMinor.String());
	if (minorDiff != 0)
		return minorDiff;

	int microDiff = NaturalCompare(fMicro.String(), other.fMicro.String());
	if (microDiff != 0)
		return microDiff;

	return (int)fRelease - (int)other.fRelease;
}


void
BPackageVersion::GetVersionString(BString& string) const
{
	string = fMajor;

	if (fMinor.Length() > 0) {
		string << '.' << fMinor;
		if (fMicro.Length() > 0)
			string << '.' << fMicro;
	}

	if (fRelease > 0)
		string << '-' << fRelease;
}


void
BPackageVersion::SetTo(const BString& major, const BString& minor,
	const BString& micro, uint8 release)
{
	fMajor = major;
	fMinor = minor;
	fMicro = micro;
	fRelease = release;
}


void
BPackageVersion::Clear()
{
	fMajor.Truncate(0);
	fMinor.Truncate(0);
	fMicro.Truncate(0);
	fRelease = 0;
}


BPackageProvision::BPackageProvision()
{
}


BPackageProvision::BPackageProvision(const BString& name,
	const BPackageVersion& version)
	:
	fName(name),
	fVersion(version)
{
}


status_t
BPackageProvision::InitCheck() const
{
	return fName.Length() > 0 ? B_OK : B_NO_INIT;
}


void
BPackageProvision::GetProvisionString(BString& string) const
{
	string = fName;

	if (fVersion.InitCheck() == B_OK) {
		string << '=';
		fVersion.GetVersionString(string);
	}
}


void
BPackageProvision::SetTo(const BString& name, const BPackageVersion& version)
{
	fName = name;
	fVersion = version;
}


void
BPackageProvision::Clear()
{
	fName.Truncate(0);
	fVersion.Clear();
}


BPackageRequirement::BPackageRequirement()
{
}


BPackageRequirement::BPackageRequirement(const BString& name,
	const BString& _operator, const BPackageVersion& version)
	:
	fName(name),
	fOperator(_operator),
	fVersion(version)
{
}


status_t
BPackageRequirement::InitCheck() const
{
	if (fName.Length() == 0)
		return B_NO_INIT;

	// either both or none of operator and version must be set
	if (fOperator.Length() == 0 && fVersion.InitCheck() == B_OK)
		return B_NO_INIT;

	if (fOperator.Length() > 0 && fVersion.InitCheck() != B_OK)
		return B_NO_INIT;

	return B_OK;
}


void
BPackageRequirement::GetRequirementString(BString& string) const
{
	string = fName;

	if (fVersion.InitCheck() == B_OK) {
		string << fOperator;
		fVersion.GetVersionString(string);
	}
}


void
BPackageRequirement::SetTo(const BString& name, const BString& _operator,
	const BPackageVersion& version)
{
	fName = name;
	fOperator = _operator;
	fVersion = version;
}


void
BPackageRequirement::Clear()
{
	fName.Truncate(0);
	fOperator.Truncate(0);
	fVersion.Clear();
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
};


const char*
BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_ENUM_COUNT] = {
	"any",
	"x86",
	"x86_gcc2",
};


BPackageInfo::BPackageInfo()
	:
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fCopyrights(5, true),
	fLicenses(5, true),
	fProvisions(20, true),
	fRequirements(20, true)
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

	off_t size;
	if ((result = file.GetSize(&size)) != B_OK)
		return result;

	BString packageInfoString;
	char* buffer = packageInfoString.LockBuffer(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if ((result = file.Read(buffer, size)) < size) {
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
		|| fCopyrights.IsEmpty() || fLicenses.IsEmpty()
		|| fProvisions.IsEmpty() || fRequirements.IsEmpty())
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


const BObjectList<BString>&
BPackageInfo::Copyrights() const
{
	return fCopyrights;
}


const BObjectList<BString>&
BPackageInfo::Licenses() const
{
	return fLicenses;
}


const BObjectList<BPackageRequirement>&
BPackageInfo::Requirements() const
{
	return fRequirements;
}


const BObjectList<BPackageProvision>&
BPackageInfo::Provisions() const
{
	return fProvisions;
}


void
BPackageInfo::SetName(const BString& name)
{
	fName = name;
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
BPackageInfo::SetVersion(const BPackageVersion& version)
{
	fVersion = version;
}


void
BPackageInfo::SetArchitecture(BPackageArchitecture architecture)
{
	fArchitecture = architecture;
}


void
BPackageInfo::ClearCopyrights()
{
	fCopyrights.MakeEmpty();
}


status_t
BPackageInfo::AddCopyright(const BString& copyright)
{
	BString* newCopyright = new (std::nothrow) BString(copyright);
	if (newCopyright == NULL)
		return B_NO_MEMORY;

	return fCopyrights.AddItem(newCopyright) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearLicenses()
{
	fLicenses.MakeEmpty();
}


status_t
BPackageInfo::AddLicense(const BString& license)
{
	BString* newLicense = new (std::nothrow) BString(license);
	if (newLicense == NULL)
		return B_NO_MEMORY;

	return fLicenses.AddItem(newLicense) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearProvisions()
{
	fProvisions.MakeEmpty();
}


status_t
BPackageInfo::AddProvision(const BPackageProvision& provision)
{
	BPackageProvision* newProvision
		= new (std::nothrow) BPackageProvision(provision);
	if (newProvision == NULL)
		return B_NO_MEMORY;

	return fProvisions.AddItem(newProvision) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearRequirements()
{
	fRequirements.MakeEmpty();
}


status_t
BPackageInfo::AddRequirement(const BPackageRequirement& requirement)
{
	BPackageRequirement* newRequirement
		= new (std::nothrow) BPackageRequirement(requirement);
	if (newRequirement == NULL)
		return B_NO_MEMORY;

	return fRequirements.AddItem(newRequirement) ? B_OK : B_ERROR;
}


void
BPackageInfo::Clear()
{
	fName.Truncate(0);
	fSummary.Truncate(0);
	fDescription.Truncate(0);
	fVendor.Truncate(0);
	fPackager.Truncate(0);
	fVersion.Clear();
	fArchitecture = B_PACKAGE_ARCHITECTURE_ENUM_COUNT;
	fCopyrights.MakeEmpty();
	fLicenses.MakeEmpty();
	fRequirements.MakeEmpty();
	fProvisions.MakeEmpty();
}


/*static*/ status_t
BPackageInfo::GetElementName(BPackageInfoIndex index, const char** name)
{
	if (index < 0 || index >= B_PACKAGE_INFO_ENUM_COUNT || name == NULL)
		return B_BAD_VALUE;

	*name = kElementNames[index];

	return B_OK;
}


/*static*/ status_t
BPackageInfo::GetArchitectureName(BPackageArchitecture arch, const char** name)
{
	if (arch < 0 || arch >= B_PACKAGE_ARCHITECTURE_ENUM_COUNT || name == NULL)
		return B_BAD_VALUE;

	*name = kArchitectureNames[arch];

	return B_OK;
}


}	// namespace BPackageKit

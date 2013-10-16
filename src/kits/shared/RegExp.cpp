/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include <RegExp.h>

#include <new>

#include <regex.h>

#include <String.h>

#include <Referenceable.h>


// #pragma mark - RegExp::Data


struct RegExp::Data : public BReferenceable {
	Data(const char* pattern, PatternType patternType, bool caseSensitive)
		:
		BReferenceable()
	{
		// convert the shell pattern to a regular expression
		BString patternString;
		if (patternType == PATTERN_TYPE_WILDCARD) {
			while (*pattern != '\0') {
				char c = *pattern++;
				switch (c) {
					case '?':
						patternString += '.';
						continue;
					case '*':
						patternString += ".*";
						continue;
					case '[':
					{
						// find the matching ']' first
						const char* end = pattern;
						while (*end != ']') {
							if (*end++ == '\0') {
								fError = REG_EBRACK;
								return;
							}
						}

						if (pattern == end) {
							// Empty bracket expression. It will never match
							// anything. Strictly speaking this is not
							// considered an error, but we handle it like one.
							fError = REG_EBRACK;
							return;
						}

						patternString += '[';

						// We need to avoid "[." ... ".]", "[=" ... "=]", and
						// "[:" ... ":]" sequences, since those have special
						// meaning in regular expressions. If we encounter
						// a '[' followed by either of '.', '=', or ':', we
						// replace the '[' by "[.[.]".
						while (pattern < end) {
							c = *pattern++;
							if (c == '[' && pattern < end) {
								switch (*pattern) {
									case '.':
									case '=':
									case ':':
										patternString += "[.[.]";
										continue;
								}
							}
							patternString += c;
						}

						pattern++;
						patternString += ']';
						break;
					}

					case '\\':
					{
						// Quotes the next character. Works the same way for
						// regular expressions.
						if (*pattern == '\0') {
							fError = REG_EESCAPE;
							return;
						}

						patternString += '\\';
						patternString += *pattern++;
						break;
					}

					case '^':
					case '.':
					case '$':
					case '(':
					case ')':
					case '|':
					case '+':
					case '{':
						// need to be quoted
						patternString += '\\';
						// fall through
					default:
						patternString += c;
						break;
				}
			}

			pattern = patternString.String();
		}

		int flags = REG_EXTENDED;
		if (!caseSensitive)
			flags |= REG_ICASE;

		fError = regcomp(&fCompiledExpression, pattern, flags);
	}

	~Data()
	{
		if (fError == 0)
			regfree(&fCompiledExpression);
	}

	bool IsValid() const
	{
		return fError == 0;
	}

	const regex_t* CompiledExpression() const
	{
		return &fCompiledExpression;
	}

private:
	int		fError;
	regex_t	fCompiledExpression;
};


// #pragma mark - RegExp::MatchResultData


struct RegExp::MatchResultData : public BReferenceable {
	MatchResultData(const regex_t* compiledExpression, const char* string)
		:
		BReferenceable(),
		fMatchCount(0),
		fMatches(NULL)
	{
		// fMatchCount is always set to the number of matching groups in the
		// expression (or 0 if an error occured). Some of the "matches" in
		// the array may still point to the (-1,-1) range if they don't
		// actually match anything.
		fMatchCount = compiledExpression->re_nsub + 1;
		fMatches = new regmatch_t[fMatchCount];
		if (regexec(compiledExpression, string, fMatchCount, fMatches, 0)
				!= 0) {
			delete[] fMatches;
			fMatches = NULL;
			fMatchCount = 0;
		}
	}

	~MatchResultData()
	{
		delete[] fMatches;
	}

	size_t MatchCount() const
	{
		return fMatchCount;
	}

	const regmatch_t* Matches() const
	{
		return fMatches;
	}

private:
	size_t		fMatchCount;
	regmatch_t*	fMatches;
};


// #pragma mark - RegExp


RegExp::RegExp()
	:
	fData(NULL)
{
}


RegExp::RegExp(const char* pattern, PatternType patternType,
	bool caseSensitive)
	:
	fData(NULL)
{
	SetPattern(pattern, patternType, caseSensitive);
}


RegExp::RegExp(const RegExp& other)
	:
	fData(other.fData)
{
	if (fData != NULL)
		fData->AcquireReference();
}


RegExp::~RegExp()
{
	if (fData != NULL)
		fData->ReleaseReference();
}


bool
RegExp::SetPattern(const char* pattern, PatternType patternType,
	bool caseSensitive)
{
	if (fData != NULL) {
		fData->ReleaseReference();
		fData = NULL;
	}

	Data* newData = new(std::nothrow) Data(pattern, patternType, caseSensitive);
	if (newData == NULL)
		return false;

	BReference<Data> dataReference(newData, true);
	if (!newData->IsValid())
		return false;

	fData = dataReference.Detach();
	return true;
}


RegExp::MatchResult
RegExp::Match(const char* string) const
{
	if (!IsValid())
		return MatchResult();

	return MatchResult(
		new(std::nothrow) MatchResultData(fData->CompiledExpression(),
			string));
}


RegExp&
RegExp::operator=(const RegExp& other)
{
	if (fData != NULL)
		fData->ReleaseReference();

	fData = other.fData;

	if (fData != NULL)
		fData->AcquireReference();

	return *this;
}


// #pragma mark - RegExp::MatchResult


RegExp::MatchResult::MatchResult()
	:
	fData(NULL)
{
}


RegExp::MatchResult::MatchResult(MatchResultData* data)
	:
	fData(data)
{
}


RegExp::MatchResult::MatchResult(const MatchResult& other)
	:
	fData(other.fData)
{
	if (fData != NULL)
		fData->AcquireReference();
}


RegExp::MatchResult::~MatchResult()
{
	if (fData != NULL)
		fData->ReleaseReference();
}


bool
RegExp::MatchResult::HasMatched() const
{
	return fData != NULL && fData->MatchCount() > 0;
}


size_t
RegExp::MatchResult::StartOffset() const
{
	return fData != NULL && fData->MatchCount() > 0
		? fData->Matches()[0].rm_so : 0;
}


size_t
RegExp::MatchResult::EndOffset() const
{
	return fData != NULL && fData->MatchCount() > 0
		? fData->Matches()[0].rm_eo : 0;
}


size_t
RegExp::MatchResult::GroupCount() const
{
	if (fData == NULL)
		return 0;

	size_t matchCount = fData->MatchCount();
	return matchCount > 0 ? matchCount - 1 : 0;
}


size_t
RegExp::MatchResult::GroupStartOffsetAt(size_t index) const
{
	return fData != NULL && fData->MatchCount() > index + 1
		? fData->Matches()[index + 1].rm_so : 0;
}


size_t
RegExp::MatchResult::GroupEndOffsetAt(size_t index) const
{
	return fData != NULL && fData->MatchCount() > index + 1
		? fData->Matches()[index + 1].rm_eo : 0;
}


RegExp::MatchResult&
RegExp::MatchResult::operator=(const MatchResult& other)
{
	if (fData != NULL)
		fData->ReleaseReference();

	fData = other.fData;

	if (fData != NULL)
		fData->AcquireReference();

	return *this;
}

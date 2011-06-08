/* Query - query parsing and evaluation
 *
 * The pattern matching is roughly based on code originally written
 * by J. Kercheval, and on code written by Kenneth Almquist, though
 * it shares no code.
 *
 * Copyright 2001-2006, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */

// Adjusted by Ingo Weinhold <bonefish@cs.tu-berlin.de> for usage in RAM FS.


#include "Query.h"
#include "Debug.h"
#include "Directory.h"
#include "Entry.h"
#include "Misc.h"
#include "Node.h"
#include "Volume.h"
#include "Index.h"

#include <SupportDefs.h>
#include <TypeConstants.h>
#include <AppDefs.h>
#include <fs_query.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>


// IndexWrapper

// constructor
IndexWrapper::IndexWrapper(Volume *volume)
	: fVolume(volume),
	  fIndex(NULL)
{
}

// SetTo
status_t
IndexWrapper::SetTo(const char *name)
{
	fIndex = NULL;
	if (fVolume)
		fIndex = fVolume->FindIndex(name);
	return (fIndex ? B_OK : B_ENTRY_NOT_FOUND);
}

// Unset
void
IndexWrapper::Unset()
{
	fIndex = NULL;
}

// Type
uint32
IndexWrapper::Type() const
{
	return (fIndex ? fIndex->GetType() : 0);
}

// GetSize
off_t
IndexWrapper::GetSize() const
{
	// Compute a fake "index size" based on the number of entries
	// (1024 + 16 * entry count), so we don't need to adjust the code using it.
	return 1024LL + (fIndex ? fIndex->CountEntries() : 0) * 16LL;
}

// KeySize
int32
IndexWrapper::KeySize() const
{
	return (fIndex ? fIndex->GetKeyLength() : 0);
}


// IndexIterator

// constructor
IndexIterator::IndexIterator(IndexWrapper *indexWrapper)
	: fIndexWrapper(indexWrapper),
	  fIterator(),
	  fInitialized(false)
{
}

// Find
status_t
IndexIterator::Find(const uint8 *const key, size_t keyLength)
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (fIndexWrapper && fIndexWrapper->fIndex) {
		// TODO: We actually don't want an exact Find() here, but rather a
		// FindClose().
		fInitialized = fIndexWrapper->fIndex->Find(key, keyLength, &fIterator);
		if (fInitialized)
			error = B_OK;
	}
	return error;
}

// Rewind
status_t
IndexIterator::Rewind()
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (fIndexWrapper && fIndexWrapper->fIndex) {
		fInitialized = fIndexWrapper->fIndex->GetIterator(&fIterator);
		if (fInitialized)
			error = B_OK;
	}
	return error;
}

// GetNextEntry
status_t
IndexIterator::GetNextEntry(uint8 *buffer, uint16 *_keyLength,
							size_t /*bufferSize*/, Entry **_entry)
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (fIndexWrapper && fIndexWrapper->fIndex) {
		// init iterator, if not done yet
		if (!fInitialized) {
			fIndexWrapper->fIndex->GetIterator(&fIterator);
			fInitialized = true;
		}

		// get key
		size_t keyLength;
		if (Entry *entry = fIterator.GetCurrent(buffer, &keyLength)) {
			*_keyLength = keyLength;
			*_entry = entry;
			error = B_OK;
		}

		// get next entry
		fIterator.GetNext();
	}
	return error;
}


// compare_integral
template<typename Key>
static inline
int
compare_integral(const Key &a, const Key &b)
{
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	return 0;
}

// compare_keys
static
int
compare_keys(const uint8 *key1, size_t length1, const uint8 *key2,
			 size_t length2, uint32 type)
{
	switch (type) {
		case B_INT32_TYPE:
			return compare_integral(*(int32*)key1, *(int32*)key2);
		case B_UINT32_TYPE:
			return compare_integral(*(uint32*)key1, *(uint32*)key2);
		case B_INT64_TYPE:
			return compare_integral(*(int64*)key1, *(int64*)key2);
		case B_UINT64_TYPE:
			return compare_integral(*(uint64*)key1, *(uint64*)key2);
		case B_FLOAT_TYPE:
			return compare_integral(*(float*)key1, *(float*)key2);
		case B_DOUBLE_TYPE:
			return compare_integral(*(double*)key1, *(double*)key2);
		case B_STRING_TYPE:
		{
			int result = strncmp((const char*)key1, (const char*)key2,
								 min(length1, length2));
			if (result == 0) {
				result = compare_integral(strnlen((const char*)key1, length1),
										  strnlen((const char*)key2, length2));
			}
			return result;
		}
	}
	return -1;
}

// compareKeys
static inline
int
compareKeys(uint32 type, const uint8 *key1, size_t length1, const uint8 *key2,
			size_t length2)
{
	return compare_keys(key1, length1, key2, length2, type);
}





// The parser has a very static design, but it will do what is required.
//
// ParseOr(), ParseAnd(), ParseEquation() are guarantying the operator
// precedence, that is =,!=,>,<,>=,<= .. && .. ||.
// Apparently, the "!" (not) can only be used with brackets.
//
// If you think that there are too few NULL pointer checks in some places
// of the code, just read the beginning of the query constructor.
// The API is not fully available, just the Query and the Expression class
// are.


enum ops {
	OP_NONE,

	OP_AND,
	OP_OR,

	OP_EQUATION,

	OP_EQUAL,
	OP_UNEQUAL,
	OP_GREATER_THAN,
	OP_LESS_THAN,
	OP_GREATER_THAN_OR_EQUAL,
	OP_LESS_THAN_OR_EQUAL,
};

enum match {
	NO_MATCH = 0,
	MATCH_OK = 1,
	
	MATCH_BAD_PATTERN = -2,
	MATCH_INVALID_CHARACTER
};

// return values from isValidPattern()
enum {
	PATTERN_INVALID_ESCAPE = -3,
	PATTERN_INVALID_RANGE,
	PATTERN_INVALID_SET
};

union value {
	int64	Int64;
	uint64	Uint64;
	int32	Int32;
	uint32	Uint32;
	float	Float;
	double	Double;
	char	CString[kMaxIndexKeyLength];
};

// B_MIME_STRING_TYPE is defined in storage/Mime.h, but we
// don't need the whole file here; the type can't change anyway
#ifndef _MIME_H
#	define B_MIME_STRING_TYPE 'MIMS'
#endif

class Term {
	public:
		Term(int8 op) : fOp(op), fParent(NULL) {}
		virtual ~Term() {}

		int8		Op() const { return fOp; }

		void		SetParent(Term *parent) { fParent = parent; }
		Term		*Parent() const { return fParent; }

		virtual status_t Match(Entry *entry, Node* node,
			const char *attribute = NULL, int32 type = 0,
			const uint8 *key = NULL, size_t size = 0) = 0;
		virtual void Complement() = 0;

		virtual void CalculateScore(IndexWrapper &index) = 0;
		virtual int32 Score() const = 0;

		virtual status_t InitCheck() = 0;

		virtual bool NeedsEntry() = 0;

#ifdef DEBUG
		virtual void	PrintToStream() = 0;
#endif

	protected:
		int8	fOp;
		Term	*fParent;
};

// Although an Equation object is quite independent from the volume on which
// the query is run, there are some dependencies that are produced while
// querying:
// The type/size of the value, the score, and if it has an index or not.
// So you could run more than one query on the same volume, but it might return
// wrong values when it runs concurrently on another volume.
// That's not an issue right now, because we run single-threaded and don't use
// queries more than once.

class Equation : public Term {
	public:
		Equation(char **expr);
		virtual ~Equation();

		virtual status_t InitCheck();

		status_t	ParseQuotedString(char **_start, char **_end);
		char		*CopyString(char *start, char *end);

		virtual status_t Match(Entry *entry, Node* node,
			const char *attribute = NULL, int32 type = 0,
			const uint8 *key = NULL, size_t size = 0);
		virtual void Complement();

		status_t	PrepareQuery(Volume *volume, IndexWrapper &index, IndexIterator **iterator,
						bool queryNonIndexed);
		status_t	GetNextMatching(Volume *volume, IndexIterator *iterator,
						struct dirent *dirent, size_t bufferSize);

		virtual void CalculateScore(IndexWrapper &index);
		virtual int32 Score() const { return fScore; }

		virtual bool NeedsEntry();

#ifdef DEBUG
		virtual void PrintToStream();
#endif

	private:
		Equation(const Equation &);
		Equation &operator=(const Equation &);
			// no implementation

		status_t	ConvertValue(type_code type);
		bool		CompareTo(const uint8 *value, uint16 size);
		uint8		*Value() const { return (uint8 *)&fValue; }
		status_t	MatchEmptyString();

		char		*fAttribute;
		char		*fString;
		union value fValue;
		type_code	fType;
		size_t		fSize;
		bool		fIsPattern;

		int32		fScore;
		bool		fHasIndex;
};

class Operator : public Term {
	public:
		Operator(Term *,int8,Term *);
		virtual ~Operator();

		Term		*Left() const { return fLeft; }
		Term		*Right() const { return fRight; }

		virtual status_t Match(Entry *entry, Node* node,
			const char *attribute = NULL, int32 type = 0,
			const uint8 *key = NULL, size_t size = 0);
		virtual void Complement();

		virtual void CalculateScore(IndexWrapper &index);
		virtual int32 Score() const;

		virtual status_t InitCheck();

		virtual bool NeedsEntry();

		//Term		*Copy() const;
#ifdef DEBUG
		virtual void PrintToStream();
#endif

	private:
		Operator(const Operator &);
		Operator &operator=(const Operator &);
			// no implementation

		Term		*fLeft,*fRight;
};


//---------------------------------


void 
skipWhitespace(char **expr, int32 skip = 0)
{
	char *string = (*expr) + skip;
	while (*string == ' ' || *string == '\t') string++;
	*expr = string;
}


void 
skipWhitespaceReverse(char **expr,char *stop)
{
	char *string = *expr;
	while (string > stop && (*string == ' ' || *string == '\t')) string--;
	*expr = string;
}


//	#pragma mark -


uint32
utf8ToUnicode(char **string)
{
	uint8 *bytes = (uint8 *)*string;
	int32 length;
	uint8 mask = 0x1f;

	switch (bytes[0] & 0xf0) {
		case 0xc0:
		case 0xd0:	length = 2; break;
		case 0xe0:	length = 3; break;
		case 0xf0:
			mask = 0x0f;
			length = 4;
			break;
		default:
			// valid 1-byte character
			// and invalid characters
			(*string)++;
			return bytes[0];
	}
	uint32 c = bytes[0] & mask;
	int32 i = 1;
	for (;i < length && (bytes[i] & 0x80) > 0;i++)
		c = (c << 6) | (bytes[i] & 0x3f);

	if (i < length) {
		// invalid character
		(*string)++;
		return (uint32)bytes[0];
	}
	*string += length;
	return c;
}


int32
getFirstPatternSymbol(char *string)
{
	char c;

	for (int32 index = 0;(c = *string++);index++) {
		if (c == '*' || c == '?' || c == '[')
			return index;
	}
	return -1;
}


bool
isPattern(char *string)
{
	return getFirstPatternSymbol(string) >= 0 ? true : false;
}


status_t
isValidPattern(char *pattern)
{
	while (*pattern) {
		switch (*pattern++) {
			case '\\':
				// the escape character must not be at the end of the pattern
				if (!*pattern++)
					return PATTERN_INVALID_ESCAPE;
				break;

			case '[':
				if (pattern[0] == ']' || !pattern[0])
					return PATTERN_INVALID_SET;

				while (*pattern != ']') {
					if (*pattern == '\\' && !*++pattern)
						return PATTERN_INVALID_ESCAPE;

					if (!*pattern)
						return PATTERN_INVALID_SET;

					if (pattern[0] == '-' && pattern[1] == '-')
						return PATTERN_INVALID_RANGE;

					pattern++;
				}
				break;
		}
	}
	return B_OK;
}


/**	Matches the string against the given wildcard pattern.
 *	Returns either MATCH_OK, or NO_MATCH when everything went fine,
 *	or values < 0 (see enum at the top of Query.cpp) if an error
 *	occurs
 */

status_t
matchString(char *pattern, char *string)
{
	while (*pattern) {
		// end of string == valid end of pattern?
		if (!string[0]) {
			while (pattern[0] == '*')
				pattern++;
			return !pattern[0] ? MATCH_OK : NO_MATCH;
		}

		switch (*pattern++) {
			case '?':
			{
				// match exactly one UTF-8 character; we are
				// not interested in the result
				utf8ToUnicode(&string);
				break;
			}

			case '*':
			{
				// compact pattern
				while (true) {
					if (pattern[0] == '?') {
						if (!*++string)
							return NO_MATCH;
					} else if (pattern[0] != '*')
						break;

					pattern++;
				}

				// if the pattern is done, we have matched the string
				if (!pattern[0])
					return MATCH_OK;

				while(true) {
					// we have removed all occurences of '*' and '?'
					if (pattern[0] == string[0]
						|| pattern[0] == '['
						|| pattern[0] == '\\') {
						status_t status = matchString(pattern,string);
						if (status < B_OK || status == MATCH_OK)
							return status;
					}

					// we could be nice here and just jump to the next
					// UTF-8 character - but we wouldn't gain that much
					// and it'd be slower (since we're checking for
					// equality before entering the recursion)
					if (!*++string)
						return NO_MATCH;
				}
				break;
			}

			case '[':
			{
				bool invert = false;
				if (pattern[0] == '^' || pattern[0] == '!') {
					invert = true;
					pattern++;
				}

				if (!pattern[0] || pattern[0] == ']')
					return MATCH_BAD_PATTERN;

				uint32 c = utf8ToUnicode(&string);
				bool matched = false;

				while (pattern[0] != ']') {
					if (!pattern[0])
						return MATCH_BAD_PATTERN;

					if (pattern[0] == '\\')
						pattern++;

					uint32 first = utf8ToUnicode(&pattern);

					// Does this character match, or is this a range?
					if (first == c) {
						matched = true;
						break;
					} else if (pattern[0] == '-' && pattern[1] != ']' && pattern[1]) {
						pattern++;

						if (pattern[0] == '\\') {
							pattern++;
							if (!pattern[0])
								return MATCH_BAD_PATTERN;
						}
						uint32 last = utf8ToUnicode(&pattern);

						if (c >= first && c <= last) {
							matched = true;
							break;
						}
					}
				}

				if (invert)
					matched = !matched;

				if (matched) {
					while (pattern[0] != ']') {
						if (!pattern[0])
							return MATCH_BAD_PATTERN;
						pattern++;
					}
					pattern++;
					break;
				}
				return NO_MATCH;
			}

            case '\\':
				if (!pattern[0])
					return MATCH_BAD_PATTERN;
				// supposed to fall through
			default:
				if (pattern[-1] != string[0])
					return NO_MATCH;
				string++;
				break;
		}
	}

	if (string[0])
		return NO_MATCH;
	
	return MATCH_OK;
}


//	#pragma mark -


Equation::Equation(char **expr)
	: Term(OP_EQUATION),
	fAttribute(NULL),
	fString(NULL),
	fType(0),
	fIsPattern(false)
{
	char *string = *expr;
	char *start = string;
	char *end = NULL;

	// Since the equation is the integral part of any query, we're just parsing
	// the whole thing here.
	// The whitespace at the start is already removed in Expression::ParseEquation()

	if (*start == '"' || *start == '\'') {
		// string is quoted (start has to be on the beginning of a string)
		if (ParseQuotedString(&start, &end) < B_OK)
			return;

		// set string to a valid start of the equation symbol
		string = end + 2;
		skipWhitespace(&string);
		if (*string != '=' && *string != '<' && *string != '>' && *string != '!') {
			*expr = string;
			return;
		}
	} else {
		// search the (in)equation for the actual equation symbol (and for other operators
		// in case the equation is malformed)
		while (*string && *string != '=' && *string != '<' && *string != '>' && *string != '!'
			&& *string != '&' && *string != '|')
			string++;

		// get the attribute string	(and trim whitespace), in case
		// the string was not quoted
		end = string - 1;
		skipWhitespaceReverse(&end, start);
	}

	// attribute string is empty (which is not allowed)
	if (start > end)
		return;
		
	// at this point, "start" points to the beginning of the string, "end" points
	// to the last character of the string, and "string" points to the first
	// character of the equation symbol

	// test for the right symbol (as this doesn't need any memory)
	switch (*string) {
		case '=':
			fOp = OP_EQUAL;
			break;
		case '>':
			fOp = *(string + 1) == '=' ? OP_GREATER_THAN_OR_EQUAL : OP_GREATER_THAN;
			break;
		case '<':
			fOp = *(string + 1) == '=' ? OP_LESS_THAN_OR_EQUAL : OP_LESS_THAN;
			break;
		case '!':
			if (*(string + 1) != '=')
				return;
			fOp = OP_UNEQUAL;
			break;
		
		// any invalid characters will be rejected
		default:
			*expr = string;
			return;
	}
	// lets change "start" to point to the first character after the symbol
	if (*(string + 1) == '=')
		string++;
	string++;
	skipWhitespace(&string);

	// allocate & copy the attribute string

	fAttribute = CopyString(start, end);
	if (fAttribute == NULL)
		return;

	start = string;
	if (*start == '"' || *start == '\'') {
		// string is quoted (start has to be on the beginning of a string)
		if (ParseQuotedString(&start, &end) < B_OK)
			return;
		
		string = end + 2;
		skipWhitespace(&string);
	} else {
		while (*string && *string != '&' && *string != '|' && *string != ')')
			string++;

		end = string - 1;
		skipWhitespaceReverse(&end, start);
	}
	
	// at this point, "start" will point to the first character of the value,
	// "end" will point to its last character, and "start" to the first non-
	// whitespace character after the value string

	fString = CopyString(start, end);
	if (fString == NULL)
		return;

	// patterns are only allowed for these operations (and strings)
	if (fOp == OP_EQUAL || fOp == OP_UNEQUAL) {
		fIsPattern = isPattern(fString);
		if (fIsPattern && isValidPattern(fString) < B_OK) {
			// we only want to have valid patterns; setting fString
			// to NULL will cause InitCheck() to fail
			free(fString);
			fString = NULL;
		}
	}

	*expr = string;
}


Equation::~Equation()
{
	if (fAttribute != NULL)
		free(fAttribute);
	if (fString != NULL)
		free(fString);
}


status_t 
Equation::InitCheck()
{
	if (fAttribute == NULL
		|| fString == NULL
		|| fOp == OP_NONE)
		return B_BAD_VALUE;

	return B_OK;
}


status_t 
Equation::ParseQuotedString(char **_start, char **_end)
{
	char *start = *_start;
	char quote = *start++;
	char *end = start;
	
	for (;*end && *end != quote;end++) {
		if (*end == '\\')
			end++;
	}
	if (*end == '\0')
		return B_BAD_VALUE;

	*_start = start;
	*_end = end - 1;

	return B_OK;
}


char *
Equation::CopyString(char *start, char *end)
{
	// end points to the last character of the string - and the length
	// also has to include the null-termination
	int32 length = end + 2 - start;
	// just to make sure; since that's the max. attribute name length and
	// the max. string in an index, it make sense to have it that way
	if (length > (int32)kMaxIndexKeyLength || length <= 0)
		return NULL;

	char *copy = (char *)malloc(length);
	if (copy == NULL)
		return NULL;

	memcpy(copy,start,length - 1);
	copy[length - 1] = '\0';

	return copy;
}


status_t 
Equation::ConvertValue(type_code type)
{
	// Has the type already been converted?
	if (type == fType)
		return B_OK;

	char *string = fString;

	switch (type) {
		case B_MIME_STRING_TYPE:
			type = B_STRING_TYPE;
			// supposed to fall through
		case B_STRING_TYPE:
			strncpy(fValue.CString, string, kMaxIndexKeyLength);
			fValue.CString[kMaxIndexKeyLength - 1] = '\0';
			fSize = strlen(fValue.CString);
			break;
		case B_INT32_TYPE:
			fValue.Int32 = strtol(string, &string, 0);
			fSize = sizeof(int32);
			break;
		case B_UINT32_TYPE:
			fValue.Int32 = strtoul(string, &string, 0);
			fSize = sizeof(uint32);
			break;
		case B_INT64_TYPE:
			fValue.Int64 = strtoll(string, &string, 0);
			fSize = sizeof(int64);
			break;
		case B_UINT64_TYPE:
			fValue.Uint64 = strtoull(string, &string, 0);
			fSize = sizeof(uint64);
			break;
		case B_FLOAT_TYPE:
			fValue.Float = strtod(string, &string);
			fSize = sizeof(float);
			break;
		case B_DOUBLE_TYPE:
			fValue.Double = strtod(string, &string);
			fSize = sizeof(double);
			break;
		default:
			FATAL(("query value conversion to 0x%lx requested!\n", type));
			// should we fail here or just do a safety int32 conversion?
			return B_ERROR;
	}

	fType = type;

	// patterns are only allowed for string types
	if (fType != B_STRING_TYPE && fIsPattern)
		fIsPattern = false;

	return B_OK;
}


/**	Returns true when the key matches the equation. You have to
 *	call ConvertValue() before this one.
 */

bool
Equation::CompareTo(const uint8 *value, uint16 size)
{
	int32 compare;

	// fIsPattern is only true if it's a string type, and fOp OP_EQUAL, or OP_UNEQUAL
	if (fIsPattern) {
		// we have already validated the pattern, so we don't check for failing
		// here - if something is broken, and matchString() returns an error,
		// we just don't match
		compare = matchString(fValue.CString, (char *)value) == MATCH_OK ? 0 : 1;
	} else
		compare = compareKeys(fType, value, size, Value(), fSize);

	switch (fOp) {
		case OP_EQUAL:
			return compare == 0;
		case OP_UNEQUAL:
			return compare != 0;
		case OP_LESS_THAN:
			return compare < 0;
		case OP_LESS_THAN_OR_EQUAL:
			return compare <= 0;
		case OP_GREATER_THAN:
			return compare > 0;
		case OP_GREATER_THAN_OR_EQUAL:
			return compare >= 0;
	}
	FATAL(("Unknown/Unsupported operation: %d\n", fOp));
	return false;
}


void 
Equation::Complement()
{
	D(if (fOp <= OP_EQUATION || fOp > OP_LESS_THAN_OR_EQUAL) {
		FATAL(("op out of range!"));
		return;
	});

	int8 complementOp[] = {OP_UNEQUAL, OP_EQUAL, OP_LESS_THAN_OR_EQUAL,
			OP_GREATER_THAN_OR_EQUAL, OP_LESS_THAN, OP_GREATER_THAN};
	fOp = complementOp[fOp - OP_EQUAL];
}


status_t
Equation::MatchEmptyString()
{
	// there is no matching attribute, we will just bail out if we
	// already know that our value is not of a string type.
	// If not, it will be converted to a string - and then be compared with "".
	// That's why we have to call ConvertValue() here - but it will be
	// a cheap call for the next time
	// Should we do this only for OP_UNEQUAL?
	if (fType != 0 && fType != B_STRING_TYPE)
		return NO_MATCH;

	status_t status = ConvertValue(B_STRING_TYPE);
	if (status == B_OK)
		status = CompareTo((const uint8 *)"", fSize) ? MATCH_OK : NO_MATCH;

	return status;
}


/**	Matches the inode's attribute value with the equation.
 *	Returns MATCH_OK if it matches, NO_MATCH if not, < 0 if something went wrong
 */

status_t
Equation::Match(Entry *entry, Node* node, const char *attributeName, int32 type,
	const uint8 *key, size_t size)
{
	// get a pointer to the attribute in question
	union value value;
	const uint8 *buffer;

	// first, check if we are matching for a live query and use that value
	if (attributeName != NULL && !strcmp(fAttribute, attributeName)) {
		if (key == NULL) {
			if (type == B_STRING_TYPE) {
				// special case: a NULL "name" means the entry has been removed
				// or not yet been added -- we refuse to match, whatever the
				// pattern
				if (!strcmp(fAttribute, "name"))
					return NO_MATCH;

				return MatchEmptyString();
			}

			return NO_MATCH;
		}
		buffer = const_cast<uint8 *>(key);
	} else if (!strcmp(fAttribute, "name")) {
		// if not, check for "fake" attributes, "name", "size", "last_modified",
		if (!entry)
			return B_ERROR;
		buffer = (uint8 *)entry->GetName();
		if (buffer == NULL)
			return B_ERROR;

		type = B_STRING_TYPE;
		size = strlen((const char *)buffer);
	} else if (!strcmp(fAttribute,"size")) {
		value.Int64 = node->GetSize();
		buffer = (uint8 *)&value;
		type = B_INT64_TYPE;
	} else if (!strcmp(fAttribute,"last_modified")) {
		value.Int32 = node->GetMTime();
		buffer = (uint8 *)&value;
		type = B_INT32_TYPE;
	} else {
		// then for attributes
		Attribute *attribute = NULL;

		if (node->FindAttribute(fAttribute, &attribute) == B_OK) {
			attribute->GetKey(&buffer, &size);
			type = attribute->GetType();
		} else
			return MatchEmptyString();
	}
	// prepare own value for use, if it is possible to convert it
	status_t status = ConvertValue(type);
	if (status == B_OK)
		status = CompareTo(buffer, size) ? MATCH_OK : NO_MATCH;

	RETURN_ERROR(status);
}


void 
Equation::CalculateScore(IndexWrapper &index)
{
	// As always, these values could be tuned and refined.
	// And the code could also need some real world testing :-)

	// do we have to operate on a "foreign" index?
	if (fOp == OP_UNEQUAL || index.SetTo(fAttribute) < B_OK) {
		fScore = 0;
		return;
	}

	// if we have a pattern, how much does it help our search?
	if (fIsPattern)
		fScore = getFirstPatternSymbol(fString) << 3;
	else {
		// Score by operator
		if (fOp == OP_EQUAL)
			// higher than pattern="255 chars+*"
			fScore = 2048;
		else
			// the pattern search is regarded cheaper when you have at
			// least one character to set your index to
			fScore = 5;
	}

	// take index size into account (1024 is the current node size
	// in our B+trees)
	// 2048 * 2048 == 4194304 is the maximum score (for an empty
	// tree, since the header + 1 node are already 2048 bytes)
	fScore = fScore * ((2048 * 1024LL) / index.GetSize());
}


status_t
Equation::PrepareQuery(Volume */*volume*/, IndexWrapper &index, IndexIterator **iterator, bool queryNonIndexed)
{
	status_t status = index.SetTo(fAttribute);
	
	// if we should query attributes without an index, we can just proceed here
	if (status < B_OK && !queryNonIndexed)
		return B_ENTRY_NOT_FOUND;

	type_code type;

	// special case for OP_UNEQUAL - it will always operate through the whole index
	// but we need the call to the original index to get the correct type
	if (status < B_OK || fOp == OP_UNEQUAL) {
		// Try to get an index that holds all files (name)
		// Also sets the default type for all attributes without index
		// to string.
		type = status < B_OK ? B_STRING_TYPE : index.Type();

		if (index.SetTo("name") < B_OK)
			return B_ENTRY_NOT_FOUND;

		fHasIndex = false;
	} else {
		fHasIndex = true;
		type = index.Type();
	}

	if (ConvertValue(type) < B_OK)
		return B_BAD_VALUE;

	*iterator = new IndexIterator(&index);
	if (*iterator == NULL)
		return B_NO_MEMORY;

	if ((fOp == OP_EQUAL || fOp == OP_GREATER_THAN || fOp == OP_GREATER_THAN_OR_EQUAL
		|| fIsPattern)
		&& fHasIndex) {
		// set iterator to the exact position

		int32 keySize = index.KeySize();

		// at this point, fIsPattern is only true if it's a string type, and fOp
		// is either OP_EQUAL or OP_UNEQUAL
		if (fIsPattern) {
			// let's see if we can use the beginning of the key for positioning
			// the iterator and adjust the key size; if not, just leave the
			// iterator at the start and return success
			keySize = getFirstPatternSymbol(fString);
			if (keySize <= 0)
				return B_OK;
		}

		if (keySize == 0) {
			// B_STRING_TYPE doesn't have a fixed length, so it was set
			// to 0 before - we compute the correct value here
			if (fType == B_STRING_TYPE) {
				keySize = strlen(fValue.CString);

				// The empty string is a special case - we normally don't check
				// for the trailing null byte, in the case for the empty string
				// we do it explicitly, because there can't be keys in the B+tree
				// with a length of zero
				if (keySize == 0)
					keySize = 1;
			} else
				RETURN_ERROR(B_ENTRY_NOT_FOUND);
		}

		status = (*iterator)->Find(Value(), keySize);
		if (fOp == OP_EQUAL && !fIsPattern)
			return status;
		else if (status == B_ENTRY_NOT_FOUND
			&& (fIsPattern || fOp == OP_GREATER_THAN
				|| fOp == OP_GREATER_THAN_OR_EQUAL)) {
			return (*iterator)->Rewind();
		}

		RETURN_ERROR(status);
	}

	return B_OK;
}


status_t 
Equation::GetNextMatching(Volume *volume, IndexIterator *iterator,
	struct dirent *dirent, size_t bufferSize)
{
	while (true) {
		union value indexValue;
		uint16 keyLength;
		Entry *entry = NULL;

		status_t status = iterator->GetNextEntry((uint8*)&indexValue, &keyLength,
			(uint16)sizeof(indexValue), &entry);
		if (status < B_OK)
			return status;

		// only compare against the index entry when this is the correct
		// index for the equation
		if (fHasIndex && !CompareTo((uint8 *)&indexValue, keyLength)) {
			// They aren't equal? let the operation decide what to do
			// Since we always start at the beginning of the index (or the correct
			// position), only some needs to be stopped if the entry doesn't fit.
			if (fOp == OP_LESS_THAN
				|| fOp == OP_LESS_THAN_OR_EQUAL
				|| (fOp == OP_EQUAL && !fIsPattern))
				return B_ENTRY_NOT_FOUND;

			continue;
		}

		// ToDo: check user permissions here - but which one?!
		// we could filter out all those where we don't have
		// read access... (we should check for every parent
		// directory if the X_OK is allowed)
		// Although it's quite expensive to open all parents,
		// it's likely that the application that runs the
		// query will do something similar (and we don't have
		// to do it for root, either).

		// go up in the tree until a &&-operator is found, and check if the
		// inode matches with the rest of the expression - we don't have to
		// check ||-operators for that
		Term *term = this;
		status = MATCH_OK;

		if (!fHasIndex)
			status = Match(entry, entry->GetNode());

		while (term != NULL && status == MATCH_OK) {
			Operator *parent = (Operator *)term->Parent();
			if (parent == NULL)
				break;

			if (parent->Op() == OP_AND) {
				// choose the other child of the parent
				Term *other = parent->Right();
				if (other == term)
					other = parent->Left();

				if (other == NULL) {
					FATAL(("&&-operator has only one child... (parent = %p)\n", parent));
					break;
				}
				status = other->Match(entry, entry->GetNode());
				if (status < 0) {
					REPORT_ERROR(status);
					status = NO_MATCH;
				}
			}
			term = (Term *)parent;
		}

		if (status == MATCH_OK) {
			size_t nameLen = strlen(entry->GetName());

			// check, whether the entry fits into the buffer,
			// and fill it in
			size_t length = (dirent->d_name + nameLen + 1) - (char*)dirent;
			if (length > bufferSize)
				RETURN_ERROR(B_BUFFER_OVERFLOW);

			dirent->d_dev = volume->GetID();
			dirent->d_ino = entry->GetNode()->GetID();
			dirent->d_pdev = volume->GetID();
			dirent->d_pino = entry->GetParent()->GetID();

			memcpy(dirent->d_name, entry->GetName(), nameLen);
			dirent->d_name[nameLen] = '\0';
			dirent->d_reclen = length;
		}

		if (status == MATCH_OK)
			return B_OK;
	}
	RETURN_ERROR(B_ERROR);
}


bool
Equation::NeedsEntry()
{
	return strcmp(fAttribute, "name") == 0;
}


//	#pragma mark -


Operator::Operator(Term *left, int8 op, Term *right)
	: Term(op),
	fLeft(left),
	fRight(right)
{
	if (left)
		left->SetParent(this);
	if (right)
		right->SetParent(this);
}


Operator::~Operator()
{
	delete fLeft;
	delete fRight;
}


status_t
Operator::Match(Entry *entry, Node* node, const char *attribute,
	int32 type, const uint8 *key, size_t size)
{
	if (fOp == OP_AND) {
		status_t status = fLeft->Match(entry, node, attribute, type, key, size);
		if (status != MATCH_OK)
			return status;

		return fRight->Match(entry, node, attribute, type, key, size);
	} else {
		// choose the term with the better score for OP_OR
		if (fRight->Score() > fLeft->Score()) {
			status_t status = fRight->Match(entry, node, attribute, type, key,
				size);
			if (status != NO_MATCH)
				return status;
		}
		return fLeft->Match(entry, node, attribute, type, key, size);
	}
}


void 
Operator::Complement()
{
	if (fOp == OP_AND)
		fOp = OP_OR;
	else
		fOp = OP_AND;
	
	fLeft->Complement();
	fRight->Complement();
}


void 
Operator::CalculateScore(IndexWrapper &index)
{
	fLeft->CalculateScore(index);
	fRight->CalculateScore(index);
}


int32 
Operator::Score() const
{
	if (fOp == OP_AND) {
		// return the one with the better score
		if (fRight->Score() > fLeft->Score())
			return fRight->Score();
		
		return fLeft->Score();
	}
	
	// for OP_OR, be honest, and return the one with the worse score
	if (fRight->Score() < fLeft->Score())
		return fRight->Score();
	
	return fLeft->Score();
}


status_t 
Operator::InitCheck()
{
	if ((fOp != OP_AND && fOp != OP_OR)
		|| fLeft == NULL || fLeft->InitCheck() < B_OK
		|| fRight == NULL || fRight->InitCheck() < B_OK)
		return B_ERROR;

	return B_OK;
}


bool
Operator::NeedsEntry()
{
	return ((fLeft && fLeft->NeedsEntry()) || (fRight && fRight->NeedsEntry()));
}


#if 0
Term *
Operator::Copy() const
{
	if (fEquation != NULL) {
		Equation *equation = new Equation(*fEquation);
		if (equation == NULL)
			return NULL;

		Term *term = new Term(equation);
		if (term == NULL)
			delete equation;
		
		return term;
	}

	Term *left = NULL, *right = NULL;

	if (fLeft != NULL && (left = fLeft->Copy()) == NULL)
		return NULL;
	if (fRight != NULL && (right = fRight->Copy()) == NULL) {
		delete left;
		return NULL;
	}

	Term *term = new Term(left,fOp,right);
	if (term == NULL) {
		delete left;
		delete right;
		return NULL;
	}
	return term;
}
#endif


//	#pragma mark -

#ifdef DEBUG
void
Operator::PrintToStream()
{
	D(__out("( "));
	if (fLeft != NULL)
		fLeft->PrintToStream();

	const char* op;
	switch (fOp) {
		case OP_OR: op = "OR"; break;
		case OP_AND: op = "AND"; break;
		default: op = "?"; break;
	}
	D(__out(" %s ", op));

	if (fRight != NULL)
		fRight->PrintToStream();

	D(__out(" )"));
}


void
Equation::PrintToStream()
{
	const char* op;
	switch (fOp) {
		case OP_EQUAL: op = "=="; break;
		case OP_UNEQUAL: op = "!="; break;
		case OP_GREATER_THAN: op = ">"; break;
		case OP_GREATER_THAN_OR_EQUAL: op = ">="; break;
		case OP_LESS_THAN: op = "<"; break;
		case OP_LESS_THAN_OR_EQUAL: op = "<="; break;
		default: op = "???"; break;
	}
	D(__out("[\"%s\" %s \"%s\"]", fAttribute, op, fString));
}


#endif	/* DEBUG */

//	#pragma mark -


Expression::Expression(char *expr)
{
	if (expr == NULL)
		return;
	
	fTerm = ParseOr(&expr);
	if (fTerm != NULL && fTerm->InitCheck() < B_OK) {
		FATAL(("Corrupt tree in expression!\n"));
		delete fTerm;
		fTerm = NULL;
	}
	D(if (fTerm != NULL) {
		fTerm->PrintToStream();
		D(__out("\n"));
		if (*expr != '\0')
			PRINT(("Unexpected end of string: \"%s\"!\n", expr));
	});
	fPosition = expr;
}


Expression::~Expression()
{
	delete fTerm;
}


Term *
Expression::ParseEquation(char **expr)
{
	skipWhitespace(expr);

	bool nott = false;	// note: not is a C++ keyword
	if (**expr == '!') {
		skipWhitespace(expr, 1);
		if (**expr != '(')
			return NULL;
		
		nott = true;
	}

	if (**expr == ')') {
		// shouldn't be handled here
		return NULL;
	} else if (**expr == '(') {
		skipWhitespace(expr, 1);
		
		Term *term = ParseOr(expr);
		
		skipWhitespace(expr);
		
		if (**expr != ')') {
			delete term;
			return NULL;
		}
		
		// If the term is negated, we just complement the tree, to get
		// rid of the not, a.k.a. DeMorgan's Law.
		if (nott)
			term->Complement();

		skipWhitespace(expr, 1);

		return term;
	}

	Equation *equation = new Equation(expr);
	if (equation == NULL || equation->InitCheck() < B_OK) {
		delete equation;
		return NULL;
	}
	return equation;
}


Term *
Expression::ParseAnd(char **expr)
{
	Term *left = ParseEquation(expr);
	if (left == NULL)
		return NULL;

	while (IsOperator(expr,'&')) {
		Term *right = ParseAnd(expr);
		Term *newParent = NULL;

		if (right == NULL || (newParent = new Operator(left, OP_AND, right)) == NULL) {
			delete left;
			delete right;

			return NULL;
		}
		left = newParent;
	}

	return left;
}


Term *
Expression::ParseOr(char **expr)
{
	Term *left = ParseAnd(expr);
	if (left == NULL)
		return NULL;

	while (IsOperator(expr,'|')) {
		Term *right = ParseAnd(expr);
		Term *newParent = NULL;

		if (right == NULL || (newParent = new Operator(left, OP_OR, right)) == NULL) {
			delete left;
			delete right;

			return NULL;
		}
		left = newParent;
	}

	return left;
}


bool 
Expression::IsOperator(char **expr, char op)
{
	char *string = *expr;
	
	if (*string == op && *(string + 1) == op) {
		*expr += 2;
		return true;
	}
	return false;
}


status_t 
Expression::InitCheck()
{
	if (fTerm == NULL)
		return B_BAD_VALUE;

	return B_OK;
}


//	#pragma mark -


Query::Query(Volume *volume, Expression *expression, uint32 flags)
	:
	fVolume(volume),
	fExpression(expression),
	fCurrent(NULL),
	fIterator(NULL),
	fIndex(volume),
	fFlags(flags),
	fPort(-1),
	fNeedsEntry(false)
{
	// if the expression has a valid root pointer, the whole tree has
	// already passed the sanity check, so that we don't have to check
	// every pointer
	if (volume == NULL || expression == NULL || expression->Root() == NULL)
		return;

	// create index on the stack and delete it afterwards
	fExpression->Root()->CalculateScore(fIndex);
	fIndex.Unset();

	fNeedsEntry = fExpression->Root()->NeedsEntry();

	Rewind();

	if (fFlags & B_LIVE_QUERY)
		volume->AddQuery(this);
}


Query::~Query()
{
	if (fFlags & B_LIVE_QUERY)
		fVolume->RemoveQuery(this);
}


status_t
Query::Rewind()
{
	// free previous stuff

	fStack.MakeEmpty();

	delete fIterator;
	fIterator = NULL;
	fCurrent = NULL;

	// put the whole expression on the stack

	Stack<Term *> stack;
	stack.Push(fExpression->Root());

	Term *term;
	while (stack.Pop(&term)) {
		if (term->Op() < OP_EQUATION) {
			Operator *op = (Operator *)term;

			if (op->Op() == OP_OR) {
				stack.Push(op->Left());
				stack.Push(op->Right());
			} else {
				// For OP_AND, we can use the scoring system to decide which path to add
				if (op->Right()->Score() > op->Left()->Score())
					stack.Push(op->Right());
				else
					stack.Push(op->Left());
			}
		} else if (term->Op() == OP_EQUATION || fStack.Push((Equation *)term) < B_OK)
			FATAL(("Unknown term on stack or stack error"));
	}

	return B_OK;
}


status_t 
Query::GetNextEntry(struct dirent *dirent, size_t size)
{
	// If we don't have an equation to use yet/anymore, get a new one
	// from the stack
	while (true) {
		if (fIterator == NULL) {
			if (!fStack.Pop(&fCurrent)
				|| fCurrent == NULL
				|| fCurrent->PrepareQuery(fVolume, fIndex, &fIterator,
						fFlags & B_QUERY_NON_INDEXED) < B_OK)
				return B_ENTRY_NOT_FOUND;
		}
		if (fCurrent == NULL)
			RETURN_ERROR(B_ERROR);

		status_t status = fCurrent->GetNextMatching(fVolume, fIterator, dirent, size);
		if (status < B_OK) {
			delete fIterator;
			fIterator = NULL;
			fCurrent = NULL;
		} else {
			// only return if we have another entry
			return B_OK;
		}
	}
}


void 
Query::SetLiveMode(port_id port, int32 token)
{
	fPort = port;
	fToken = token;

	if ((fFlags & B_LIVE_QUERY) == 0) {
		// you can decide at any point to set the live query mode,
		// only live queries have to be updated by attribute changes
		fFlags |= B_LIVE_QUERY;
		fVolume->AddQuery(this);
	}
}


static void
send_entry_notification(port_id port, int32 token, Volume* volume, Entry* entry,
	bool created)
{
	if (created) {
		notify_query_entry_created(port, token, volume->GetID(),
			entry->GetParent()->GetID(), entry->GetName(),
			entry->GetNode()->GetID());
	} else {
		notify_query_entry_removed(port, token, volume->GetID(),
			entry->GetParent()->GetID(), entry->GetName(),
			entry->GetNode()->GetID());
	}
}


void 
Query::LiveUpdate(Entry *entry, Node* node, const char *attribute, int32 type,
	const uint8 *oldKey, size_t oldLength, const uint8 *newKey,
	size_t newLength)
{
PRINT(("%p->Query::LiveUpdate(%p, %p, \"%s\", 0x%lx, %p, %lu, %p, %lu)\n",
this, entry, node, attribute, type, oldKey, oldLength, newKey, newLength));
	if (fPort < 0 || fExpression == NULL || node == NULL || attribute == NULL)
		return;

	// ToDo: check if the attribute is part of the query at all...

	// If no entry has been supplied, but the we need one for the evaluation
	// (i.e. the "name" attribute is used), we invoke ourselves for all entries
	// referring to the given node.
	if (!entry && fNeedsEntry) {
		entry = node->GetFirstReferrer();
		while (entry) {
			LiveUpdate(entry, node, attribute, type, oldKey, oldLength, newKey,
				newLength);
			entry = node->GetNextReferrer(entry);
		}
		return;
	}

	status_t oldStatus = fExpression->Root()->Match(entry, node, attribute,
		type, oldKey, oldLength);
	status_t newStatus = fExpression->Root()->Match(entry, node, attribute,
		type, newKey, newLength);
PRINT(("  oldStatus: 0x%lx, newStatus: 0x%lx\n", oldStatus, newStatus));

	bool created;
	if (oldStatus == MATCH_OK && newStatus == MATCH_OK) {
		// only send out a notification if the name was changed 
		if (oldKey == NULL || strcmp(attribute,"name"))
			return;

		if (entry) {
			// entry should actually always be given, when the changed
			// attribute is the entry name
PRINT(("notification: old: removed\n"));
			notify_query_entry_removed(fPort, fToken, fVolume->GetID(),
				entry->GetParent()->GetID(), (const char *)oldKey,
				entry->GetNode()->GetID());
		}
		created = true;
	} else if (oldStatus != MATCH_OK && newStatus != MATCH_OK) {
		// nothing has changed
		return;
	} else if (oldStatus == MATCH_OK && newStatus != MATCH_OK)
		created = false;
	else
		created = true;

	// We send a notification for the given entry, if any, or otherwise for
	// all entries referring to the node;
	if (entry) {
PRINT(("notification: new: %s\n", (created ? "created" : "removed")));
		send_entry_notification(fPort, fToken, fVolume, entry, created);
	} else {
		entry = node->GetFirstReferrer();
		while (entry) {
			send_entry_notification(fPort, fToken, fVolume, entry, created);
			entry = node->GetNextReferrer(entry);
		}
	}
}


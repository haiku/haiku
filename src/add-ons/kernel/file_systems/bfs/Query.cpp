/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * This file may be used under the terms of the MIT License.
 */


/*!	Query parsing and evaluation

	The pattern matching is roughly based on code originally written
	by J. Kercheval, and on code written by Kenneth Almquist, though
	it shares no code.
*/


#include "Query.h"

#include <query_private.h>

#include "BPlusTree.h"
#include "bfs.h"
#include "Debug.h"
#include "Index.h"
#include "Inode.h"
#include "Volume.h"


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
		// is only used for invalid equations

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
	char	String[INODE_FILE_NAME_LENGTH];
};


/*!	Abstract base class for the operator/equation classes.
*/
class Term {
public:
						Term(int8 op) : fOp(op), fParent(NULL) {}
	virtual				~Term() {}

			int8		Op() const { return fOp; }

			void		SetParent(Term* parent) { fParent = parent; }
			Term*		Parent() const { return fParent; }

	virtual	status_t	Match(Inode* inode, const char* attribute = NULL,
							int32 type = 0, const uint8* key = NULL,
							size_t size = 0) = 0;
	virtual	void		Complement() = 0;

	virtual	void		CalculateScore(Index& index) = 0;
	virtual	int32		Score() const = 0;

	virtual	status_t	InitCheck() = 0;

#ifdef DEBUG
	virtual	void		PrintToStream() = 0;
#endif

protected:
			int8		fOp;
			Term*		fParent;
};


/*!	An Equation object represents an "attribute-equation operator-value" pair.

	Although an Equation object is quite independent from the volume on which
	the query is run, there are some dependencies that are produced while
	querying:
	The type/size of the value, the score, and if it has an index or not.
	So you could run more than one query on the same volume, but it might return
	wrong values when it runs concurrently on another volume.
	That's not an issue right now, because we run single-threaded and don't use
	queries more than once.
*/
class Equation : public Term {
public:
						Equation(char** expression);
	virtual				~Equation();

	virtual	status_t	InitCheck();

			status_t	ParseQuotedString(char** _start, char** _end);
			char*		CopyString(char* start, char* end);

	virtual	status_t	Match(Inode* inode, const char* attribute = NULL,
							int32 type = 0, const uint8* key = NULL,
							size_t size = 0);
	virtual void		Complement();

			status_t	PrepareQuery(Volume* volume, Index& index,
							TreeIterator** iterator, bool queryNonIndexed);
			status_t	GetNextMatching(Volume* volume, TreeIterator* iterator,
							struct dirent* dirent, size_t bufferSize);

	virtual	void		CalculateScore(Index &index);
	virtual	int32		Score() const { return fScore; }

#ifdef DEBUG
	virtual	void		PrintToStream();
#endif

private:
						Equation(const Equation& other);
						Equation& operator=(const Equation& other);
							// no implementation

			status_t	ConvertValue(type_code type);
			bool		CompareTo(const uint8* value, uint16 size);
			uint8*		Value() const { return (uint8*)&fValue; }
			status_t	MatchEmptyString();

			char*		fAttribute;
			char*		fString;
			union value fValue;
			type_code	fType;
			size_t		fSize;
			bool		fIsPattern;
			bool		fIsSpecialTime;

			int32		fScore;
			bool		fHasIndex;
};


/*!	The Operator class does not represent a generic operator, but only those
	that combine two equations, namely "or", and "and".
*/
class Operator : public Term {
public:
						Operator(Term* left, int8 op, Term* right);
	virtual				~Operator();

			Term*		Left() const { return fLeft; }
			Term*		Right() const { return fRight; }

	virtual	status_t	Match(Inode* inode, const char* attribute = NULL,
							int32 type = 0, const uint8* key = NULL,
							size_t size = 0);
	virtual	void		Complement();

	virtual	void		CalculateScore(Index& index);
	virtual	int32		Score() const;

	virtual	status_t	InitCheck();

#ifdef DEBUG
	virtual	void		PrintToStream();
#endif

private:
						Operator(const Operator& other);
						Operator& operator=(const Operator& other);
							// no implementation

			Term*		fLeft;
			Term*		fRight;
};


//	#pragma mark -


void
skipWhitespace(char** expr, int32 skip = 0)
{
	char* string = (*expr) + skip;
	while (*string == ' ' || *string == '\t') string++;
	*expr = string;
}


void
skipWhitespaceReverse(char** expr, char* stop)
{
	char* string = *expr;
	while (string > stop && (*string == ' ' || *string == '\t'))
		string--;
	*expr = string;
}


//	#pragma mark -


uint32
utf8ToUnicode(char** string)
{
	uint8* bytes = (uint8*)*string;
	int32 length;
	uint8 mask = 0x1f;

	switch (bytes[0] & 0xf0) {
		case 0xc0:
		case 0xd0:
			length = 2;
			break;
		case 0xe0:
			length = 3;
			break;
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
	for (; i < length && (bytes[i] & 0x80) > 0; i++)
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
getFirstPatternSymbol(char* string)
{
	char c;

	for (int32 index = 0; (c = *string++); index++) {
		if (c == '*' || c == '?' || c == '[')
			return index;
	}
	return -1;
}


bool
isPattern(char* string)
{
	return getFirstPatternSymbol(string) >= 0 ? true : false;
}


status_t
isValidPattern(char* pattern)
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


/*!	Matches the string against the given wildcard pattern.
	Returns either MATCH_OK, or NO_MATCH when everything went fine, or
	values < 0 (see enum at the top of Query.cpp) if an error occurs.
*/
status_t
matchString(char* pattern, char* string)
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
						status_t status = matchString(pattern, string);
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
					} else if (pattern[0] == '-' && pattern[1] != ']'
							&& pattern[1]) {
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


Equation::Equation(char** expr)
	: Term(OP_EQUATION),
	fAttribute(NULL),
	fString(NULL),
	fType(0),
	fIsPattern(false)
{
	char* string = *expr;
	char* start = string;
	char* end = NULL;

	// Since the equation is the integral part of any query, we're just parsing
	// the whole thing here.
	// The whitespace at the start is already removed in
	// Expression::ParseEquation()

	if (*start == '"' || *start == '\'') {
		// string is quoted (start has to be on the beginning of a string)
		if (ParseQuotedString(&start, &end) < B_OK)
			return;

		// set string to a valid start of the equation symbol
		string = end + 2;
		skipWhitespace(&string);
		if (*string != '=' && *string != '<' && *string != '>'
			&& *string != '!') {
			*expr = string;
			return;
		}
	} else {
		// search the (in)equation for the actual equation symbol (and for other operators
		// in case the equation is malformed)
		while (*string && *string != '=' && *string != '<' && *string != '>'
			&& *string != '!' && *string != '&' && *string != '|') {
			string++;
		}

		// get the attribute string	(and trim whitespace), in case
		// the string was not quoted
		end = string - 1;
		skipWhitespaceReverse(&end, start);
	}

	// attribute string is empty (which is not allowed)
	if (start > end)
		return;

	// At this point, "start" points to the beginning of the string, "end"
	// points to the last character of the string, and "string" points to the
	// first character of the equation symbol

	// test for the right symbol (as this doesn't need any memory)
	switch (*string) {
		case '=':
			fOp = OP_EQUAL;
			break;
		case '>':
			fOp = *(string + 1) == '='
				? OP_GREATER_THAN_OR_EQUAL : OP_GREATER_THAN;
			break;
		case '<':
			fOp = *(string + 1) == '='
				? OP_LESS_THAN_OR_EQUAL : OP_LESS_THAN;
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

	// The special time flag is set if the time values are shifted
	// 64-bit values to reduce the number of duplicates.
	// We have to be able to compare them against unshifted values
	// later. The only index which needs this is the last_modified
	// index, but we may want to open that feature for other indices,
	// too one day.
	fIsSpecialTime = !strcmp(fAttribute, "last_modified");

	*expr = string;
}


Equation::~Equation()
{
	free(fAttribute);
	free(fString);
}


status_t
Equation::InitCheck()
{
	if (fAttribute == NULL || fString == NULL || fOp == OP_NONE)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
Equation::ParseQuotedString(char** _start, char** _end)
{
	char* start = *_start;
	char quote = *start++;
	char* end = start;

	for (; *end && *end != quote; end++) {
		if (*end == '\\')
			end++;
	}
	if (*end == '\0')
		return B_BAD_VALUE;

	*_start = start;
	*_end = end - 1;

	return B_OK;
}


char*
Equation::CopyString(char* start, char* end)
{
	// end points to the last character of the string - and the length
	// also has to include the null-termination
	int32 length = end + 2 - start;
	// just to make sure; since that's the max. attribute name length and
	// the max. string in an index, it make sense to have it that way
	if (length > INODE_FILE_NAME_LENGTH || length <= 0)
		return NULL;

	char* copy = (char*)malloc(length);
	if (copy == NULL)
		return NULL;

	memcpy(copy, start, length - 1);
	copy[length - 1] = '\0';

	return copy;
}


status_t
Equation::ConvertValue(type_code type)
{
	// Has the type already been converted?
	if (type == fType)
		return B_OK;

	char* string = fString;

	switch (type) {
		case B_MIME_STRING_TYPE:
			type = B_STRING_TYPE;
			// supposed to fall through
		case B_STRING_TYPE:
			strncpy(fValue.String, string, INODE_FILE_NAME_LENGTH);
			fValue.String[INODE_FILE_NAME_LENGTH - 1] = '\0';
			fSize = strlen(fValue.String);
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
			FATAL(("query value conversion to 0x%x requested!\n", (int)type));
			// should we fail here or just do a safety int32 conversion?
			return B_ERROR;
	}

	fType = type;

	// patterns are only allowed for string types
	if (fType != B_STRING_TYPE && fIsPattern)
		fIsPattern = false;

	return B_OK;
}


/*!	Returns true when the key matches the equation. You have to
	call ConvertValue() before this one.
*/
bool
Equation::CompareTo(const uint8* value, uint16 size)
{
	int32 compare;

	// fIsPattern is only true if it's a string type, and fOp OP_EQUAL, or
	// OP_UNEQUAL
	if (fIsPattern) {
		// we have already validated the pattern, so we don't check for failing
		// here - if something is broken, and matchString() returns an error,
		// we just don't match
		compare = matchString(fValue.String, (char*)value) == MATCH_OK ? 0 : 1;
	} else if (fIsSpecialTime) {
		// the index is a shifted int64 index, but we have to match
		// against an unshifted value (i.e. the last_modified index)
		int64 timeValue = *(int64*)value >> INODE_TIME_SHIFT;
		compare = compareKeys(fType, &timeValue, sizeof(int64), &fValue.Int64,
			sizeof(int64));
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
	// There is no matching attribute, we will just bail out if we
	// already know that our value is not of a string type.
	// If not, it will be converted to a string - and then be compared with "".
	// That's why we have to call ConvertValue() here - but it will be
	// a cheap call for the next time
	// TODO: Should we do this only for OP_UNEQUAL?
	if (fType != 0 && fType != B_STRING_TYPE)
		return NO_MATCH;

	status_t status = ConvertValue(B_STRING_TYPE);
	if (status == B_OK)
		status = CompareTo((const uint8*)"", fSize) ? MATCH_OK : NO_MATCH;

	return status;
}


/*!	Matches the inode's attribute value with the equation.
	Returns MATCH_OK if it matches, NO_MATCH if not, < 0 if something went
	wrong.
*/
status_t
Equation::Match(Inode* inode, const char* attributeName, int32 type,
	const uint8* key, size_t size)
{
	// get a pointer to the attribute in question
	NodeGetter nodeGetter(inode->GetVolume());
	union value value;
	uint8* buffer = (uint8*)&value;
	bool locked = false;

	// first, check if we are matching for a live query and use that value
	if (attributeName != NULL && !strcmp(fAttribute, attributeName)) {
		if (key == NULL) {
			if (type == B_STRING_TYPE)
				return MatchEmptyString();

			return NO_MATCH;
		}
		buffer = const_cast<uint8*>(key);
	} else if (!strcmp(fAttribute, "name")) {
		// we need to lock before accessing Inode::Name()
		nodeGetter.SetToNode(inode);

		recursive_lock_lock(&inode->SmallDataLock());
		locked = true;

		// if not, check for "fake" attributes ("name", "size", "last_modified")
		buffer = (uint8*)inode->Name(nodeGetter.Node());
		if (buffer == NULL) {
			recursive_lock_unlock(&inode->SmallDataLock());
			return B_ERROR;
		}

		type = B_STRING_TYPE;
		size = strlen((const char*)buffer);
	} else if (!strcmp(fAttribute, "size")) {
#ifdef BFS_NATIVE_ENDIAN
		buffer = (uint8*)&inode->Node().data.size;
#else
		value.Int64 = inode->Size();
#endif
		type = B_INT64_TYPE;
	} else if (!strcmp(fAttribute, "last_modified")) {
#ifdef BFS_NATIVE_ENDIAN
		buffer = (uint8*)&inode->Node().last_modified_time;
#else
		value.Int64 = inode->Node().LastModifiedTime();
#endif
		type = B_INT64_TYPE;
	} else {
		// then for attributes in the small_data section, and finally for the
		// real attributes
		nodeGetter.SetToNode(inode);
		Inode* attribute;

		recursive_lock_lock(&inode->SmallDataLock());
		small_data* smallData = inode->FindSmallData(nodeGetter.Node(),
			fAttribute);
		if (smallData != NULL) {
			buffer = smallData->Data();
			type = smallData->type;
			size = smallData->data_size;
			locked = true;
		} else {
			// needed to unlock the small_data section as fast as possible
			recursive_lock_unlock(&inode->SmallDataLock());
			nodeGetter.Unset();

			if (inode->GetAttribute(fAttribute, &attribute) == B_OK) {
				buffer = (uint8*)&value;
				type = attribute->Type();
				size = attribute->Size();

				if (size > INODE_FILE_NAME_LENGTH)
					size = INODE_FILE_NAME_LENGTH;

				if (attribute->ReadAt(0, buffer, &size) < B_OK) {
					inode->ReleaseAttribute(attribute);
					return B_IO_ERROR;
				}
				inode->ReleaseAttribute(attribute);
			} else
				return MatchEmptyString();
		}
	}
	// prepare own value for use, if it is possible to convert it
	status_t status = ConvertValue(type);
	if (status == B_OK)
		status = CompareTo(buffer, size) ? MATCH_OK : NO_MATCH;

	if (locked)
		recursive_lock_unlock(&inode->SmallDataLock());

	RETURN_ERROR(status);
}


void
Equation::CalculateScore(Index &index)
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
	fScore = fScore * ((2048 * 1024LL) / index.Node()->Size());
}


status_t
Equation::PrepareQuery(Volume* /*volume*/, Index& index,
	TreeIterator** iterator, bool queryNonIndexed)
{
	status_t status = index.SetTo(fAttribute);

	// if we should query attributes without an index, we can just proceed here
	if (status != B_OK && !queryNonIndexed)
		return B_ENTRY_NOT_FOUND;

	type_code type;

	// Special case for OP_UNEQUAL - it will always operate through the whole
	// index but we need the call to the original index to get the correct type
	if (status != B_OK || fOp == OP_UNEQUAL) {
		// Try to get an index that holds all files (name)
		// Also sets the default type for all attributes without index
		// to string.
		type = status < B_OK ? B_STRING_TYPE : index.Type();

		if (index.SetTo("name") != B_OK)
			return B_ENTRY_NOT_FOUND;

		fHasIndex = false;
	} else {
		fHasIndex = true;
		type = index.Type();
	}

	if (ConvertValue(type) < B_OK)
		return B_BAD_VALUE;

	BPlusTree* tree = index.Node()->Tree();
	if (tree == NULL)
		return B_ERROR;

	*iterator = new TreeIterator(tree);
	if (*iterator == NULL)
		return B_NO_MEMORY;

	if ((fOp == OP_EQUAL || fOp == OP_GREATER_THAN
			|| fOp == OP_GREATER_THAN_OR_EQUAL || fIsPattern)
		&& fHasIndex) {
		// set iterator to the exact position

		int32 keySize = index.KeySize();

		// At this point, fIsPattern is only true if it's a string type, and fOp
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
				keySize = strlen(fValue.String);

				// The empty string is a special case - we normally don't check
				// for the trailing null byte, in the case for the empty string
				// we do it explicitly, because there can't be keys in the
				// B+tree with a length of zero
				if (keySize == 0)
					keySize = 1;
			} else
				RETURN_ERROR(B_ENTRY_NOT_FOUND);
		}

		if (fIsSpecialTime) {
			// we have to find the first matching shifted value
			off_t value = fValue.Int64 << INODE_TIME_SHIFT;
			status = (*iterator)->Find((uint8*)&value, keySize);
			if (status == B_ENTRY_NOT_FOUND)
				return B_OK;
		} else {
			status = (*iterator)->Find(Value(), keySize);
			if (fOp == OP_EQUAL && !fIsPattern)
				return status;
			else if (status == B_ENTRY_NOT_FOUND
				&& (fIsPattern || fOp == OP_GREATER_THAN
					|| fOp == OP_GREATER_THAN_OR_EQUAL))
				return B_OK;
		}

		RETURN_ERROR(status);
	}

	return B_OK;
}


status_t
Equation::GetNextMatching(Volume* volume, TreeIterator* iterator,
	struct dirent* dirent, size_t bufferSize)
{
	while (true) {
		union value indexValue;
		uint16 keyLength;
		uint16 duplicate;
		off_t offset;

		status_t status = iterator->GetNextEntry(&indexValue, &keyLength,
			(uint16)sizeof(indexValue), &offset, &duplicate);
		if (status != B_OK)
			return status;

		// only compare against the index entry when this is the correct
		// index for the equation
		if (fHasIndex && duplicate < 2
			&& !CompareTo((uint8*)&indexValue, keyLength)) {
			// They aren't equal? Let the operation decide what to do. Since
			// we always start at the beginning of the index (or the correct
			// position), only some needs to be stopped if the entry doesn't
			// fit.
			if (fOp == OP_LESS_THAN
				|| fOp == OP_LESS_THAN_OR_EQUAL
				|| (fOp == OP_EQUAL && !fIsPattern))
				return B_ENTRY_NOT_FOUND;

			if (duplicate > 0)
				iterator->SkipDuplicates();
			continue;
		}

		Vnode vnode(volume, offset);
		Inode* inode;
		if ((status = vnode.Get(&inode)) != B_OK) {
			REPORT_ERROR(status);
			FATAL(("could not get inode %" B_PRIdOFF " in index \"%s\"!\n",
				offset, fAttribute));
			// try with next
			continue;
		}

		// TODO: check user permissions here - but which one?!
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
		Term* term = this;
		status = MATCH_OK;

		if (!fHasIndex)
			status = Match(inode);

		while (term != NULL && status == MATCH_OK) {
			Operator* parent = (Operator*)term->Parent();
			if (parent == NULL)
				break;

			if (parent->Op() == OP_AND) {
				// choose the other child of the parent
				Term* other = parent->Right();
				if (other == term)
					other = parent->Left();

				if (other == NULL) {
					FATAL(("&&-operator has only one child... (parent = %p)\n",
						parent));
					break;
				}
				status = other->Match(inode);
				if (status < 0) {
					REPORT_ERROR(status);
					status = NO_MATCH;
				}
			}
			term = (Term*)parent;
		}

		if (status == MATCH_OK) {
			dirent->d_dev = volume->ID();
			dirent->d_ino = offset;
			dirent->d_pdev = volume->ID();
			dirent->d_pino = volume->ToVnode(inode->Parent());

			if (inode->GetName(dirent->d_name) < B_OK) {
				FATAL(("inode %" B_PRIdOFF " in query has no name!\n",
					inode->BlockNumber()));
			}

			dirent->d_reclen = sizeof(struct dirent) + strlen(dirent->d_name);
		}

		if (status == MATCH_OK)
			return B_OK;
	}
	RETURN_ERROR(B_ERROR);
}


//	#pragma mark -


Operator::Operator(Term* left, int8 op, Term* right)
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
Operator::Match(Inode* inode, const char* attribute, int32 type,
	const uint8* key, size_t size)
{
	if (fOp == OP_AND) {
		status_t status = fLeft->Match(inode, attribute, type, key, size);
		if (status != MATCH_OK)
			return status;

		return fRight->Match(inode, attribute, type, key, size);
	} else {
		// choose the term with the better score for OP_OR
		Term* first;
		Term* second;
		if (fRight->Score() > fLeft->Score()) {
			first = fLeft;
			second = fRight;
		} else {
			first = fRight;
			second = fLeft;
		}

		status_t status = first->Match(inode, attribute, type, key, size);
		if (status != NO_MATCH)
			return status;

		return second->Match(inode, attribute, type, key, size);
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
Operator::CalculateScore(Index &index)
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


#if 0
Term*
Operator::Copy() const
{
	if (fEquation != NULL) {
		Equation* equation = new Equation(*fEquation);
		if (equation == NULL)
			return NULL;

		Term* term = new Term(equation);
		if (term == NULL)
			delete equation;

		return term;
	}

	Term* left = NULL;
	Term* right = NULL;

	if (fLeft != NULL && (left = fLeft->Copy()) == NULL)
		return NULL;
	if (fRight != NULL && (right = fRight->Copy()) == NULL) {
		delete left;
		return NULL;
	}

	Term* term = new Term(left, fOp, right);
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
	const char* symbol = "???";
	switch (fOp) {
		case OP_EQUAL: symbol = "=="; break;
		case OP_UNEQUAL: symbol = "!="; break;
		case OP_GREATER_THAN: symbol = ">"; break;
		case OP_GREATER_THAN_OR_EQUAL: symbol = ">="; break;
		case OP_LESS_THAN: symbol = "<"; break;
		case OP_LESS_THAN_OR_EQUAL: symbol = "<="; break;
	}
	D(__out("[\"%s\" %s \"%s\"]", fAttribute, symbol, fString));
}

#endif	// DEBUG

//	#pragma mark -


Expression::Expression(char* expr)
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


Term*
Expression::ParseEquation(char** expr)
{
	skipWhitespace(expr);

	bool _not = false;
	if (**expr == '!') {
		skipWhitespace(expr, 1);
		if (**expr != '(')
			return NULL;

		_not = true;
	}

	if (**expr == ')') {
		// shouldn't be handled here
		return NULL;
	} else if (**expr == '(') {
		skipWhitespace(expr, 1);

		Term* term = ParseOr(expr);

		skipWhitespace(expr);

		if (**expr != ')') {
			delete term;
			return NULL;
		}

		// If the term is negated, we just complement the tree, to get
		// rid of the not, a.k.a. DeMorgan's Law.
		if (_not)
			term->Complement();

		skipWhitespace(expr, 1);

		return term;
	}

	Equation* equation = new Equation(expr);
	if (equation == NULL || equation->InitCheck() < B_OK) {
		delete equation;
		return NULL;
	}
	return equation;
}


Term*
Expression::ParseAnd(char** expr)
{
	Term* left = ParseEquation(expr);
	if (left == NULL)
		return NULL;

	while (IsOperator(expr, '&')) {
		Term* right = ParseAnd(expr);
		Term* newParent = NULL;

		if (right == NULL
			|| (newParent = new Operator(left, OP_AND, right)) == NULL) {
			delete left;
			delete right;

			return NULL;
		}
		left = newParent;
	}

	return left;
}


Term*
Expression::ParseOr(char** expr)
{
	Term* left = ParseAnd(expr);
	if (left == NULL)
		return NULL;

	while (IsOperator(expr, '|')) {
		Term* right = ParseAnd(expr);
		Term* newParent = NULL;

		if (right == NULL
			|| (newParent = new Operator(left, OP_OR, right)) == NULL) {
			delete left;
			delete right;

			return NULL;
		}
		left = newParent;
	}

	return left;
}


bool
Expression::IsOperator(char** expr, char op)
{
	char* string = *expr;

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


Query::Query(Volume* volume, Expression* expression, uint32 flags)
	:
	fVolume(volume),
	fExpression(expression),
	fCurrent(NULL),
	fIterator(NULL),
	fIndex(volume),
	fFlags(flags),
	fPort(-1)
{
	// If the expression has a valid root pointer, the whole tree has
	// already passed the sanity check, so that we don't have to check
	// every pointer
	if (volume == NULL || expression == NULL || expression->Root() == NULL)
		return;

	// create index on the stack and delete it afterwards
	fExpression->Root()->CalculateScore(fIndex);
	fIndex.Unset();

	Rewind();

	if ((fFlags & B_LIVE_QUERY) != 0)
		volume->AddQuery(this);
}


Query::~Query()
{
	if ((fFlags & B_LIVE_QUERY) != 0)
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

	Stack<Term*> stack;
	stack.Push(fExpression->Root());

	Term* term;
	while (stack.Pop(&term)) {
		if (term->Op() < OP_EQUATION) {
			Operator* op = (Operator*)term;

			if (op->Op() == OP_OR) {
				stack.Push(op->Left());
				stack.Push(op->Right());
			} else {
				// For OP_AND, we can use the scoring system to decide which
				// path to add
				if (op->Right()->Score() > op->Left()->Score())
					stack.Push(op->Right());
				else
					stack.Push(op->Left());
			}
		} else if (term->Op() == OP_EQUATION
			|| fStack.Push((Equation*)term) != B_OK)
			FATAL(("Unknown term on stack or stack error"));
	}

	return B_OK;
}


status_t
Query::GetNextEntry(struct dirent* dirent, size_t size)
{
	// If we don't have an equation to use yet/anymore, get a new one
	// from the stack
	while (true) {
		if (fIterator == NULL) {
			if (!fStack.Pop(&fCurrent)
				|| fCurrent == NULL)
				return B_ENTRY_NOT_FOUND;

			status_t status = fCurrent->PrepareQuery(fVolume, fIndex,
				&fIterator, fFlags & B_QUERY_NON_INDEXED);
			if (status == B_ENTRY_NOT_FOUND) {
				// try next equation
				continue;
			}

			if (status != B_OK)
				return status;
		}
		if (fCurrent == NULL)
			RETURN_ERROR(B_ERROR);

		status_t status = fCurrent->GetNextMatching(fVolume, fIterator, dirent,
			size);
		if (status != B_OK) {
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


void
Query::LiveUpdate(Inode* inode, const char* attribute, int32 type,
	const uint8* oldKey, size_t oldLength, const uint8* newKey,
	size_t newLength)
{
	if (fPort < 0 || fExpression == NULL || attribute == NULL)
		return;

	// TODO: check if the attribute is part of the query at all...

	status_t oldStatus = fExpression->Root()->Match(inode, attribute, type,
		oldKey, oldLength);
	status_t newStatus = fExpression->Root()->Match(inode, attribute, type,
		newKey, newLength);

	bool entryCreated = false;
	bool stillInQuery = false;

	if (oldStatus != MATCH_OK) {
		if (newStatus != MATCH_OK) {
			// nothing has changed
			return;
		}
		entryCreated = true;
	} else if (newStatus != MATCH_OK) {
		// entry got removed
		entryCreated = false;
	} else if ((fFlags & B_ATTR_CHANGE_NOTIFICATION) != 0) {
		// The entry stays in the query
		stillInQuery = true;
	} else
		return;

	// we may need to get the name of the inode

	char nameBuffer[B_FILE_NAME_LENGTH];
	const char* name;

	if (strcmp(attribute, "name")) {
		if (inode->GetName(nameBuffer) != B_OK)
			nameBuffer[0] = '\0';
		name = nameBuffer;
	} else {
		// a shortcut to prevent having to scan the attribute section
		name = (const char*)newKey;
	}

	// notify query listeners

	if (stillInQuery) {
		notify_query_attr_changed(fPort, fToken, fVolume->ID(),
			fVolume->ToVnode(inode->Parent()), name, inode->ID());
	} else if (entryCreated) {
		notify_query_entry_created(fPort, fToken, fVolume->ID(),
			fVolume->ToVnode(inode->Parent()), name, inode->ID());
	} else {
		notify_query_entry_removed(fPort, fToken, fVolume->ID(),
			fVolume->ToVnode(inode->Parent()), name, inode->ID());
	}
}


void
Query::LiveUpdateRenameMove(Inode* inode, ino_t oldDirectoryID,
	const char* oldName, size_t oldLength, ino_t newDirectoryID,
	const char* newName, size_t newLength)
{
	if (fPort < 0 || fExpression == NULL)
		return;

	// TODO: check if the attribute is part of the query at all...

	status_t oldStatus = fExpression->Root()->Match(inode, "name",
		B_STRING_TYPE, (const uint8*)oldName, oldLength);
	status_t newStatus = fExpression->Root()->Match(inode, "name",
		B_STRING_TYPE, (const uint8*)newName, newLength);

	if (oldStatus != MATCH_OK || oldStatus != newStatus)
		return;

	// The entry stays in the query, notify query listeners about the rename
	// or move

	notify_query_entry_removed(fPort, fToken, fVolume->ID(),
		oldDirectoryID, oldName, inode->ID());

	notify_query_entry_created(fPort, fToken, fVolume->ID(),
		newDirectoryID, newName, inode->ID());
}

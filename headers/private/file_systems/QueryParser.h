/*
 * Copyright 2001-2014, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef _FILE_SYSTEMS_QUERY_PARSER_H
#define _FILE_SYSTEMS_QUERY_PARSER_H


/*!	Query parsing and evaluation. */


// The operator precedence is =,!=,>,<,>=,<= .. && .. ||.
// Apparently, the "!" (not) can only be used with parentheses.
//
// If you think that there are too few NULL pointer checks in some places
// of the code, just read the beginning of the query constructor.
// The API is not fully available, just the Query and the Expression class
// are.

#ifdef FS_SHELL
#	include <new>

#	include "fssh_api_wrapper.h"
#	include "fssh_auto_deleter.h"
#else
#	include <dirent.h>
#	include <stdlib.h>
#	include <string.h>

#	include <algorithm>
#	include <new>

#	include <fs_interface.h>
#	include <fs_query.h>
#	include <TypeConstants.h>

#	include <util/SinglyLinkedList.h>
#	include <util/Stack.h>

#	include <query_private.h>
#endif	// !FS_SHELL

#include <file_systems/QueryParserUtils.h>


//#define DEBUG_QUERY

#ifndef QUERY_RETURN_ERROR
#	define QUERY_RETURN_ERROR(error)	return (error)
#endif

#ifndef QUERY_REPORT_ERROR
#	define QUERY_REPORT_ERROR(error)	do {} while (false)
#endif

#ifndef QUERY_FATAL
#	define QUERY_FATAL(message...)		panic(message)
#endif

#ifndef QUERY_INFORM
#	define QUERY_INFORM(message...)		dprintf(message)
#endif

#ifndef QUERY_D
#	define QUERY_D(block)
#endif


namespace QueryParser {


template<typename QueryPolicy> class Equation;
template<typename QueryPolicy> class Expression;
template<typename QueryPolicy> class Term;
template<typename QueryPolicy> class Query;


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


template<typename QueryPolicy>
union value {
	int64	Int64;
	uint64	Uint64;
	int32	Int32;
	uint32	Uint32;
	float	Float;
	double	Double;
	char	String[QueryPolicy::kMaxFileNameLength];
};


template<typename QueryPolicy>
class Query {
public:
			typedef typename QueryPolicy::Entry Entry;
			typedef typename QueryPolicy::Index Index;
			typedef typename QueryPolicy::IndexIterator IndexIterator;
			typedef typename QueryPolicy::Node Node;
			typedef typename QueryPolicy::NodeHolder NodeHolder;
			typedef typename QueryPolicy::Context Context;

public:
							Query(Context* context,
								Expression<QueryPolicy>* expression,
								uint32 flags, port_id port, uint32 token);
							~Query();

	static	status_t		Create(Context* context, const char* queryString,
								uint32 flags, port_id port, uint32 token,
								Query<QueryPolicy>*& _query);

			status_t		Rewind();
	inline	status_t		GetNextEntry(struct dirent* dirent, size_t size);

			void			LiveUpdate(Entry* entry, Node* node,
								const char* attribute, int32 type,
								const uint8* oldKey, size_t oldLength,
								const uint8* newKey, size_t newLength);
			void			LiveUpdateRenameMove(Entry* entry, Node* node,
								ino_t oldDirectoryID, const char* oldName,
								size_t oldLength, ino_t newDirectoryID,
								const char* newName, size_t newLength);

			Expression<QueryPolicy>* GetExpression() const
								{ return fExpression; }

			uint32			Flags() const
								{ return fFlags; }

private:
			status_t		_GetNextEntry(struct dirent* dirent, size_t size);
			void			_SendEntryNotification(Entry* entry,
								status_t (*notify)(port_id, int32, dev_t, ino_t,
									const char*, ino_t));

private:
			Context*		fContext;
			Expression<QueryPolicy>* fExpression;
			Equation<QueryPolicy>* fCurrent;
			IndexIterator*	fIterator;
			Index			fIndex;
			Stack<Equation<QueryPolicy>*> fStack;

			uint32			fFlags;
			port_id			fPort;
			int32			fToken;
			bool			fNeedsEntry;
};


/*!	Abstract base class for the operator/equation classes.
*/
template<typename QueryPolicy>
class Term {
public:
			typedef typename QueryPolicy::Entry Entry;
			typedef typename QueryPolicy::Index Index;
			typedef typename QueryPolicy::IndexIterator IndexIterator;
			typedef typename QueryPolicy::Node Node;
			typedef typename QueryPolicy::NodeHolder NodeHolder;
			typedef typename QueryPolicy::Context Context;

public:
						Term(int8 op) : fOp(op), fParent(NULL) {}
	virtual				~Term() {}

			int8		Op() const { return fOp; }

			void		SetParent(Term<QueryPolicy>* parent)
							{ fParent = parent; }
			Term<QueryPolicy>* Parent() const { return fParent; }

	virtual	status_t	Match(Entry* entry, Node* node,
							const char* attribute = NULL, int32 type = 0,
							const uint8* key = NULL, size_t size = 0) = 0;
	virtual	void		Complement() = 0;

	virtual	void		CalculateScore(Index& index) = 0;
	virtual	int32		Score() const = 0;

	virtual	status_t	InitCheck() = 0;

	virtual	bool		NeedsEntry() = 0;

#ifdef DEBUG_QUERY
	virtual	void		PrintToStream() = 0;
#endif

protected:
			int8		fOp;
			Term<QueryPolicy>* fParent;
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
template<typename QueryPolicy>
class Equation : public Term<QueryPolicy> {
public:
			typedef typename QueryPolicy::Entry Entry;
			typedef typename QueryPolicy::Index Index;
			typedef typename QueryPolicy::IndexIterator IndexIterator;
			typedef typename QueryPolicy::Node Node;
			typedef typename QueryPolicy::NodeHolder NodeHolder;
			typedef typename QueryPolicy::Context Context;

public:
						Equation(const char** expression);
	virtual				~Equation();

	virtual	status_t	InitCheck();

			status_t	ParseQuotedString(const char** _start, const char** _end);
			char*		CopyString(const char* start, const char* end);
	inline	bool		_IsEquationChar(char c) const;
	inline	bool		_IsOperatorChar(char c) const;

	virtual	status_t	Match(Entry* entry, Node* node,
							const char* attribute = NULL, int32 type = 0,
							const uint8* key = NULL, size_t size = 0);
	virtual void		Complement();

			status_t	PrepareQuery(Context* context, Index& index,
							IndexIterator** iterator, bool queryNonIndexed);
			status_t	GetNextMatching(Context* context,
							IndexIterator* iterator, struct dirent* dirent,
							size_t bufferSize);

	virtual	void		CalculateScore(Index &index);
	virtual	int32		Score() const { return fScore; }

	virtual	bool		NeedsEntry();

#ifdef DEBUG_QUERY
	virtual	void		PrintToStream();
#endif

private:
						Equation(const Equation& other);
						Equation& operator=(const Equation& other);
							// no implementation

			status_t	ConvertValue(type_code type);
			bool		CompareTo(const uint8* value, size_t size);
			uint8*		Value() const { return (uint8*)&fValue; }

			char*		fAttribute;
			char*		fString;
			union value<QueryPolicy> fValue;
			type_code	fType;
			size_t		fSize;
			bool		fIsPattern;

			int32		fScore;
			bool		fHasIndex;
};


/*!	The Operator class does not represent a generic operator, but only those
	that combine two equations, namely "or", and "and".
*/
template<typename QueryPolicy>
class Operator : public Term<QueryPolicy> {
public:
			typedef typename QueryPolicy::Entry Entry;
			typedef typename QueryPolicy::Index Index;
			typedef typename QueryPolicy::IndexIterator IndexIterator;
			typedef typename QueryPolicy::Node Node;
			typedef typename QueryPolicy::Context Context;

public:
						Operator(Term<QueryPolicy>* left, int8 op,
							Term<QueryPolicy>* right);
	virtual				~Operator();

			Term<QueryPolicy>* Left() const { return fLeft; }
			Term<QueryPolicy>* Right() const { return fRight; }

	virtual	status_t	Match(Entry* entry, Node* node,
							const char* attribute = NULL, int32 type = 0,
							const uint8* key = NULL, size_t size = 0);
	virtual	void		Complement();

	virtual	void		CalculateScore(Index& index);
	virtual	int32		Score() const;

	virtual	status_t	InitCheck();

	virtual	bool		NeedsEntry();

#ifdef DEBUG_QUERY
	virtual	void		PrintToStream();
#endif

private:
						Operator(const Operator& other);
						Operator& operator=(const Operator& other);
							// no implementation

			Term<QueryPolicy>* fLeft;
			Term<QueryPolicy>* fRight;
};


template<typename QueryPolicy>
class Expression {
public:
			typedef typename QueryPolicy::Entry Entry;
			typedef typename QueryPolicy::Index Index;
			typedef typename QueryPolicy::IndexIterator IndexIterator;
			typedef typename QueryPolicy::Node Node;
			typedef typename QueryPolicy::Context Context;

public:
							Expression();
							~Expression();

			status_t		Init(const char* expr, const char** position);

			Term<QueryPolicy>* Root() const { return fTerm; }

protected:
			bool			IsOperator(const char** expr, char op);

private:
							Expression(const Expression& other);
							Expression& operator=(const Expression& other);
								// no implementation

			Term<QueryPolicy>* fTerm;
};


//	#pragma mark -


template<typename QueryPolicy>
Equation<QueryPolicy>::Equation(const char** expr)
	:
	Term<QueryPolicy>(OP_EQUATION),
	fAttribute(NULL),
	fString(NULL),
	fType(0),
	fIsPattern(false)
{
	const char* string = *expr;
	const char* start = string;
	const char* end = NULL;

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
		if (!_IsEquationChar(string[0])) {
			*expr = string;
			return;
		}
	} else {
		// search the (in)equation for the actual equation symbol (and for other operators
		// in case the equation is malformed)
		while (string[0] != 0 && !_IsOperatorChar(string[0])
			&& !_IsEquationChar(string[0])) {
			if (string[0] == '\\' && string[1] != 0)
				string++;
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
			Term<QueryPolicy>::fOp = OP_EQUAL;
			break;
		case '>':
			Term<QueryPolicy>::fOp = *(string + 1) == '='
				? OP_GREATER_THAN_OR_EQUAL : OP_GREATER_THAN;
			break;
		case '<':
			Term<QueryPolicy>::fOp = *(string + 1) == '='
				? OP_LESS_THAN_OR_EQUAL : OP_LESS_THAN;
			break;
		case '!':
			if (*(string + 1) != '=')
				return;
			Term<QueryPolicy>::fOp = OP_UNEQUAL;
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
		while (string[0] && !_IsOperatorChar(string[0]) && string[0] != ')')
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
	if (Term<QueryPolicy>::fOp == OP_EQUAL
		|| Term<QueryPolicy>::fOp == OP_UNEQUAL) {
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


template<typename QueryPolicy>
Equation<QueryPolicy>::~Equation()
{
	free(fAttribute);
	free(fString);
}


template<typename QueryPolicy>
status_t
Equation<QueryPolicy>::InitCheck()
{
	if (fAttribute == NULL || fString == NULL
		|| Term<QueryPolicy>::fOp == OP_NONE) {
		return B_BAD_VALUE;
	}

	return B_OK;
}


template<typename QueryPolicy>
status_t
Equation<QueryPolicy>::ParseQuotedString(const char** _start, const char** _end)
{
	const char* start = *_start;
	const char quote = *start++;
	const char* end = start;

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


template<typename QueryPolicy>
char*
Equation<QueryPolicy>::CopyString(const char* start, const char* end)
{
	// end points to the last character of the string - and the length
	// also has to include the null-termination
	int32 length = end + 2 - start;
	// just to make sure; since that's the max. attribute name length and
	// the max. string in an index, it make sense to have it that way
	if (length > QueryPolicy::kMaxFileNameLength || length <= 0)
		return NULL;

	char* copy = (char*)malloc(length);
	if (copy == NULL)
		return NULL;

	// Filter out remaining escaping slashes
	for (int32 i = 0; i < length; i++) {
		char c = start++[0];
		if (c == '\\' && i < length) {
			length--;
			i--;
			continue;
		}
		copy[i] = c;
	}
	copy[length - 1] = '\0';

	return copy;
}


template<typename QueryPolicy>
bool
Equation<QueryPolicy>::_IsEquationChar(char c) const
{
	return c == '=' || c == '<' || c == '>' || c == '!';
}


template<typename QueryPolicy>
bool
Equation<QueryPolicy>::_IsOperatorChar(char c) const
{
	return c == '&' || c == '|';
}


template<typename QueryPolicy>
status_t
Equation<QueryPolicy>::ConvertValue(type_code type)
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
			strncpy(fValue.String, string, QueryPolicy::kMaxFileNameLength);
			fValue.String[QueryPolicy::kMaxFileNameLength - 1] = '\0';
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
		case B_TIME_TYPE:
			type = B_INT64_TYPE;
			// supposed to fall through
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
			QUERY_FATAL("query value conversion to 0x%x requested!\n",
				(int)type);
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
template<typename QueryPolicy>
bool
Equation<QueryPolicy>::CompareTo(const uint8* value, size_t size)
{
	int32 compare;

	// fIsPattern is only true if it's a string type, and fOp OP_EQUAL, or
	// OP_UNEQUAL
	if (fIsPattern) {
		// we have already validated the pattern, so we don't check for failing
		// here - if something is broken, and matchString() returns an error,
		// we just don't match
		compare = matchString(fValue.String, (const char*)value) == MATCH_OK ? 0 : 1;
	} else
		compare = compareKeys(fType, value, size, Value(), fSize);

	switch (Term<QueryPolicy>::fOp) {
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
	QUERY_FATAL("Unknown/Unsupported operation: %d\n", Term<QueryPolicy>::fOp);
	return false;
}


template<typename QueryPolicy>
void
Equation<QueryPolicy>::Complement()
{
	QUERY_D(if (Term<QueryPolicy>::fOp <= OP_EQUATION || Term<QueryPolicy>::fOp > OP_LESS_THAN_OR_EQUAL) {
		QUERY_FATAL("op out of range!\n");
		return;
	});

	int8 complementOp[] = {OP_UNEQUAL, OP_EQUAL, OP_LESS_THAN_OR_EQUAL,
			OP_GREATER_THAN_OR_EQUAL, OP_LESS_THAN, OP_GREATER_THAN};
	Term<QueryPolicy>::fOp = complementOp[Term<QueryPolicy>::fOp - OP_EQUAL];
}


/*!	Matches the node's attribute value with the equation.
	Returns MATCH_OK if it matches, NO_MATCH if not, < 0 if something went
	wrong.
*/
template<typename QueryPolicy>
status_t
Equation<QueryPolicy>::Match(Entry* entry, Node* node,
	const char* attributeName, int32 type, const uint8* key, size_t size)
{
	// get a pointer to the attribute in question
	NodeHolder nodeHolder;
	union value<QueryPolicy> value;
	uint8* buffer = (uint8*)&value;
	const size_t bufferSize = sizeof(value);

	// first, check if we are matching for a live query and use that value
	if (attributeName != NULL && !strcmp(fAttribute, attributeName)) {
		if (key == NULL)
			return NO_MATCH;
		buffer = const_cast<uint8*>(key);
	} else if (!strcmp(fAttribute, "name")) {
		// if not, check for "fake" attributes ("name", "size", "last_modified")
		if (entry == NULL)
			return B_ERROR;
		buffer = (uint8*)QueryPolicy::EntryGetNameNoCopy(nodeHolder, entry);
		if (buffer == NULL)
			return B_ERROR;

		type = B_STRING_TYPE;
		size = strlen((const char*)buffer);
	} else if (!strcmp(fAttribute, "size")) {
		value.Int64 = QueryPolicy::NodeGetSize(node);
		type = B_INT64_TYPE;
	} else if (!strcmp(fAttribute, "last_modified")) {
		value.Int64 = QueryPolicy::NodeGetLastModifiedTime(node);
		type = B_INT64_TYPE;
	} else {
		// then for attributes
		size = bufferSize;
		if (QueryPolicy::NodeGetAttribute(nodeHolder, node,
				fAttribute, buffer, &size, &type) != B_OK) {
			return NO_MATCH;
		}
	}

	// prepare own value for use, if it is possible to convert it
	status_t status = ConvertValue(type);
	if (status == B_OK)
		status = CompareTo(buffer, size) ? MATCH_OK : NO_MATCH;

	QUERY_RETURN_ERROR(status);
}


template<typename QueryPolicy>
void
Equation<QueryPolicy>::CalculateScore(Index &index)
{
	// As always, these values could be tuned and refined.
	// And the code could also need some real world testing :-)

	// do we have to operate on a "foreign" index?
	if (Term<QueryPolicy>::fOp == OP_UNEQUAL
		|| QueryPolicy::IndexSetTo(index, fAttribute) < B_OK) {
		fScore = 0;
		return;
	}

	// if we have a pattern, how much does it help our search?
	if (fIsPattern) {
		fScore = getFirstPatternSymbol(fString) << 3;

		// Even if the first pattern symbol is at position 0,
		// there's still an index, so don't let our score revert to zero.
		if (fScore == 0)
			fScore = 1;
	} else {
		// Score by operator
		if (Term<QueryPolicy>::fOp == OP_EQUAL) {
			// higher than pattern="255 chars+*"
			fScore = 2048;
		} else {
			// the pattern search is regarded cheaper when you have at
			// least one character to set your index to
			fScore = 5;
		}
	}

	// take index size into account
	fScore = QueryPolicy::IndexGetWeightedScore(index, fScore);
}


template<typename QueryPolicy>
status_t
Equation<QueryPolicy>::PrepareQuery(Context* /*context*/, Index& index,
	IndexIterator** iterator, bool queryNonIndexed)
{
	status_t status = QueryPolicy::IndexSetTo(index, fAttribute);

	// if we should query attributes without an index, we can just proceed here
	if (status != B_OK && !queryNonIndexed)
		return B_ENTRY_NOT_FOUND;

	type_code type;

	// Special case for OP_UNEQUAL - it will always operate through the whole
	// index but we need the call to the original index to get the correct type
	if (status != B_OK || Term<QueryPolicy>::fOp == OP_UNEQUAL) {
		// Try to get an index that holds all files (name)
		// Also sets the default type for all attributes without index
		// to string.
		type = status < B_OK ? B_STRING_TYPE : QueryPolicy::IndexGetType(index);

		if (QueryPolicy::IndexSetTo(index, "name") != B_OK)
			return B_ENTRY_NOT_FOUND;

		fHasIndex = false;
	} else {
		fHasIndex = true;
		type = QueryPolicy::IndexGetType(index);
	}

	if (ConvertValue(type) < B_OK)
		return B_BAD_VALUE;

	*iterator = QueryPolicy::IndexCreateIterator(index);
	if (*iterator == NULL)
		return B_NO_MEMORY;

	if ((Term<QueryPolicy>::fOp == OP_EQUAL
			|| Term<QueryPolicy>::fOp == OP_GREATER_THAN
			|| Term<QueryPolicy>::fOp == OP_GREATER_THAN_OR_EQUAL || fIsPattern)
		&& fHasIndex) {
		// set iterator to the exact position

		int32 keySize = QueryPolicy::IndexGetKeySize(index);

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
				QUERY_RETURN_ERROR(B_ENTRY_NOT_FOUND);
		}

		status = QueryPolicy::IndexIteratorFind(*iterator, Value(), keySize);
		if (Term<QueryPolicy>::fOp == OP_EQUAL && !fIsPattern)
			return status;
		else if (status == B_ENTRY_NOT_FOUND
			&& (fIsPattern || Term<QueryPolicy>::fOp == OP_GREATER_THAN
				|| Term<QueryPolicy>::fOp == OP_GREATER_THAN_OR_EQUAL))
			return B_OK;

		QUERY_RETURN_ERROR(status);
	}

	return B_OK;
}


template<typename QueryPolicy>
status_t
Equation<QueryPolicy>::GetNextMatching(Context* context,
	IndexIterator* iterator, struct dirent* dirent, size_t bufferSize)
{
	while (true) {
		NodeHolder nodeHolder;
		union value<QueryPolicy> indexValue;
		size_t keyLength;
		size_t duplicate = 0;

		status_t status = QueryPolicy::IndexIteratorFetchNextEntry(iterator,
			&indexValue, &keyLength, (size_t)sizeof(indexValue), &duplicate);
		if (status != B_OK)
			return status;

		// only compare against the index entry when this is the correct
		// index for the equation
		if (fHasIndex && duplicate < 2 && !CompareTo((uint8*)&indexValue, keyLength)) {
			// They aren't equal? Let the operation decide what to do. Since
			// we always start at the beginning of the index (or the correct
			// position), only some needs to be stopped if the entry doesn't
			// fit.
			if (Term<QueryPolicy>::fOp == OP_LESS_THAN
				|| Term<QueryPolicy>::fOp == OP_LESS_THAN_OR_EQUAL
				|| (Term<QueryPolicy>::fOp == OP_EQUAL && !fIsPattern))
				return B_ENTRY_NOT_FOUND;

			if (duplicate > 0)
				QueryPolicy::IndexIteratorSkipDuplicates(iterator);
			continue;
		}

		Entry* entry = NULL;
		status = QueryPolicy::IndexIteratorGetEntry(context, iterator,
			nodeHolder, &entry);
		if (status != B_OK) {
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
		// node matches with the rest of the expression - we don't have to
		// check ||-operators for that
		Term<QueryPolicy>* term = this;
		status = MATCH_OK;

		if (!fHasIndex)
			status = Match(entry, QueryPolicy::EntryGetNode(entry));

		while (term != NULL && status == MATCH_OK) {
			Operator<QueryPolicy>* parent
				= (Operator<QueryPolicy>*)term->Parent();
			if (parent == NULL)
				break;

			if (parent->Op() == OP_AND) {
				// choose the other child of the parent
				Term<QueryPolicy>* other = parent->Right();
				if (other == term)
					other = parent->Left();

				if (other == NULL) {
					QUERY_FATAL("&&-operator has only one child... "
						"(parent = %p)\n", parent);
					break;
				}
				status = other->Match(entry, QueryPolicy::EntryGetNode(entry));
				if (status < 0) {
					QUERY_REPORT_ERROR(status);
					status = NO_MATCH;
				}
			}
			term = (Term<QueryPolicy>*)parent;
		}

		if (status == MATCH_OK) {
			ssize_t nameLength = QueryPolicy::EntryGetName(entry,
				dirent->d_name,
				(const char*)dirent + bufferSize - dirent->d_name);
			if (nameLength < 0) {
				// Invalid or unknown name.
				nameLength = 0;
			}

			dirent->d_dev = QueryPolicy::ContextGetVolumeID(context);
			dirent->d_ino = QueryPolicy::EntryGetNodeID(entry);
			dirent->d_pdev = dirent->d_dev;
			dirent->d_pino = QueryPolicy::EntryGetParentID(entry);
			dirent->d_reclen = offsetof(struct dirent, d_name) + nameLength;
		}

		if (status == MATCH_OK)
			return B_OK;
	}
	QUERY_RETURN_ERROR(B_ERROR);
}


template<typename QueryPolicy>
bool
Equation<QueryPolicy>::NeedsEntry()
{
	return strcmp(fAttribute, "name") == 0;
}


//	#pragma mark -


template<typename QueryPolicy>
Operator<QueryPolicy>::Operator(Term<QueryPolicy>* left, int8 op,
	Term<QueryPolicy>* right)
	:
	Term<QueryPolicy>(op),
	fLeft(left),
	fRight(right)
{
	if (left)
		left->SetParent(this);
	if (right)
		right->SetParent(this);
}


template<typename QueryPolicy>
Operator<QueryPolicy>::~Operator()
{
	delete fLeft;
	delete fRight;
}


template<typename QueryPolicy>
status_t
Operator<QueryPolicy>::Match(Entry* entry, Node* node, const char* attribute,
	int32 type, const uint8* key, size_t size)
{
	if (Term<QueryPolicy>::fOp == OP_AND) {
		status_t status = fLeft->Match(entry, node, attribute, type, key,
			size);
		if (status != MATCH_OK)
			return status;

		return fRight->Match(entry, node, attribute, type, key, size);
	} else {
		// choose the term with the better score for OP_OR
		Term<QueryPolicy>* first;
		Term<QueryPolicy>* second;
		if (fRight->Score() > fLeft->Score()) {
			first = fLeft;
			second = fRight;
		} else {
			first = fRight;
			second = fLeft;
		}

		status_t status = first->Match(entry, node, attribute, type, key,
			size);
		if (status != NO_MATCH)
			return status;

		return second->Match(entry, node, attribute, type, key, size);
	}
}


template<typename QueryPolicy>
void
Operator<QueryPolicy>::Complement()
{
	if (Term<QueryPolicy>::fOp == OP_AND)
		Term<QueryPolicy>::fOp = OP_OR;
	else
		Term<QueryPolicy>::fOp = OP_AND;

	fLeft->Complement();
	fRight->Complement();
}


template<typename QueryPolicy>
void
Operator<QueryPolicy>::CalculateScore(Index &index)
{
	fLeft->CalculateScore(index);
	fRight->CalculateScore(index);
}


template<typename QueryPolicy>
int32
Operator<QueryPolicy>::Score() const
{
	if (Term<QueryPolicy>::fOp == OP_AND) {
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


template<typename QueryPolicy>
status_t
Operator<QueryPolicy>::InitCheck()
{
	if ((Term<QueryPolicy>::fOp != OP_AND && Term<QueryPolicy>::fOp != OP_OR)
		|| fLeft == NULL || fLeft->InitCheck() < B_OK
		|| fRight == NULL || fRight->InitCheck() < B_OK)
		return B_ERROR;

	return B_OK;
}


template<typename QueryPolicy>
bool
Operator<QueryPolicy>::NeedsEntry()
{
	return ((fLeft && fLeft->NeedsEntry()) || (fRight && fRight->NeedsEntry()));
}


//	#pragma mark -

#ifdef DEBUG_QUERY

template<typename QueryPolicy>
void
Operator<QueryPolicy>::PrintToStream()
{
	QUERY_D(__out("( "));
	if (fLeft != NULL)
		fLeft->PrintToStream();

	const char* op;
	switch (Term<QueryPolicy>::fOp) {
		case OP_OR: op = "OR"; break;
		case OP_AND: op = "AND"; break;
		default: op = "?"; break;
	}
	QUERY_D(__out(" %s ", op));

	if (fRight != NULL)
		fRight->PrintToStream();

	QUERY_D(__out(" )"));
}


template<typename QueryPolicy>
void
Equation<QueryPolicy>::PrintToStream()
{
	const char* symbol = "???";
	switch (Term<QueryPolicy>::fOp) {
		case OP_EQUAL: symbol = "=="; break;
		case OP_UNEQUAL: symbol = "!="; break;
		case OP_GREATER_THAN: symbol = ">"; break;
		case OP_GREATER_THAN_OR_EQUAL: symbol = ">="; break;
		case OP_LESS_THAN: symbol = "<"; break;
		case OP_LESS_THAN_OR_EQUAL: symbol = "<="; break;
	}
	QUERY_D(__out("[\"%s\" %s \"%s\"]", fAttribute, symbol, fString));
}

#endif	// DEBUG_QUERY

//	#pragma mark -


template<typename QueryPolicy>
Expression<QueryPolicy>::Expression()
	:
	fTerm(NULL)
{
}


template<typename QueryPolicy>
status_t
Expression<QueryPolicy>::Init(const char* expr, const char** position)
{
	if (expr == NULL)
		return B_BAD_VALUE;
	if (fTerm != NULL)
		return EALREADY;

	status_t status = B_OK;
	int32 equations = 0;
	const int32 kMaxEquations = 32;

	struct ExpressionNode {
		Term<QueryPolicy>* term = NULL;
		bool negated = false;
		ops op = OP_NONE;
	};
	Stack<Stack<ExpressionNode>*> exprsTree;
	Stack<ExpressionNode>* currentExpr = NULL;
	ExpressionNode* current = NULL;
	while (expr != NULL) {
		skipWhitespace(&expr);

		if (currentExpr == NULL) {
			 currentExpr = new(std::nothrow) Stack<ExpressionNode>;
			 if (currentExpr == NULL) {
				 status = B_NO_MEMORY;
				 break;
			 }
		}

		const char c = *expr;
		bool complete = false;

		if (c == ')' || c == '\0') {
			if (currentExpr->IsEmpty())
				break; // Illegal empty expression.

			complete = true;
		}

		if (current == NULL && !complete) {
			currentExpr->Push(ExpressionNode());
			current = currentExpr->Array() + (currentExpr->CountItems() - 1);
		}

		if (c == '(') {
			exprsTree.Push(currentExpr);
			currentExpr = NULL;
			current = NULL;
			expr++;
		} else if (c == '!') {
			skipWhitespace(&expr, 1);
			if (*expr != '(')
				break; // Not allowed.
			current->negated = true;
		} else if (c == '|' || c == '&') {
			if (current->term == NULL)
				break; // Nothing to operate on.

			ops op = OP_NONE;
			if (IsOperator(&expr, '|'))
				op = OP_OR;
			else if (IsOperator(&expr, '&'))
				op = OP_AND;
			else
				break; // Illegal operator.

			current->op = op;
			current = NULL;
		} else if (!complete) {
			if (current->term != NULL)
				break; // There already is a term.
			if ((equations + 1) > kMaxEquations) {
				status = E2BIG;
				break;
			}

			Equation<QueryPolicy>* equation
				= new(std::nothrow) Equation<QueryPolicy>(&expr);
			if (equation == NULL) {
				status = B_NO_MEMORY;
				break;
			}
			if (equation == NULL || equation->InitCheck() != B_OK) {
				status = equation->InitCheck();
				delete equation;
				break;
			}

			current->term = equation;
			equations++;
		}
		if (!complete)
			continue;

		if (currentExpr->CountItems() == 1) {
			if (current == NULL)
				break; // Probably an anomalous operator.
		}

		// First pass: negation.
		for (int32 i = 0; i < currentExpr->CountItems(); i++) {
			current = currentExpr->Array() + i;
			// If the term is negated, we just complement the tree, to get
			// rid of the not, a.k.a. DeMorgan's Law.
			if (current->negated) {
				current->term->Complement();
				current->negated = false;
			}
		}
		// Second & third passes: && and ||.
		int32 nodes = currentExpr->CountItems();
		for (ops op = OP_AND; op <= OP_OR; op = (ops)(op + 1)) {
			for (int32 i = 0; i < (currentExpr->CountItems() - 1); ) {
				ExpressionNode* left = currentExpr->Array() + i;
				if (left->op != op) {
					i++;
					continue;
				}

				// Find the right-hand expression (may have to jump over now-unused nodes.)
				ExpressionNode* right = NULL;
				for (int32 j = i + 1; j < currentExpr->CountItems(); j++) {
					current = currentExpr->Array() + j;
					if (current->term == NULL)
						continue;

					right = current;
					break;
				}
				if (right == NULL)
					break; // Invalid expression, somehow.

				Term<QueryPolicy>* newTerm = new(std::nothrow) Operator<QueryPolicy>(
					left->term, left->op, right->term);
				if (newTerm == NULL) {
					status = B_NO_MEMORY;
					break;
				}

				left->term = newTerm;
				left->op = right->op;
				right->op = OP_NONE;
				right->term = NULL;
				nodes--;
			}
		}

		// At this point we should have only one node left.
		if (nodes != 1)
			break;

		current = currentExpr->Array();
		Term<QueryPolicy>* term = current->term;

		delete currentExpr;
		currentExpr = NULL;
		if (exprsTree.Pop(&currentExpr)) {
			current = currentExpr->Array() + (currentExpr->CountItems() - 1);
			if (current->term != NULL)
				break; // There already is a term.
			current->term = term;
		} else {
			if (c != '\0')
				break; // Unexpected end of expression.

			fTerm = term;
			break;
		}

		expr++;
	}

	if (position != NULL)
		*position = expr;

	do {
		if (currentExpr == NULL)
			continue;

		ExpressionNode item;
		while (currentExpr->Pop(&item))
			delete item.term;
		delete currentExpr;
	} while (exprsTree.Pop(&currentExpr));

	if (fTerm == NULL && status == B_OK)
		return B_BAD_VALUE;

	QUERY_D(if (fTerm != NULL) {
		fTerm->PrintToStream();
		QUERY_D(__out("\n"));
		if (*expr != '\0')
			PRINT(("Unexpected end of string: \"%s\"!\n", expr));
	});

	return status;
}


template<typename QueryPolicy>
Expression<QueryPolicy>::~Expression()
{
	delete fTerm;
}


template<typename QueryPolicy>
bool
Expression<QueryPolicy>::IsOperator(const char** expr, char op)
{
	const char* string = *expr;

	if (*string == op && *(string + 1) == op) {
		*expr += 2;
		return true;
	}
	return false;
}


//	#pragma mark -


template<typename QueryPolicy>
Query<QueryPolicy>::Query(Context* context, Expression<QueryPolicy>* expression,
	uint32 flags, port_id port, uint32 token)
	:
	fContext(context),
	fExpression(expression),
	fCurrent(NULL),
	fIterator(NULL),
	fIndex(context),
	fFlags(flags),
	fPort(port),
	fToken(token),
	fNeedsEntry(false)
{
	// If the expression has a valid root pointer, the whole tree has
	// already passed the sanity check, so that we don't have to check
	// every pointer
	if (context == NULL || expression == NULL || expression->Root() == NULL)
		return;

	// create index on the stack and delete it afterwards
	fExpression->Root()->CalculateScore(fIndex);
	QueryPolicy::IndexUnset(fIndex);

	fNeedsEntry = fExpression->Root()->NeedsEntry();

	Rewind();
}


template<typename QueryPolicy>
Query<QueryPolicy>::~Query()
{
	delete fExpression;
}


template<typename QueryPolicy>
/*static*/ status_t
Query<QueryPolicy>::Create(Context* context, const char* queryString,
	uint32 flags, port_id port, uint32 token, Query<QueryPolicy>*& _query)
{
	Expression<QueryPolicy>* expression
		= new(std::nothrow) Expression<QueryPolicy>;
	if (expression == NULL)
		QUERY_RETURN_ERROR(B_NO_MEMORY);

	const char* position = NULL;
	status_t status = expression->Init(queryString, &position);
	if (status != B_OK) {
		QUERY_INFORM("Could not parse query \"%s\", stopped at: \"%s\"\n",
			queryString, position);

		delete expression;
		QUERY_RETURN_ERROR(status);
	}

	Query<QueryPolicy>* query = new(std::nothrow) Query<QueryPolicy>(context,
		expression, flags, port, token);
	if (query == NULL) {
		delete expression;
		QUERY_RETURN_ERROR(B_NO_MEMORY);
	}

	_query = query;
	return B_OK;
}


template<typename QueryPolicy>
status_t
Query<QueryPolicy>::Rewind()
{
	// free previous stuff

	fStack.MakeEmpty();

	QueryPolicy::IndexIteratorDelete(fIterator);
	fIterator = NULL;
	fCurrent = NULL;

	// put the whole expression on the stack

	Stack<Term<QueryPolicy>*> stack;
	stack.Push(fExpression->Root());

	Term<QueryPolicy>* term;
	while (stack.Pop(&term)) {
		if (term->Op() < OP_EQUATION) {
			Operator<QueryPolicy>* op = (Operator<QueryPolicy>*)term;

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
			|| fStack.Push((Equation<QueryPolicy>*)term) != B_OK)
			QUERY_FATAL("Unknown term on stack or stack error\n");
	}

	return B_OK;
}


template<typename QueryPolicy>
status_t
Query<QueryPolicy>::GetNextEntry(struct dirent* dirent, size_t size)
{
	if (fIterator != NULL)
		QueryPolicy::IndexIteratorResume(fIterator);

	status_t error = _GetNextEntry(dirent, size);

	if (fIterator != NULL)
		QueryPolicy::IndexIteratorSuspend(fIterator);

	return error;
}


template<typename QueryPolicy>
void
Query<QueryPolicy>::LiveUpdate(Entry* entry, Node* node, const char* attribute,
	int32 type, const uint8* oldKey, size_t oldLength, const uint8* newKey,
	size_t newLength)
{
	if (fPort < 0 || fExpression == NULL || attribute == NULL)
		return;

	// TODO: check if the attribute is part of the query at all...

	// If no entry has been supplied, but the we need one for the evaluation
	// (i.e. the "name" attribute is used), we invoke ourselves for all entries
	// referring to the given node.
	if (entry == NULL && fNeedsEntry) {
		entry = QueryPolicy::NodeGetFirstReferrer(node);
		while (entry) {
			LiveUpdate(entry, node, attribute, type, oldKey, oldLength, newKey,
				newLength);
			entry = QueryPolicy::NodeGetNextReferrer(node, entry);
		}
		return;
	}

	status_t oldStatus = fExpression->Root()->Match(entry, node, attribute,
		type, oldKey, oldLength);
	status_t newStatus = fExpression->Root()->Match(entry, node, attribute,
		type, newKey, newLength);

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

	// notify query listeners
	status_t (*notify)(port_id, int32, dev_t, ino_t, const char*, ino_t);

	if (stillInQuery)
		notify = notify_query_attr_changed;
	else if (entryCreated)
		notify = notify_query_entry_created;
	else
		notify = notify_query_entry_removed;

	if (entry != NULL) {
		_SendEntryNotification(entry, notify);
	} else {
		entry = QueryPolicy::NodeGetFirstReferrer(node);
		while (entry) {
			_SendEntryNotification(entry, notify);
			entry = QueryPolicy::NodeGetNextReferrer(node, entry);
		}
	}
}


template<typename QueryPolicy>
void
Query<QueryPolicy>::LiveUpdateRenameMove(Entry* entry, Node* node,
	ino_t oldDirectoryID, const char* oldName, size_t oldLength,
	ino_t newDirectoryID, const char* newName, size_t newLength)
{
	if (fPort < 0 || fExpression == NULL)
		return;

	// TODO: check if the attribute is part of the query at all...

	status_t oldStatus = fExpression->Root()->Match(entry, node, "name",
		B_STRING_TYPE, (const uint8*)oldName, oldLength);
	status_t newStatus = fExpression->Root()->Match(entry, node, "name",
		B_STRING_TYPE, (const uint8*)newName, newLength);

	if (oldStatus != MATCH_OK || oldStatus != newStatus)
		return;

	// The entry stays in the query, notify query listeners about the rename
	// or move

	// We send a notification for the given entry, if any, or otherwise for
	// all entries referring to the node;
	if (entry != NULL) {
		_SendEntryNotification(entry, notify_query_entry_removed);
		_SendEntryNotification(entry, notify_query_entry_created);
	} else {
		entry = QueryPolicy::NodeGetFirstReferrer(node);
		while (entry) {
			_SendEntryNotification(entry, notify_query_entry_removed);
			_SendEntryNotification(entry, notify_query_entry_created);
			entry = QueryPolicy::NodeGetNextReferrer(node, entry);
		}
	}
}


template<typename QueryPolicy>
status_t
Query<QueryPolicy>::_GetNextEntry(struct dirent* dirent, size_t size)
{
	// If we don't have an equation to use yet/anymore, get a new one
	// from the stack
	while (true) {
		if (fIterator == NULL) {
			if (!fStack.Pop(&fCurrent)
				|| fCurrent == NULL)
				return B_ENTRY_NOT_FOUND;

			status_t status = fCurrent->PrepareQuery(fContext, fIndex,
				&fIterator, fFlags & B_QUERY_NON_INDEXED);
			if (status == B_ENTRY_NOT_FOUND) {
				// try next equation
				continue;
			}

			if (status != B_OK)
				return status;
		}
		if (fCurrent == NULL)
			QUERY_RETURN_ERROR(B_ERROR);

		status_t status = fCurrent->GetNextMatching(fContext, fIterator, dirent,
			size);
		if (status != B_OK) {
			QueryPolicy::IndexIteratorDelete(fIterator);
			fIterator = NULL;
			fCurrent = NULL;
		} else {
			// only return if we have another entry
			return B_OK;
		}
	}
}


template<typename QueryPolicy>
void
Query<QueryPolicy>::_SendEntryNotification(Entry* entry,
	status_t (*notify)(port_id, int32, dev_t, ino_t, const char*, ino_t))
{
	NodeHolder nodeHolder;
	const char* name = QueryPolicy::EntryGetNameNoCopy(nodeHolder, entry);
	if (name != NULL) {
		notify(fPort, fToken, QueryPolicy::ContextGetVolumeID(fContext),
			QueryPolicy::EntryGetParentID(entry), name,
			QueryPolicy::EntryGetNodeID(entry));
	}
}


}	// namespace QueryParser


#endif	// _FILE_SYSTEMS_QUERY_PARSER_H

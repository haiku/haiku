/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include <Query.h>

#include <fcntl.h>
#include <new>
#include <time.h>

#include <Entry.h>
#include <fs_query.h>
#include <parsedate.h>
#include <Volume.h>

#include <MessengerPrivate.h>
#include <syscalls.h>
#include <query_private.h>

#include "QueryPredicate.h"
#include "storage_support.h"


using namespace std;
using namespace BPrivate::Storage;


// Creates an uninitialized BQuery.
BQuery::BQuery()
	:
	BEntryList(),
	fStack(NULL),
	fPredicate(NULL),
	fDevice((dev_t)B_ERROR),
	fLive(false),
	fPort(B_ERROR),
	fToken(0),
	fQueryFd(-1)
{
}


// Frees all resources associated with the object.
BQuery::~BQuery()
{
	Clear();
}


// Resets the object to a uninitialized state.
status_t
BQuery::Clear()
{
	// close the currently open query
	status_t error = B_OK;
	if (fQueryFd >= 0) {
		error = _kern_close(fQueryFd);
		fQueryFd = -1;
	}
	// delete the predicate stack and the predicate
	delete fStack;
	fStack = NULL;
	delete[] fPredicate;
	fPredicate = NULL;
	// reset the other parameters
	fDevice = (dev_t)B_ERROR;
	fLive = false;
	fPort = B_ERROR;
	fToken = 0;
	return error;
}


// Pushes an attribute name onto the predicate stack.
status_t
BQuery::PushAttr(const char* attrName)
{
	return _PushNode(new(nothrow) AttributeNode(attrName), true);
}


// Pushes an operator onto the predicate stack.
status_t
BQuery::PushOp(query_op op)
{
	status_t error = B_OK;
	switch (op) {
		case B_EQ:
		case B_GT:
		case B_GE:
		case B_LT:
		case B_LE:
		case B_NE:
		case B_CONTAINS:
		case B_BEGINS_WITH:
		case B_ENDS_WITH:
		case B_AND:
		case B_OR:
			error = _PushNode(new(nothrow) BinaryOpNode(op), true);
			break;
		case B_NOT:
			error = _PushNode(new(nothrow) UnaryOpNode(op), true);
			break;
		default:
			error = _PushNode(new(nothrow) SpecialOpNode(op), true);
			break;
	}
	return error;
}


// Pushes a uint32 onto the predicate stack.
status_t
BQuery::PushUInt32(uint32 value)
{
	return _PushNode(new(nothrow) UInt32ValueNode(value), true);
}


// Pushes an int32 onto the predicate stack.
status_t
BQuery::PushInt32(int32 value)
{
	return _PushNode(new(nothrow) Int32ValueNode(value), true);
}


// Pushes a uint64 onto the predicate stack.
status_t
BQuery::PushUInt64(uint64 value)
{
	return _PushNode(new(nothrow) UInt64ValueNode(value), true);
}


// Pushes an int64 onto the predicate stack.
status_t
BQuery::PushInt64(int64 value)
{
	return _PushNode(new(nothrow) Int64ValueNode(value), true);
}


// Pushes a float onto the predicate stack.
status_t
BQuery::PushFloat(float value)
{
	return _PushNode(new(nothrow) FloatValueNode(value), true);
}


// Pushes a double onto the predicate stack.
status_t
BQuery::PushDouble(double value)
{
	return _PushNode(new(nothrow) DoubleValueNode(value), true);
}


// Pushes a string onto the predicate stack.
status_t
BQuery::PushString(const char* value, bool caseInsensitive)
{
	return _PushNode(new(nothrow) StringNode(value, caseInsensitive), true);
}


// Pushes a date onto the predicate stack.
status_t
BQuery::PushDate(const char* date)
{
	if (date == NULL || !date[0] || parsedate(date, time(NULL)) < 0)
		return B_BAD_VALUE;

	return _PushNode(new(nothrow) DateNode(date), true);
}


// Assigns a volume to the BQuery object.
status_t
BQuery::SetVolume(const BVolume* volume)
{
	if (volume == NULL)
		return B_BAD_VALUE;
	if (_HasFetched())
		return B_NOT_ALLOWED;

	if (volume->InitCheck() == B_OK)
		fDevice = volume->Device();
	else
		fDevice = (dev_t)B_ERROR;

	return B_OK;
}


// Assigns the passed-in predicate expression.
status_t
BQuery::SetPredicate(const char* expression)
{
	status_t error = (expression ? B_OK : B_BAD_VALUE);
	if (error == B_OK && _HasFetched())
		error = B_NOT_ALLOWED;
	if (error == B_OK)
		error = _SetPredicate(expression);
	return error;
}


// Assigns the target messenger and makes the query live.
status_t
BQuery::SetTarget(BMessenger messenger)
{
	status_t error = (messenger.IsValid() ? B_OK : B_BAD_VALUE);
	if (error == B_OK && _HasFetched())
		error = B_NOT_ALLOWED;
	if (error == B_OK) {
		BMessenger::Private messengerPrivate(messenger);
		fPort = messengerPrivate.Port();
		fToken = (messengerPrivate.IsPreferredTarget()
			? -1 : messengerPrivate.Token());
		fLive = true;
	}
	return error;
}


// Gets whether the query associated with this object is live.
bool
BQuery::IsLive() const
{
	return fLive;
}


// Fills out buffer with the predicate string assigned to the BQuery object.
status_t
BQuery::GetPredicate(char* buffer, size_t length)
{
	status_t error = (buffer ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		_EvaluateStack();
	if (error == B_OK && !fPredicate)
		error = B_NO_INIT;
	if (error == B_OK && length <= strlen(fPredicate))
		error = B_BAD_VALUE;
	if (error == B_OK)
		strcpy(buffer, fPredicate);
	return error;
}


// Fills out the passed-in BString object with the predicate string
// assigned to the BQuery object.
status_t
BQuery::GetPredicate(BString* predicate)
{
	status_t error = (predicate ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		_EvaluateStack();
	if (error == B_OK && !fPredicate)
		error = B_NO_INIT;
	if (error == B_OK)
		predicate->SetTo(fPredicate);
	return error;
}


// Gets the length of the predicate string.
size_t
BQuery::PredicateLength()
{
	status_t error = _EvaluateStack();
	if (error == B_OK && !fPredicate)
		error = B_NO_INIT;
	size_t size = 0;
	if (error == B_OK)
		size = strlen(fPredicate) + 1;
	return size;
}


// Gets the device ID identifying the volume of the BQuery object.
dev_t
BQuery::TargetDevice() const
{
	return fDevice;
}


// Start fetching entries satisfying the predicate.
status_t
BQuery::Fetch()
{
	if (_HasFetched())
		return B_NOT_ALLOWED;

	_EvaluateStack();

	if (!fPredicate || fDevice < 0)
		return B_NO_INIT;

	BString parsedPredicate;
	_ParseDates(parsedPredicate);

	fQueryFd = _kern_open_query(fDevice, parsedPredicate.String(),
		parsedPredicate.Length(), fLive ? B_LIVE_QUERY : 0, fPort, fToken);
	if (fQueryFd < 0)
		return fQueryFd;

	// set close on exec flag
	fcntl(fQueryFd, F_SETFD, FD_CLOEXEC);

	return B_OK;
}


//	#pragma mark - BEntryList interface


// Fills out entry with the next entry traversing symlinks if traverse is true.
status_t
BQuery::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		entry_ref ref;
		error = GetNextRef(&ref);
		if (error == B_OK)
			error = entry->SetTo(&ref, traverse);
	}
	return error;
}


// Fills out ref with the next entry as an entry_ref.
status_t
BQuery::GetNextRef(entry_ref* ref)
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK && !_HasFetched())
		error = B_FILE_ERROR;
	if (error == B_OK) {
		BPrivate::Storage::LongDirEntry entry;
		bool next = true;
		while (error == B_OK && next) {
			if (GetNextDirents(&entry, sizeof(entry), 1) != 1) {
				error = B_ENTRY_NOT_FOUND;
			} else {
				next = (!strcmp(entry.d_name, ".")
						|| !strcmp(entry.d_name, ".."));
			}
		}
		if (error == B_OK) {
			ref->device = entry.d_pdev;
			ref->directory = entry.d_pino;
			error = ref->set_name(entry.d_name);
		}
	}
	return error;
}


// Fill out up to count entries into the array of dirent structs pointed
// to by buffer.
int32
BQuery::GetNextDirents(struct dirent* buffer, size_t length, int32 count)
{
	if (!buffer)
		return B_BAD_VALUE;
	if (!_HasFetched())
		return B_FILE_ERROR;
	return _kern_read_dir(fQueryFd, buffer, length, count);
}


// Rewinds the entry list back to the first entry.
status_t
BQuery::Rewind()
{
	if (!_HasFetched())
		return B_FILE_ERROR;
	return _kern_rewind_dir(fQueryFd);
}


// Unimplemented method of the BEntryList interface.
int32
BQuery::CountEntries()
{
	return B_ERROR;
}


// Gets whether Fetch() has already been called on this object.
bool
BQuery::_HasFetched() const
{
	return fQueryFd >= 0;
}


// Pushes a node onto the predicate stack.
status_t
BQuery::_PushNode(QueryNode* node, bool deleteOnError)
{
	status_t error = (node ? B_OK : B_NO_MEMORY);
	if (error == B_OK && _HasFetched())
		error = B_NOT_ALLOWED;
	// allocate the stack, if necessary
	if (error == B_OK && !fStack) {
		fStack = new(nothrow) QueryStack;
		if (!fStack)
			error = B_NO_MEMORY;
	}
	if (error == B_OK)
		error = fStack->PushNode(node);
	if (error != B_OK && deleteOnError)
		delete node;
	return error;
}


// Helper method to set the predicate.
status_t
BQuery::_SetPredicate(const char* expression)
{
	status_t error = B_OK;
	// unset the old predicate
	delete[] fPredicate;
	fPredicate = NULL;
	// set the new one
	if (expression) {
		fPredicate = new(nothrow) char[strlen(expression) + 1];
		if (fPredicate)
			strcpy(fPredicate, expression);
		else
			error = B_NO_MEMORY;
	}
	return error;
}


// Evaluates the predicate stack.
status_t
BQuery::_EvaluateStack()
{
	status_t error = B_OK;
	if (fStack) {
		_SetPredicate(NULL);
		if (_HasFetched())
			error = B_NOT_ALLOWED;
		// convert the stack to a tree and evaluate it
		QueryNode* node = NULL;
		if (error == B_OK)
			error = fStack->ConvertToTree(node);
		BString predicate;
		if (error == B_OK)
			error = node->GetString(predicate);
		if (error == B_OK)
			error = _SetPredicate(predicate.String());
		delete fStack;
		fStack = NULL;
	}
	return error;
}


void
BQuery::_ParseDates(BString& parsedPredicate)
{
	const char* start = fPredicate;
	const char* pos = start;
	bool quotes = false;

	while (pos[0]) {
		if (pos[0] == '\\') {
			pos++;
			continue;
		}
		if (pos[0] == '"')
			quotes = !quotes;
		else if (!quotes && pos[0] == '%') {
			const char* end = strchr(pos + 1, '%');
			if (end == NULL)
				continue;

			parsedPredicate.Append(start, pos - start);
			start = end + 1;

			// We have a date string
			BString date(pos + 1, start - 1 - pos);
			parsedPredicate << parsedate(date.String(), time(NULL));

			pos = end;
		}
		pos++;
	}

	parsedPredicate.Append(start, pos - start);
}


// FBC
void BQuery::_QwertyQuery1() {}
void BQuery::_QwertyQuery2() {}
void BQuery::_QwertyQuery3() {}
void BQuery::_QwertyQuery4() {}
void BQuery::_QwertyQuery5() {}
void BQuery::_QwertyQuery6() {}


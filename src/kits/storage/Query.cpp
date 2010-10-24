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


/*!	\brief Creates an uninitialized BQuery.
*/
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


/*!	\brief Frees all resources associated with the object.
*/
BQuery::~BQuery()
{
	Clear();
}


/*!	\brief Resets the object to a uninitialized state.
	\return \c B_OK
*/
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


/*!	\brief Pushes an attribute name onto the BQuery's predicate stack.
	\param attrName the attribute name
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushAttribute() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushAttr(const char* attrName)
{
	return _PushNode(new(nothrow) AttributeNode(attrName), true);
}


/*!	\brief Pushes an operator onto the BQuery's predicate stack.
	\param op the code representing the operator
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushOp() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
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


/*!	\brief Pushes a uint32 value onto the BQuery's predicate stack.
	\param value the value
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushUInt32() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushUInt32(uint32 value)
{
	return _PushNode(new(nothrow) UInt32ValueNode(value), true);
}


/*!	\brief Pushes an int32 value onto the BQuery's predicate stack.
	\param value the value
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushInt32() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushInt32(int32 value)
{
	return _PushNode(new(nothrow) Int32ValueNode(value), true);
}


/*!	\brief Pushes a uint64 value onto the BQuery's predicate stack.
	\param value the value
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushUInt64() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushUInt64(uint64 value)
{
	return _PushNode(new(nothrow) UInt64ValueNode(value), true);
}


/*!	\brief Pushes an int64 value onto the BQuery's predicate stack.
	\param value the value
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushInt64() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushInt64(int64 value)
{
	return _PushNode(new(nothrow) Int64ValueNode(value), true);
}


/*!	\brief Pushes a float value onto the BQuery's predicate stack.
	\param value the value
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushFloat() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushFloat(float value)
{
	return _PushNode(new(nothrow) FloatValueNode(value), true);
}


/*!	\brief Pushes a double value onto the BQuery's predicate stack.
	\param value the value
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushDouble() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushDouble(double value)
{
	return _PushNode(new(nothrow) DoubleValueNode(value), true);
}


/*!	\brief Pushes a string value onto the BQuery's predicate stack.
	\param value the value
	\param caseInsensitive \c true, if the case of the string should be
		   ignored, \c false otherwise
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Not enough memory.
	- \c B_NOT_ALLOWED: PushString() was called after Fetch().
	\note In BeOS R5 this method returns \c void. That is checking the return
		  value will render your code source and binary incompatible!
		  Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushString(const char* value, bool caseInsensitive)
{
	return _PushNode(new(nothrow) StringNode(value, caseInsensitive), true);
}


/*!	\brief Pushes a date value onto the BQuery's predicate stack.
	The supplied date can be any string understood by the POSIX function
	parsedate().
	\param date the date string
	\return
	- \c B_OK: Everything went fine.
	- \c B_ERROR: Error parsing the string.
	- \c B_NOT_ALLOWED: PushDate() was called after Fetch().
	\note Calling PushXYZ() after a Fetch() does change the predicate on R5,
		  but it doesn't affect the active query and the newly created
		  predicate can not even be used for the next query, since in order
		  to be able to reuse the BQuery object for another query, Clear() has
		  to be called and Clear() also deletes the predicate.
*/
status_t
BQuery::PushDate(const char* date)
{
	if (date == NULL || !date[0] || parsedate(date, time(NULL)) < 0)
		return B_BAD_VALUE;

	return _PushNode(new(nothrow) DateNode(date), true);
}


/*!	\brief Sets the BQuery's volume.
	A query is restricted to one volume. This method sets this volume. It
	fails, if called after Fetch(). To reuse a BQuery object it has to be
	reset via Clear().
	\param volume the volume
	\return
	- \c B_OK: Everything went fine.
	- \c B_NOT_ALLOWED: SetVolume() was called after Fetch().
*/
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


/*!	\brief Sets the BQuery's predicate.
	A predicate can be set either using this method or constructing one on
	the predicate stack. The two methods can not be mixed. The letter one
	has precedence over this one.
	The method fails, if called after Fetch(). To reuse a BQuery object it has
	to be reset via Clear().
	\param predicate the predicate string
	\return
	- \c B_OK: Everything went fine.
	- \c B_NOT_ALLOWED: SetPredicate() was called after Fetch().
	- \c B_NO_MEMORY: Insufficient memory to store the predicate.
*/
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


/*!	\brief Sets the BQuery's target and makes the query live.
	The query update messages are sent to the specified target. They might
	roll in immediately after calling Fetch().
	This methods fails, if called after Fetch(). To reuse a BQuery object it
	has to be reset via Clear().
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \a messenger was not properly initialized.
	- \c B_NOT_ALLOWED: SetTarget() was called after Fetch().
*/
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


/*!	\brief Returns whether the query associated with this object is live.
	\return \c true, if the query is live, \c false otherwise
*/
bool
BQuery::IsLive() const
{
	return fLive;
}


/*!	\brief Returns the BQuery's predicate.
	Regardless of whether the predicate has been constructed using the
	predicate stack or set via SetPredicate(), this method returns a
	string representation.
	\param buffer a pointer to a buffer into which the predicate shall be
		   written
	\param length the size of the provided buffer
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The predicate isn't set.
	- \c B_BAD_VALUE: \a buffer is \c NULL or too short.
	\note This method causes the predicate stack to be evaluated and cleared.
		  You can't interleave Push*() and GetPredicate() calls.
*/
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


/*!	\brief Returns the BQuery's predicate.
	Regardless of whether the predicate has been constructed using the
	predicate stack or set via SetPredicate(), this method returns a
	string representation.
	\param predicate a pointer to a BString which shall be set to the
		   predicate string
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The predicate isn't set.
	- \c B_BAD_VALUE: \c NULL \a predicate.
	\note This method causes the predicate stack to be evaluated and cleared.
		  You can't interleave Push*() and GetPredicate() calls.
*/
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


/*!	\brief Returns the length of the BQuery's predicate string.
	Regardless of whether the predicate has been constructed using the
	predicate stack or set via SetPredicate(), this method returns the length
	of its string representation (counting the terminating null).
	\return
	- the length of the predicate string (counting the terminating null) or
	- 0, if an error occured
	\note This method causes the predicate stack to be evaluated and cleared.
		  You can't interleave Push*() and PredicateLength() calls.
*/
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


/*!	\brief Returns the device ID identifying the BQuery's volume.
	\return the device ID of the BQuery's volume or \c B_NO_INIT, if the
			volume isn't set.
*/
dev_t
BQuery::TargetDevice() const
{
	return fDevice;
}


/*!	\brief Tells the BQuery to start fetching entries satisfying the predicate.
	After Fetch() has been called GetNextEntry(), GetNextRef() and
	GetNextDirents() can be used to retrieve the enties. Live query updates
	may be sent immediately after this method has been called.
	Fetch() fails, if it has already been called. To reuse a BQuery object it
	has to be reset via Clear().
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The predicate or the volume aren't set.
	- \c B_BAD_VALUE: The predicate is invalid.
	- \c B_NOT_ALLOWED: Fetch() has already been called.
*/
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


/*!	\brief Returns the BQuery's next entry as a BEntry.
	Places the next entry in the list in \a entry, traversing symlinks if
	\a traverse is \c true.
	\param entry a pointer to a BEntry to be initialized with the found entry
	\param traverse specifies whether to follow it, if the found entry
		   is a symbolic link.
	\note The iterator used by this method is the same one used by
		  GetNextRef() and GetNextDirents().
	\return
	- \c B_OK if successful,
	- \c B_ENTRY_NOT_FOUND when at the end of the list,
	- \c B_BAD_VALUE: The queries predicate includes unindexed attributes.
	- \c B_FILE_ERROR: Fetch() has not been called before.
*/
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


/*!	\brief Returns the BQuery's next entry as an entry_ref.
	Places an entry_ref to the next entry in the list into \a ref.
	\param ref a pointer to an entry_ref to be filled in with the data of the
		   found entry
	\note The iterator used by this method is the same one used by
		  GetNextEntry() and GetNextDirents().
	\return
	- \c B_OK if successful,
	- \c B_ENTRY_NOT_FOUND when at the end of the list,
	- \c B_BAD_VALUE: The queries predicate includes unindexed attributes.
	- \c B_FILE_ERROR: Fetch() has not been called before.
*/
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


/*!	\brief Returns the BQuery's next entries as dirent structures.
	Reads a number of entries into the array of dirent structures pointed to by
	\a buf. Reads as many but no more than \a count entries, as many entries as
	remain, or as many entries as will fit into the array at \a buf with given
	length \a length (in bytes), whichever is smallest.
	\param buf a pointer to a buffer to be filled with dirent structures of
		   the found entries
	\param length the maximal number of entries to be read.
	\note The iterator used by this method is the same one used by
		  GetNextEntry() and GetNextRef().
	\return
	- The number of dirent structures stored in the buffer, 0 when there are
	  no more entries to be read.
	- \c B_BAD_VALUE: The queries predicate includes unindexed attributes.
	- \c B_FILE_ERROR: Fetch() has not been called before.
*/
int32
BQuery::GetNextDirents(struct dirent* buffer, size_t length, int32 count)
{
	if (!buffer)
		return B_BAD_VALUE;
	if (!_HasFetched())
		return B_FILE_ERROR;
	return _kern_read_dir(fQueryFd, buffer, length, count);
}


/*!	\brief Rewinds the entry list back to the first entry.

	Unlike R5 Haiku implements this method for BQuery.

	\return
	- \c B_OK on success,
	- \c B_FILE_ERROR, if Fetch() has not yet been called.
*/
status_t
BQuery::Rewind()
{
	if (!_HasFetched())
		return B_FILE_ERROR;
	return _kern_rewind_dir(fQueryFd);
}


/*!	\brief Unimplemented method of the BEntryList interface.
	\return 0.
*/
int32
BQuery::CountEntries()
{
	return B_ERROR;
}


/*!	Returns whether Fetch() has already been called on this object.
	\return \c true, if Fetch() has successfully been invoked, \c false
			otherwise.
*/
bool
BQuery::_HasFetched() const
{
	return fQueryFd >= 0;
}


/*!	\brief Pushs a node onto the predicate stack.
	If the stack has not been allocate until this time, this method does
	allocate it.
	If the supplied node is \c NULL, it is assumed that there was not enough
	memory to allocate the node and thus \c B_NO_MEMORY is returned.
	In case the method fails, the caller retains the ownership of the supplied
	node and thus is responsible for deleting it, if \a deleteOnError is
	\c false. If it is \c true, the node is deleted, if an error occurs.
	\param node the node to be pushed
	\param deleteOnError
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: \c NULL \a node or insuffient memory to allocate the
	  predicate stack or push the node.
	- \c B_NOT_ALLOWED: _PushNode() was called after Fetch().
*/
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


/*!	\brief Helper method to set the BQuery's predicate.
	It is not checked whether Fetch() has already been invoked.
	\param predicate the predicate string
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Insufficient memory to store the predicate.
*/
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


/*!	Evaluates the query's predicate stack.
	The method does nothing (and returns \c B_OK), if the stack is \c NULL.
	If the stack is non-null and Fetch() has already been called, the method
	fails.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Insufficient memory.
	- \c B_NOT_ALLOWED: _EvaluateStack() was called after Fetch().
	- another error code
*/
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


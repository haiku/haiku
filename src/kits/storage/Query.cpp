//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Query.cpp
	BQuery implementation.
*/
#include <Query.h>

#include <fs_query.h>
#include <new.h>
#include <parsedate.h>
#include <time.h>

#include <Entry.h>
#include <Volume.h>

#include "kernel_interface.h"
#include "QueryPredicate.h"

using namespace StorageKit;

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};


// ===========================================================================
// Hack to get a BMessenger's port and token.

class _TRoster_ {
public:
	static inline void get_messenger_port_token(const BMessenger &messenger,
												port_id &port, int32 &token)
	{
		port = messenger.fPort;
		token = messenger.fHandlerToken;
	}
};
// ===========================================================================


// BQuery

// constructor
/*!	\brief Creates an uninitialized BQuery.
*/
BQuery::BQuery()
	  : BEntryList(),
		fStack(NULL),
		fPredicate(NULL),
		fDevice(B_ERROR),
		fLive(false),
		fPort(B_ERROR),
		fToken(0),
		fQueryFd(NullFd)
{
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
BQuery::~BQuery()
{
	Clear();
}

// Clear
/*!	\brief Resets the object to a uninitialized state.
	\return \c B_OK
*/
status_t
BQuery::Clear()
{
	// close the currently open query
	status_t error = B_OK;
	if (fQueryFd != NullFd) {
		error = close_query(fQueryFd);
		fQueryFd = NullFd;
	}
	// delete the predicate stack and the predicate
	delete fStack;
	fStack = NULL;
	delete[] fPredicate;
	fPredicate = NULL;
	// reset the other parameters
	fDevice = B_ERROR;
	fLive = false;
	fPort = B_ERROR;
	fToken = 0;
	return error;
}

// PushAttr
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
BQuery::PushAttr(const char *attrName)
{
	return _PushNode(new(nothrow) AttributeNode(attrName), true);
}

// PushOp
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

// PushUInt32
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

// PushInt32
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

// PushUInt64
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

// PushInt64
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

// PushFloat
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

// PushDouble
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

// PushString
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
BQuery::PushString(const char *value, bool caseInsensitive)
{
	return _PushNode(new(nothrow) StringNode(value, caseInsensitive), true);
}

// PushDate
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
BQuery::PushDate(const char *date)
{
	status_t error = (date ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		time_t t;
		time(&t);
		t = parsedate(date, t);
		if (t < 0) {
//			error = t;
			error = B_BAD_VALUE;
		}
	}
	if (error == B_OK)
		error =  _PushNode(new(nothrow) DateNode(date), true);
	return error;
}

// SetVolume
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
BQuery::SetVolume(const BVolume *volume)
{
	status_t error = (volume ? B_OK : B_BAD_VALUE);
	if (error == B_OK && _HasFetched())
		error = B_NOT_ALLOWED;
	if (error == B_OK) {
		if (volume->InitCheck() == B_OK)
			fDevice = volume->Device();
		else
			fDevice = B_ERROR;
	}
	return error;
}

// SetPredicate
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
BQuery::SetPredicate(const char *expression)
{
	status_t error = (expression ? B_OK : B_BAD_VALUE);
	if (error == B_OK && _HasFetched())
		error = B_NOT_ALLOWED;
	if (error == B_OK)
		error = _SetPredicate(expression);
	return error;
}

// SetTarget
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
		_TRoster_::get_messenger_port_token(messenger, fPort, fToken);
		fLive = true;
	}
	return error;
}

// IsLive
/*!	\brief Returns whether the query associated with this object is live.
	\return \c true, if the query is live, \c false otherwise
*/
bool
BQuery::IsLive() const
{
	return fLive;
}

// GetPredicate
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
BQuery::GetPredicate(char *buffer, size_t length)
{
	status_t error = (buffer ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
//		error = _EvaluateStack();
		_EvaluateStack();
	if (error == B_OK && !fPredicate)
		error = B_NO_INIT;
	if (error == B_OK && length <= strlen(fPredicate))
		error = B_BAD_VALUE;
	if (error == B_OK)
		strcpy(buffer, fPredicate);
	return error;
}

// GetPredicate
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
BQuery::GetPredicate(BString *predicate)
{
	status_t error = (predicate ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
//		error = _EvaluateStack();
		_EvaluateStack();
	if (error == B_OK && !fPredicate)
		error = B_NO_INIT;
	if (error == B_OK)
		predicate->SetTo(fPredicate);
	return error;
}

// PredicateLength
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

// TargetDevice
/*!	\brief Returns the device ID identifying the BQuery's volume.
	\return the device ID of the BQuery's volume or \c B_NO_INIT, if the
			volume isn't set.
	
*/
dev_t
BQuery::TargetDevice() const
{
	return fDevice;
}

// Fetch
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
	status_t error = (_HasFetched() ? B_NOT_ALLOWED : B_OK);
	if (error == B_OK)
//		error = _EvaluateStack();
		_EvaluateStack();
	if (error == B_OK && (!fPredicate || fDevice < 0))
		error = B_NO_INIT;
	if (error == B_OK) {
		if (fLive) {
			error = open_live_query(fDevice, fPredicate, B_LIVE_QUERY, fPort,
									fToken, fQueryFd);
		} else
			error = open_query(fDevice, fPredicate, 0, fQueryFd);
	}
	return error;
}


// BEntryList interface

// GetNextEntry
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
BQuery::GetNextEntry(BEntry *entry, bool traverse)
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

// GetNextRef
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
BQuery::GetNextRef(entry_ref *ref)
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK && !_HasFetched())
		error = B_FILE_ERROR;
	if (error == B_OK) {
		StorageKit::LongDirEntry entry;
		if (StorageKit::read_query(fQueryFd, &entry, sizeof(entry), 1) != 1)
			error = B_ENTRY_NOT_FOUND;
		if (error == B_OK)
			*ref = entry_ref(entry.d_pdev, entry.d_pino, entry.d_name);
	}
	return error;
}

// GetNextDirents
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
BQuery::GetNextDirents(struct dirent *buf, size_t length, int32 count)
{
	int32 result = (buf ? B_OK : B_BAD_VALUE);
	if (result == B_OK && !_HasFetched())
		result = B_FILE_ERROR;
	if (result == B_OK)
		result = read_query(fQueryFd, buf, length, count);
	return result;
}

// Rewind
/*!	\brief Unimplemented method of the BEntryList interface.
	\return \c B_ERROR.
*/
status_t
BQuery::Rewind()
{
	return B_ERROR;
}

// CountEntries
/*!	\brief Unimplemented method of the BEntryList interface.
	\return 0.
*/
int32
BQuery::CountEntries()
{
	return B_ERROR;
}

// _HasFetched
/*!	Returns whether Fetch() has already been called on this object.
	\return \c true, if Fetch() has successfully been invoked, \c false
			otherwise.
*/
bool
BQuery::_HasFetched() const
{
	return (fQueryFd != NullFd);
}

// _PushNode
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
BQuery::_PushNode(QueryNode *node, bool deleteOnError)
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

// _SetPredicate
/*!	\brief Helper method to set the BQuery's predicate.
	It is not checked whether Fetch() has already been invoked.
	\param predicate the predicate string
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Insufficient memory to store the predicate.
*/
status_t
BQuery::_SetPredicate(const char *expression)
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

// _EvaluateStack
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
		QueryNode *node = NULL;
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


// FBC
void BQuery::_ReservedQuery1() {}
void BQuery::_ReservedQuery2() {}
void BQuery::_ReservedQuery3() {}
void BQuery::_ReservedQuery4() {}
void BQuery::_ReservedQuery5() {}
void BQuery::_ReservedQuery6() {}

//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Query.h
	BQuery interface declaration.
*/

#ifndef _sk_query_h_
#define _sk_query_h_

#include <EntryList.h>
#include <Messenger.h>
#include <OS.h>
#include <SupportDefs.h>

#include <StorageDefs.Private.h>

class BVolume;
struct entry_ref;

namespace StorageKit {
	class QueryNode;
	class QueryStack;
	class QueryTree;
};

typedef enum {
	B_INVALID_OP	= 0,
	B_EQ,
	B_GT,
	B_GE,
	B_LT,
	B_LE,
	B_NE,
	B_CONTAINS,
	B_BEGINS_WITH,
	B_ENDS_WITH,
	B_AND			= 0x101,
	B_OR,
	B_NOT,
	_B_RESERVED_OP_	= 0x100000
} query_op;

/*!
	\class BQuery
	\brief Represents a live or non-live file system query
	
	Provides an interface for creating file system queries. Implements
	the BEntryList for iterating through the found entries.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/
class BQuery : public BEntryList {
public:
	BQuery();
	virtual ~BQuery();

	status_t Clear();

	status_t PushAttr(const char *attrName);
	status_t PushOp(query_op op);

	status_t PushUInt32(uint32 value);
	status_t PushInt32(int32 value);
	status_t PushUInt64(uint64 value);
	status_t PushInt64(int64 value);
	status_t PushFloat(float value);
	status_t PushDouble(double value);
	status_t PushString(const char *value, bool caseInsensitive = false);
	status_t PushDate(const char *date);

	status_t SetVolume(const BVolume *volume);
	status_t SetPredicate(const char *expression);
	status_t SetTarget(BMessenger messenger);

	bool IsLive() const;

	status_t GetPredicate(char *buffer, size_t length);
	status_t GetPredicate(BString *predicate);
	size_t PredicateLength();

	dev_t TargetDevice() const;

	status_t Fetch();

	// BEntryList interface
	virtual status_t GetNextEntry(BEntry *entry, bool traverse = false);
	virtual status_t GetNextRef(entry_ref *ref);
	virtual int32 GetNextDirents(struct dirent *buf, size_t length,
								 int32 count = INT_MAX);
	virtual status_t Rewind();
	virtual int32 CountEntries();

private:
	bool _HasFetched() const;
	status_t _PushNode(StorageKit::QueryNode *node, bool deleteOnError);
	status_t _SetPredicate(const char *expression);
	status_t _EvaluateStack();

	// FBC
	virtual void _ReservedQuery1();
	virtual void _ReservedQuery2();
	virtual void _ReservedQuery3();
	virtual void _ReservedQuery4();
	virtual void _ReservedQuery5();
	virtual void _ReservedQuery6();

private:
	int32		_reservedData[4];	// FBC
	StorageKit::QueryStack *fStack;
	char		*fPredicate;
	dev_t		fDevice;
	bool		fLive;
	port_id		fPort;
	long		fToken;
	StorageKit::FileDescriptor fQueryFd;
};

#endif	// _sk_query_h_

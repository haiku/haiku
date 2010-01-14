// NodeHandle.h

#ifndef NET_FS_NODE_HANDLE_H
#define NET_FS_NODE_HANDLE_H

#include <dirent.h>

#include <Query.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

#include "Lockable.h"
#include "NodeRef.h"

class Directory;
class Entry;
class Node;

// NodeHandle
class NodeHandle : public BReferenceable, public Lockable {
public:
								NodeHandle();
	virtual						~NodeHandle();

	virtual	status_t			GetStat(struct stat* st);

			void				SetCookie(int32 cookie);
			int32				GetCookie() const;

			const NodeRef&		GetNodeRef() const;

protected:
	virtual	int					GetFD() const;

protected:
			int32				fCookie;
			NodeRef				fNodeRef;
};

// FileHandle
class FileHandle : public NodeHandle {
public:
								FileHandle();
	virtual						~FileHandle();

			status_t			Open(Node* node, int openMode);
			status_t			Close();

			status_t			Read(off_t pos, void* buffer, size_t size,
									size_t* bytesRead);
			status_t			Write(off_t pos, const void* buffer,
									size_t size, size_t* bytesWritten);

 			status_t			ReadAttr(const char* name, uint32 type,
 									off_t pos, void* buffer, size_t size,
 									size_t* bytesRead);
 			status_t			WriteAttr(const char* name, uint32 type,
 									off_t pos, const void* buffer, size_t size,
 									size_t* bytesWritten);
 			status_t			RemoveAttr(const char* name);
			status_t			StatAttr(const char* name, attr_info* info);

protected:
	virtual	int					GetFD() const;

private:
			int					fFD;
};

// DirIterator
class DirIterator : public NodeHandle,
	public DoublyLinkedListLinkImpl<DirIterator> {
public:
								DirIterator();
	virtual						~DirIterator();

			bool				IsValid() const;

	virtual	Entry*				NextEntry() = 0;
	virtual	void				Rewind() = 0;


	// for Directory internal use only
	virtual	status_t			SetDirectory(Directory* directory) = 0;

	virtual	Entry*				GetCurrentEntry() const = 0;

protected:
			Directory*			fDirectory;
			bool				fValid;
};

// AttrDirIterator
class AttrDirIterator : public NodeHandle {
public:
								AttrDirIterator();
	virtual						~AttrDirIterator();

			status_t			Open(Node* node);
			status_t			Close();

	virtual	status_t			ReadDir(dirent* entry, int32 bufferSize,
									int32 count, int32* countRead, bool *done);
	virtual	status_t			RewindDir();

protected:
	virtual	int					GetFD() const;

private:
			DIR*				fDir;
};

// Query
class Query : public BQuery, public DoublyLinkedListLinkImpl<Query> {
public:
};

class QueryHandle;

// QueryListener
class QueryListener {
public:
								QueryListener();
	virtual						~QueryListener();

	virtual	void				QueryHandleClosed(QueryHandle* handle) = 0;
};

// QueryHandle
class QueryHandle : public NodeHandle {
public:
								QueryHandle(port_id remotePort,
									int32 remoteToken);
	virtual						~QueryHandle();

			void				SetQueryListener(QueryListener* listener);
			QueryListener*		GetQueryListener() const;

			void				AddQuery(Query* query);
			void				RemoveQuery(Query* query);

			port_id				GetRemotePort() const;
			int32				GetRemoteToken() const;

			status_t			Close();

			status_t			ReadDir(dirent* entry, int32 count,
									int32* countRead);
private:
			port_id				fRemotePort;
			int32				fRemoteToken;
			DoublyLinkedList<Query> fQueries;
			Query*				fCurrentQuery;
			QueryListener*		fListener;
			bool				fClosed;
};

#endif	// NET_FS_NODE_HANDLE_H

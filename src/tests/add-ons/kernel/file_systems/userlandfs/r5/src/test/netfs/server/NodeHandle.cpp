// NodeHandle.cpp

#include "NodeHandle.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fs_attr.h>

#include "Compatibility.h"
#include "Directory.h"
#include "FDManager.h"
#include "Path.h"

// TODO: Do the FD creation via the FDManager.

// NodeHandle

// constructor
NodeHandle::NodeHandle()
	: Referencable(true),
	  Lockable(),
	  fCookie(-1),
	  fNodeRef()
{
}

// destructor
NodeHandle::~NodeHandle()
{
}

// GetStat
status_t
NodeHandle::GetStat(struct stat* st)
{
	int fd = GetFD();
	if (fd < 0)
		return B_ENTRY_NOT_FOUND;

	if (fstat(fd, st) < 0)
		return errno;

	return B_OK;
}

// SetCookie
void
NodeHandle::SetCookie(int32 cookie)
{
	fCookie = cookie;
}

// GetCookie
int32
NodeHandle::GetCookie() const
{
	return fCookie;
}

// GetNodeRef
const NodeRef&
NodeHandle::GetNodeRef() const
{
	return fNodeRef;
}

// GetFD
int
NodeHandle::GetFD() const
{
	return -1;
}


// #pragma mark -

// FileHandle

// constructor
FileHandle::FileHandle()
	: fFD(-1)
{
}

// destructor
FileHandle::~FileHandle()
{
	Close();
}

// Open
status_t
FileHandle::Open(Node* node, int openMode)
{
	if (!node)
		return B_BAD_VALUE;

	openMode &= O_RWMASK | O_TRUNC | O_APPEND;

	// get a path
	Path path;
	status_t error = node->GetPath(&path);
	if (error != B_OK)
		return error;

	// open the file
	error = FDManager::Open(path.GetPath(), openMode | O_NOTRAVERSE, 0, fFD);
	if (error != B_OK)
		return error;

	fNodeRef = node->GetNodeRef();

	return B_OK;
}

// Close
status_t
FileHandle::Close()
{
	if (fFD < 0)
		return B_BAD_VALUE;

	status_t error = B_OK;
	if (close(fFD) < 0)
		error = errno;
	fFD = -1;

	return error;
}

// Read
status_t
FileHandle::Read(off_t pos, void* buffer, size_t size, size_t* _bytesRead)
{
	if (fFD < 0)
		return B_BAD_VALUE;

	ssize_t bytesRead = read_pos(fFD, pos, buffer, size);
	if (bytesRead < 0)
		return errno;

	*_bytesRead = bytesRead;
	return B_OK;
}

// Write
status_t
FileHandle::Write(off_t pos, const void* buffer, size_t size,
	size_t* _bytesWritten)
{
	if (fFD < 0)
		return B_BAD_VALUE;

	ssize_t bytesWritten = write_pos(fFD, pos, buffer, size);
	if (bytesWritten < 0)
		return errno;

	*_bytesWritten = bytesWritten;
	return B_OK;
}

// ReadAttr
status_t
FileHandle::ReadAttr(const char* name, uint32 type, off_t pos, void* buffer,
	size_t size, size_t* _bytesRead)
{
	if (fFD < 0)
		return B_BAD_VALUE;

	ssize_t bytesRead = fs_read_attr(fFD, name, type, pos, buffer, size);
	if (bytesRead < 0)
		return errno;

	*_bytesRead = bytesRead;
	return B_OK;
}

// WriteAttr
status_t
FileHandle::WriteAttr(const char* name, uint32 type, off_t pos,
	const void* buffer, size_t size, size_t* _bytesWritten)
{
	if (fFD < 0)
		return B_BAD_VALUE;

	ssize_t bytesWritten = fs_write_attr(fFD, name, type, pos, buffer, size);
	if (bytesWritten < 0)
		return errno;

	*_bytesWritten = bytesWritten;
	return B_OK;
}

// RemoveAttr
status_t
FileHandle::RemoveAttr(const char* name)
{
	if (fFD < 0)
		return B_BAD_VALUE;

	return (fs_remove_attr(fFD, name) == 0 ? B_OK : errno);
}

// StatAttr
status_t
FileHandle::StatAttr(const char* name, attr_info* info)
{
	if (fFD < 0)
		return B_BAD_VALUE;

	return (fs_stat_attr(fFD, name, info) == 0 ? B_OK : errno);
}

// GetFD
int
FileHandle::GetFD() const
{
	return fFD;
}


// #pragma mark -

// DirIterator

// constructor
DirIterator::DirIterator()
	: fDirectory(NULL)
{
}

// destructor
DirIterator::~DirIterator()
{
	if (fDirectory)
		fDirectory->RemoveDirIterator(this);
}

// IsValid
bool
DirIterator::IsValid() const
{
	return fDirectory;
}


// #pragma mark -

// AttrDirIterator

// constructor
AttrDirIterator::AttrDirIterator()
	: NodeHandle(),
	  fDir(NULL)
{
}

// destructor
AttrDirIterator::~AttrDirIterator()
{
	Close();
}

// Open
status_t
AttrDirIterator::Open(Node* node)
{
	if (!node)
		return B_BAD_VALUE;

	// get a path
	Path path;
	status_t error = node->GetPath(&path);
	if (error != B_OK)
		return error;

	// open the attribute directory
	error = FDManager::OpenAttrDir(path.GetPath(), fDir);
	if (error != B_OK)
		return errno;

	fNodeRef = node->GetNodeRef();

	return B_OK;
}

// Close
status_t
AttrDirIterator::Close()
{
	if (!fDir)
		return B_BAD_VALUE;

	status_t error = B_OK;
	if (fs_close_attr_dir(fDir) < 0)
		error = errno;

	fDir = NULL;
	return error;
}

// ReadDir
status_t
AttrDirIterator::ReadDir(dirent* _entry, int32 bufferSize,
	int32 count, int32* countRead, bool* done)
{
// TODO: Burst read.
	if (!fDir)
		return B_ENTRY_NOT_FOUND;

	*countRead = 0;
	*done = false;

	if (count == 0)
		return B_OK;

	if (struct dirent* entry = fs_read_attr_dir(fDir)) {
		size_t entryLen = entry->d_name + strlen(entry->d_name) + 1
			- (char*)entry;
		memcpy(_entry, entry, entryLen);
		*countRead = 1;
	} else {
		*countRead = 0;
		*done = true;
	}
	return B_OK;
}

// RewindDir
status_t
AttrDirIterator::RewindDir()
{
	if (!fDir)
		return B_ENTRY_NOT_FOUND;

	fs_rewind_attr_dir(fDir);

	return B_OK;
}

// GetFD
int
AttrDirIterator::GetFD() const
{
	return (fDir ? fDir->fd : -1);
}


// #pragma mark -

// QueryListener

// constructor
QueryListener::QueryListener()
{
}

// destructor
QueryListener::~QueryListener()
{
}


// #pragma mark -

// QueryHandle

// constructor
QueryHandle::QueryHandle(port_id remotePort, int32 remoteToken)
	: NodeHandle(),
	  fRemotePort(remotePort),
	  fRemoteToken(remoteToken),
	  fQueries(),
	  fCurrentQuery(NULL),
	  fListener(NULL),
	  fClosed(false)
{
}

// destructor
QueryHandle::~QueryHandle()
{
	Close();
}

// SetQueryListener
void
QueryHandle::SetQueryListener(QueryListener* listener)
{
	fListener = listener;
}

// GetQueryListener
QueryListener*
QueryHandle::GetQueryListener() const
{
	return fListener;
}

// AddQuery
void
QueryHandle::AddQuery(Query* query)
{
	if (query) {
		fQueries.Insert(query);
		if (!fCurrentQuery)
			fCurrentQuery = query;
	}
}

// RemoveQuery
void
QueryHandle::RemoveQuery(Query* query)
{
	if (query) {
		if (query == fCurrentQuery)
			fCurrentQuery = fQueries.GetNext(query);
		fQueries.Remove(query);
	}
}

// GetRemotePort
port_id
QueryHandle::GetRemotePort() const
{
	return fRemotePort;
}

// GetRemoteToken
int32
QueryHandle::GetRemoteToken() const
{
	return fRemoteToken;
}

// Close
status_t
QueryHandle::Close()
{
	if (fClosed)
		return B_OK;

	fCurrentQuery = NULL;

	// delete all queries
	for (DLList<Query>::Iterator it = fQueries.GetIterator(); it.HasNext();) {
		Query* query = it.Next();
		it.Remove();
		delete query;
	}

	fClosed = true;

	if (fListener)
		fListener->QueryHandleClosed(this);

	return B_OK;
}

// ReadDir
status_t
QueryHandle::ReadDir(dirent* entry, int32 count, int32* countRead)
{
	if (count == 0) {
		*countRead = 0;
		return B_OK;
	}
	while (fCurrentQuery) {
		int32 readEntries = fCurrentQuery->GetNextDirents(entry,
			sizeof(struct dirent) + B_FILE_NAME_LENGTH, 1);
		if (readEntries < 0)
			return readEntries;
		if (readEntries > 0) {
			*countRead = 1;
			return B_OK;
		}
		fCurrentQuery = fQueries.GetNext(fCurrentQuery);
	}
	*countRead = 0;
	return B_OK;
}


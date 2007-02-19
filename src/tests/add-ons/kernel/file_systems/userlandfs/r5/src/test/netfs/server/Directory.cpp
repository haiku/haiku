// Directory.cpp

#include <new>

#include <dirent.h>
#include <errno.h>

#include "AutoDeleter.h"
#include "Directory.h"
#include "FDManager.h"
#include "Path.h"
#include "VolumeManager.h"

// CachedDirIterator
class CachedDirIterator : public DirIterator {
public:
								CachedDirIterator();
								~CachedDirIterator();

	virtual	status_t			SetDirectory(Directory* directory);

	virtual	Entry*				NextEntry();
	virtual	void				Rewind();

	virtual	Entry*				GetCurrentEntry() const;

	virtual	status_t			GetStat(struct stat* st);

private:
			Entry*				fCurrentEntry;
};

// UncachedDirIterator
class UncachedDirIterator : public DirIterator {
public:
								UncachedDirIterator();
								~UncachedDirIterator();

	virtual	status_t			SetDirectory(Directory* directory);

	virtual	Entry*				NextEntry();
	virtual	void				Rewind();

	virtual	Entry*				GetCurrentEntry() const;

protected:
	virtual	int					GetFD() const;

private:
			DIR*				fDirHandle;
};


// #pragma mark -

// constructor
CachedDirIterator::CachedDirIterator()
	: DirIterator(),
	  fCurrentEntry(NULL)
{
}

// destructor
CachedDirIterator::~CachedDirIterator()
{
}

// SetDirectory
status_t
CachedDirIterator::SetDirectory(Directory* directory)
{
	// set the new directory
	fDirectory = directory;
	fCurrentEntry = (fDirectory ? fDirectory->GetFirstEntry() : NULL);

	if (directory)
		fNodeRef = directory->GetNodeRef();

	return B_OK;
}

// NextEntry
Entry*
CachedDirIterator::NextEntry()
{
	if (!IsValid() || !fCurrentEntry)
		return NULL;

	Entry* entry = fCurrentEntry;
	fCurrentEntry = fDirectory->GetNextEntry(fCurrentEntry);
	return entry;
}

// Rewind
void
CachedDirIterator::Rewind()
{
	fCurrentEntry = (IsValid() ? fDirectory->GetFirstEntry() : NULL);
}

// GetCurrentEntry
Entry*
CachedDirIterator::GetCurrentEntry() const
{
	return (IsValid() ? fCurrentEntry : NULL);
}

// GetStat
status_t
CachedDirIterator::GetStat(struct stat* st)
{
	if (!fDirectory || !st)
		return B_BAD_VALUE;

	*st = fDirectory->GetStat();
	return B_OK;
}


// #pragma mark -

// constructor
UncachedDirIterator::UncachedDirIterator()
	: DirIterator(),
	  fDirHandle(NULL)
{
}

// destructor
UncachedDirIterator::~UncachedDirIterator()
{
	// close
	if (fDirHandle) {
		closedir(fDirHandle);
		fDirHandle = NULL;
	}
}

// SetDirectory
status_t
UncachedDirIterator::SetDirectory(Directory* directory)
{
	// unset old
	if (fDirHandle) {
		closedir(fDirHandle);
		fDirHandle = NULL;
	}
	fDirectory = NULL;

	// set new
	if (directory) {
		// get the directory path
		Path path;
		status_t error = directory->GetPath(&path);
		if (error != B_OK)
			return error;

		// open the directory
		error = FDManager::OpenDir(path.GetPath(), fDirHandle);
		if (error != B_OK)
			return error;

		fDirectory = directory;
		fNodeRef = directory->GetNodeRef();
	}

	return B_OK;
}

// NextEntry
Entry*
UncachedDirIterator::NextEntry()
{
	if (!IsValid() && fDirHandle)
		return NULL;

	while (struct dirent* dirEntry = readdir(fDirHandle)) {
		Entry* entry;
		if (VolumeManager::GetDefault()->LoadEntry(dirEntry->d_pdev,
				dirEntry->d_pino, dirEntry->d_name, false, &entry) == B_OK) {
			return entry;
		}
	}

	// we're through: set the directory to "complete"
	fDirectory->SetComplete(true);

	return NULL;
}

// Rewind
void
UncachedDirIterator::Rewind()
{
	if (IsValid() && fDirHandle)
		rewinddir(fDirHandle);
}

// GetCurrentEntry
Entry*
UncachedDirIterator::GetCurrentEntry() const
{
	return NULL;
}

// GetFD
int
UncachedDirIterator::GetFD() const
{
	return (fDirHandle ? fDirHandle->fd : -1);
}


// #pragma mark -

// constructor
Directory::Directory(Volume* volume, const struct stat& st)
	: Node(volume, st),
	  fEntries(),
	  fIterators(),
	  fIsComplete(false)
{
}

// destructor
Directory::~Directory()
{
	// remove all directory iterators
	while (DirIterator* iterator = fIterators.GetFirst())
		iterator->SetDirectory(NULL);
}

// GetActualReferringEntry
Entry*
Directory::GetActualReferringEntry() const
{
	// any entry other than "." and ".." is fine
	for (Entry* entry = GetFirstReferringEntry();
		 entry;
		 entry = GetNextReferringEntry(entry)) {
		if (entry->IsActualEntry())
			return entry;
	}
	return NULL;
}

// AddEntry
void
Directory::AddEntry(Entry* entry)
{
	if (entry)
		fEntries.Insert(entry);
}

// RemoveEntry
void
Directory::RemoveEntry(Entry* entry)
{
	if (entry) {
		// update the directory iterators pointing to the removed entry
		for (DirIterator* iterator = fIterators.GetFirst();
			 iterator;
			 iterator = fIterators.GetNext(iterator)) {
			if (iterator->GetCurrentEntry() == entry)
				iterator->NextEntry();
		}

		fEntries.Remove(entry);
	}
}

// GetFirstEntry
Entry*
Directory::GetFirstEntry() const
{
	return fEntries.GetFirst();
}

// GetNextEntry
Entry*
Directory::GetNextEntry(Entry* entry) const
{
	return (entry ? fEntries.GetNext(entry) : NULL);
}

// CountEntries
int32
Directory::CountEntries() const
{
	int32 count = 0;
	Entry* entry = GetFirstEntry();
	while (entry) {
		count++;
		entry = GetNextEntry(entry);
	}
	return count;
}

// OpenDir
status_t
Directory::OpenDir(DirIterator** _iterator)
{
	if (!_iterator)
		return B_BAD_VALUE;

	// create the iterator
	DirIterator* iterator;
	if (fIsComplete)
		iterator = new(nothrow) CachedDirIterator;
	else
		iterator = new(nothrow) UncachedDirIterator;
	if (!iterator)
		return B_NO_MEMORY;
	ObjectDeleter<DirIterator> iteratorDeleter(iterator);

	// initialize it
	status_t error = iterator->SetDirectory(this);
	if (error != B_OK)
		return error;

	// check, if it really belongs to this node
	error = _CheckNodeHandle(iterator);
	if (error != B_OK)
		return error;

	// add it
	fIterators.Insert(iterator);

	iteratorDeleter.Detach();
	*_iterator = iterator;
	return B_OK;
}

// HasDirIterators
bool
Directory::HasDirIterators() const
{
	return fIterators.GetFirst();
}

// RemoveDirIterator
void
Directory::RemoveDirIterator(DirIterator* iterator)
{
	if (iterator)
		fIterators.Remove(iterator);
}

// SetComplete
void
Directory::SetComplete(bool complete)
{
	fIsComplete = complete;
}

// IsComplete
bool
Directory::IsComplete() const
{
	return fIsComplete;
}

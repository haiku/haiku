// FDManager.cpp

#include <new>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>

#include "AutoLocker.h"
#include "FDManager.h"

// FD constants
static const int32 kDefaultFDLimit = 512;
static const int32 kFDLimitIncrement = 128;


// constructor
FDManager::FDManager()
	: fLock("FD manager"),
	  fFDLimit(kDefaultFDLimit)
{
}

// destructor
FDManager::~FDManager()
{
}

// Init
status_t
FDManager::Init()
{
	// ask for a bunch more file descriptors
	struct rlimit rl;
	rl.rlim_cur = fFDLimit;
	rl.rlim_max = RLIM_SAVED_MAX;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
		return errno;

	return B_OK;
}

// CreateDefault
status_t
FDManager::CreateDefault()
{
	if (sManager)
		return B_OK;

	FDManager* manager = new(std::nothrow) FDManager;
	if (!manager)
		return B_NO_MEMORY;

	status_t error = manager->Init();
	if (error != B_OK) {
		delete manager;
		return error;
	}

	sManager = manager;
	return B_OK;
}

// DeleteDefault
void
FDManager::DeleteDefault()
{
	if (sManager) {
		delete sManager;
		sManager = NULL;
	}
}

// GetDefault
FDManager*
FDManager::GetDefault()
{
	return sManager;
}

// SetDirectory
status_t
FDManager::SetDirectory(BDirectory* directory, const node_ref* ref)
{
	status_t error = directory->SetTo(ref);

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();
		error = directory->SetTo(ref);
	}

	return error;
}

// SetEntry
status_t
FDManager::SetEntry(BEntry* entry, const entry_ref* ref)
{
	status_t error = entry->SetTo(ref);

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();
		error = entry->SetTo(ref);
	}

	return error;
}

// SetEntry
status_t
FDManager::SetEntry(BEntry* entry, const char* path)
{
	status_t error = entry->SetTo(path);

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();
		error = entry->SetTo(path);
	}

	return error;
}

// SetFile
status_t
FDManager::SetFile(BFile* file, const char* path, uint32 openMode)
{
	status_t error = file->SetTo(path, openMode);

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();
		error = file->SetTo(path, openMode);
	}

	return error;
}

// SetNode
status_t
FDManager::SetNode(BNode* node, const entry_ref* ref)
{
	status_t error = node->SetTo(ref);

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();
		error = node->SetTo(ref);
	}

	return error;
}

// Open
status_t
FDManager::Open(const char* path, uint32 openMode, mode_t mode, int& fd)
{
	status_t error = B_OK;
	fd = open(path, openMode, mode);
	if (fd < 0)
		error = errno;

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();

		error = B_OK;
		fd = open(path, openMode, mode);
		if (fd < 0)
			error = errno;
	}

	return error;
}

// OpenDir
status_t
FDManager::OpenDir(const char* path, DIR*& dir)
{
	status_t error = B_OK;
	dir = opendir(path);
	if (!dir)
		error = errno;

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();

		error = B_OK;
		dir = opendir(path);
		if (!dir)
			error = errno;
	}

	return error;
}

// OpenAttrDir
status_t
FDManager::OpenAttrDir(const char* path, DIR*& dir)
{
	status_t error = B_OK;
	dir = fs_open_attr_dir(path);
	if (!dir)
		error = errno;

	if (error == B_NO_MORE_FDS) {
		GetDefault()->_IncreaseLimit();

		error = B_OK;
		dir = fs_open_attr_dir(path);
		if (!dir)
			error = errno;
	}

	return error;
}

// _IncreaseLimit
status_t
FDManager::_IncreaseLimit()
{
	AutoLocker<Locker> _(fLock);

	int32 newLimit = fFDLimit + kFDLimitIncrement;

	struct rlimit rl;
	rl.rlim_cur = newLimit;
	rl.rlim_max = RLIM_SAVED_MAX;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
		return errno;

	fFDLimit = newLimit;

	return B_OK;
}


// sManager
FDManager* FDManager::sManager = NULL;

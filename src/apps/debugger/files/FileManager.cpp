/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FileManager.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "LocatableDirectory.h"
#include "LocatableFile.h"
#include "SourceFile.h"
#include "StringUtils.h"


// #pragma mark - EntryPath


struct FileManager::EntryPath {
	const char*	directory;
	const char*	name;

	EntryPath(const char* directory, const char* name)
		:
		directory(directory),
		name(name)
	{
	}

	EntryPath(const BString& directory, const BString& name)
		:
		directory(directory.Length() > 0 ? directory.String() : NULL),
		name(name.String())
	{
	}

	EntryPath(const LocatableEntry* entry)
		:
		directory(entry->Parent() != NULL ? entry->Parent()->Path() : NULL),
		name(entry->Name())
	{
	}

	EntryPath(const EntryPath& other)
		:
		directory(other.directory),
		name(other.name)
	{
	}

	size_t HashValue() const
	{
		return StringUtils::HashValue(directory)
			^ StringUtils::HashValue(name);
	}

	bool operator==(const EntryPath& other) const
	{
		if (directory != other.directory
			&& (directory == NULL || other.directory == NULL
				|| strcmp(directory, other.directory) != 0)) {
			return false;
		}

		return strcmp(name, other.name) == 0;
	}
};


// #pragma mark - EntryHashDefinition


struct FileManager::EntryHashDefinition {
	typedef EntryPath		KeyType;
	typedef	LocatableEntry	ValueType;

	size_t HashKey(const EntryPath& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const LocatableEntry* value) const
	{
		return HashKey(EntryPath(value));
	}

	bool Compare(const EntryPath& key, const LocatableEntry* value) const
	{
		return EntryPath(value) == key;
	}

	LocatableEntry*& GetLink(LocatableEntry* value) const
	{
		return value->fNext;
	}
};


// #pragma mark - Domain


class FileManager::Domain : private LocatableEntryOwner {
public:
	Domain(FileManager* manager, bool isLocal)
		:
		fManager(manager),
		fIsLocal(isLocal)
	{
	}

	~Domain()
	{
	}

	status_t Init()
	{
		status_t error = fEntries.Init();
		if (error != B_OK)
			return error;

		return B_OK;
	}

	LocatableFile* GetFile(const BString& directoryPath,
		const BString& relativePath)
	{
		if (directoryPath.Length() == 0)
			return GetFile(relativePath);
		return GetFile(BString(directoryPath) << '/' << relativePath);
	}

	LocatableFile* GetFile(const BString& path)
	{
		BString directoryPath;
		BString name;
		_SplitPath(path, directoryPath, name);
		LocatableFile* file = _GetFile(directoryPath, name);
		if (file == NULL)
			return NULL;

		// try to auto-locate the file
		if (LocatableDirectory* directory = file->Parent()) {
			if (directory->State() == LOCATABLE_ENTRY_UNLOCATED) {
				// parent not yet located -- try locate with the entry's path
				BString path;
				file->GetPath(path);
				_LocateEntry(file, path, true, true);
			} else {
				// parent already located -- locate the entry in the parent
				BString locatedDirectoryPath;
				if (directory->GetLocatedPath(locatedDirectoryPath))
					_LocateEntryInParentDir(file, locatedDirectoryPath);
			}
		}

		return file;
	}

	void EntryLocated(const BString& path, const BString& locatedPath)
	{
		BString directory;
		BString name;
		_SplitPath(path, directory, name);

		LocatableEntry* entry = _LookupEntry(EntryPath(directory, name));
		if (entry == NULL)
			return;

		_LocateEntry(entry, locatedPath, false, true);
	}

private:
	virtual bool Lock()
	{
		return fManager->Lock();
	}

	virtual void Unlock()
	{
		fManager->Unlock();
	}

	virtual void LocatableEntryUnused(LocatableEntry* entry)
	{
		AutoLocker<FileManager> lock(fManager);
		if (fEntries.Lookup(EntryPath(entry)) == entry)
			fEntries.Remove(entry);
		else {
			DeadEntryList::Iterator iterator = fDeadEntries.GetIterator();
			while (iterator.HasNext()) {
				if (iterator.Next() == entry) {
					fDeadEntries.Remove(entry);
					break;
				}
			}
		}
	}

	bool _LocateDirectory(LocatableDirectory* directory,
		const BString& locatedPath)
	{
		if (directory == NULL
			|| directory->State() != LOCATABLE_ENTRY_UNLOCATED) {
			return false;
		}

		if (!_LocateEntry(directory, locatedPath, true, true))
			return false;

		_LocateEntries(directory, locatedPath);

		return true;
	}

	bool _LocateEntry(LocatableEntry* entry, const BString& locatedPath,
		bool implicit, bool locateAncestors)
	{
		if (implicit && entry->State() == LOCATABLE_ENTRY_LOCATED_EXPLICITLY)
			return false;

		struct stat st;
		if (stat(locatedPath, &st) != 0)
			return false;

		if (S_ISDIR(st.st_mode)) {
			LocatableDirectory* directory
				= dynamic_cast<LocatableDirectory*>(entry);
			if (directory == NULL)
				return false;
			entry->SetLocatedPath(locatedPath, implicit);
		} else if (S_ISREG(st.st_mode)) {
			LocatableFile* file = dynamic_cast<LocatableFile*>(entry);
			if (file == NULL)
				return false;
			entry->SetLocatedPath(locatedPath, implicit);
		}

		// locate the ancestor directories, if requested
		if (locateAncestors) {
			BString locatedDirectory;
			BString locatedName;
			_SplitPath(locatedPath, locatedDirectory, locatedName);
			if (locatedName == entry->Name())
				_LocateDirectory(entry->Parent(), locatedDirectory);
		}

		return true;
	}

	bool _LocateEntryInParentDir(LocatableEntry* entry,
		const BString& locatedDirectoryPath)
	{
		// construct the located entry path
		BString locatedEntryPath(locatedDirectoryPath);
		int32 pathLength = locatedEntryPath.Length();
		if (pathLength >= 1 && locatedEntryPath[pathLength - 1] != '/')
			locatedEntryPath << '/';
		locatedEntryPath << entry->Name();

		return _LocateEntry(entry, locatedEntryPath, true, false);
	}

	void _LocateEntries(LocatableDirectory* directory,
		const BString& locatedPath)
	{
		for (LocatableEntryList::ConstIterator it
				= directory->Entries().GetIterator();
			LocatableEntry* entry = it.Next();) {
			if (entry->State() == LOCATABLE_ENTRY_LOCATED_EXPLICITLY)
				continue;

			 if (_LocateEntryInParentDir(entry, locatedPath)) {
				// recurse for directories
				if (LocatableDirectory* subDir
						= dynamic_cast<LocatableDirectory*>(entry)) {
					BString locatedEntryPath;
					if (subDir->GetLocatedPath(locatedEntryPath))
						_LocateEntries(subDir, locatedEntryPath);
				}
			}
		}
	}

	LocatableFile* _GetFile(const BString& directoryPath, const BString& name)
	{
		// if already known return the file
		LocatableEntry* entry = _LookupEntry(EntryPath(directoryPath, name));
		if (entry != NULL) {
			LocatableFile* file = dynamic_cast<LocatableFile*>(entry);
			if (file == NULL)
				return NULL;

			if (file->AcquireReference() == 0) {
				fEntries.Remove(file);
				fDeadEntries.Insert(file);
			} else
				return file;
		}

		// no such file yet -- create it
		BString normalizedDirPath;
		_NormalizePath(directoryPath, normalizedDirPath);
		LocatableDirectory* directory = _GetDirectory(normalizedDirPath);
		if (directory == NULL)
			return NULL;

		LocatableFile* file = new(std::nothrow) LocatableFile(this, directory,
			name);
		if (file == NULL) {
			directory->ReleaseReference();
			return NULL;
		}

		fEntries.Insert(file);
		return file;
	}

	LocatableDirectory* _GetDirectory(const BString& path)
	{
		BString directoryPath;
		BString fileName;
		_SplitNormalizedPath(path, directoryPath, fileName);

		// if already know return the directory
		LocatableEntry* entry
			= _LookupEntry(EntryPath(directoryPath, fileName));
		if (entry != NULL) {
			LocatableDirectory* directory
				= dynamic_cast<LocatableDirectory*>(entry);
			if (directory == NULL)
				return NULL;
			directory->AcquireReference();
			return directory;
		}

		// get the parent directory
		LocatableDirectory* parentDirectory = NULL;
		if (directoryPath.Length() > 0) {
			parentDirectory = _GetDirectory(directoryPath);
			if (parentDirectory == NULL)
				return NULL;
		}

		// create a new directory
		LocatableDirectory* directory = new(std::nothrow) LocatableDirectory(
			this, parentDirectory, path);
		if (directory == NULL) {
			parentDirectory->ReleaseReference();
			return NULL;
		}

		// auto-locate, if possible
		if (fIsLocal) {
			BString dirPath;
			directory->GetPath(dirPath);
			directory->SetLocatedPath(dirPath, false);
		} else if (parentDirectory != NULL
			&& parentDirectory->State() != LOCATABLE_ENTRY_UNLOCATED) {
			BString locatedDirectoryPath;
			if (parentDirectory->GetLocatedPath(locatedDirectoryPath))
				_LocateEntryInParentDir(directory, locatedDirectoryPath);
		}

		fEntries.Insert(directory);
		return directory;
	}

	LocatableEntry* _LookupEntry(const EntryPath& entryPath)
	{
		LocatableEntry* entry = fEntries.Lookup(entryPath);
		if (entry == NULL)
			return NULL;

		// if already unreferenced, remove it
		if (entry->CountReferences() == 0) {
			fEntries.Remove(entry);
			return NULL;
		}

		return entry;
	}

	void _NormalizePath(const BString& path, BString& _normalizedPath)
	{
		BString normalizedPath;
		char* buffer = normalizedPath.LockBuffer(path.Length());
		int32 outIndex = 0;
		const char* remaining = path.String();

		while (*remaining != '\0') {
			// collapse repeated slashes
			if (*remaining == '/') {
				buffer[outIndex++] = '/';
				remaining++;
				while (*remaining == '/')
					remaining++;
			}

			if (*remaining == '\0') {
				// remove trailing slash (unless it's "/" only)
				if (outIndex > 1)
					outIndex--;
				break;
			}

			// skip "." components
			if (*remaining == '.') {
				if (remaining[1] == '\0')
					break;

				if (remaining[1] == '/') {
					remaining += 2;
					while (*remaining == '/')
						remaining++;
					continue;
				}
			}

			// copy path component
			while (*remaining != '\0' && *remaining != '/')
				buffer[outIndex++] = *(remaining++);
		}

		// If the path didn't change, use the original path (BString's copy on
		// write mechanism) rather than the new string.
		if (outIndex == path.Length()) {
			_normalizedPath = path;
		} else {
			normalizedPath.UnlockBuffer(outIndex);
			_normalizedPath = normalizedPath;
		}
	}

	void _SplitPath(const BString& path, BString& _directory, BString& _name)
	{
		BString normalized;
		_NormalizePath(path, normalized);
		_SplitNormalizedPath(normalized, _directory, _name);
	}

	void _SplitNormalizedPath(const BString& path, BString& _directory,
		BString& _name)
	{
		// handle single component (including root dir) cases
		int32 lastSlash = path.FindLast('/');
		if (lastSlash < 0 || path.Length() == 1) {
			_directory = (const char*)NULL;
			_name = path;
			return;
		}

		// handle root dir + one component and multi component cases
		if (lastSlash == 0)
			_directory = "/";
		else
			_directory.SetTo(path, lastSlash);
		_name = path.String() + (lastSlash + 1);
	}

private:
	FileManager*		fManager;
	LocatableEntryTable	fEntries;
	DeadEntryList		fDeadEntries;
	bool				fIsLocal;
};


// #pragma mark - SourceFileEntry


struct FileManager::SourceFileEntry : public SourceFileOwner {

	FileManager*		manager;
	BString				path;
	SourceFile*			file;
	SourceFileEntry*	next;

	SourceFileEntry(FileManager* manager, const BString& path)
		:
		manager(manager),
		path(path),
		file(NULL)
	{
	}

	virtual void SourceFileUnused(SourceFile* sourceFile)
	{
		manager->_SourceFileUnused(this);
	}

	virtual void SourceFileDeleted(SourceFile* sourceFile)
	{
		// We have already been removed from the table, so commit suicide.
		delete this;
	}
};


// #pragma mark - SourceFileHashDefinition


struct FileManager::SourceFileHashDefinition {
	typedef BString			KeyType;
	typedef	SourceFileEntry	ValueType;

	size_t HashKey(const BString& key) const
	{
		return StringUtils::HashValue(key);
	}

	size_t Hash(const SourceFileEntry* value) const
	{
		return HashKey(value->path);
	}

	bool Compare(const BString& key, const SourceFileEntry* value) const
	{
		return value->path == key;
	}

	SourceFileEntry*& GetLink(SourceFileEntry* value) const
	{
		return value->next;
	}
};


// #pragma mark - FileManager


FileManager::FileManager()
	:
	fLock("file manager"),
	fTargetDomain(NULL),
	fSourceDomain(NULL),
	fSourceFiles(NULL)
{
}


FileManager::~FileManager()
{
	delete fTargetDomain;
	delete fSourceDomain;
	delete fSourceFiles;
}


status_t
FileManager::Init(bool targetIsLocal)
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	// create target domain
	fTargetDomain = new(std::nothrow) Domain(this, targetIsLocal);
	if (fTargetDomain == NULL)
		return B_NO_MEMORY;

	error = fTargetDomain->Init();
	if (error != B_OK)
		return error;

	// create source domain
	fSourceDomain = new(std::nothrow) Domain(this, false);
	if (fSourceDomain == NULL)
		return B_NO_MEMORY;

	error = fSourceDomain->Init();
	if (error != B_OK)
		return error;

	// create source file table
	fSourceFiles = new(std::nothrow) SourceFileTable;
	if (fSourceFiles == NULL)
		return B_NO_MEMORY;

	error = fSourceFiles->Init();
	if (error != B_OK)
		return error;

	return B_OK;
}


LocatableFile*
FileManager::GetTargetFile(const BString& directory,
	const BString& relativePath)
{
	AutoLocker<FileManager> locker(this);
	return fTargetDomain->GetFile(directory, relativePath);
}


LocatableFile*
FileManager::GetTargetFile(const BString& path)
{
	AutoLocker<FileManager> locker(this);
	return fTargetDomain->GetFile(path);
}


void
FileManager::TargetEntryLocated(const BString& path, const BString& locatedPath)
{
	AutoLocker<FileManager> locker(this);
	fTargetDomain->EntryLocated(path, locatedPath);
}


LocatableFile*
FileManager::GetSourceFile(const BString& directory,
	const BString& relativePath)
{
	AutoLocker<FileManager> locker(this);
	return fSourceDomain->GetFile(directory, relativePath);
}


LocatableFile*
FileManager::GetSourceFile(const BString& path)
{
	AutoLocker<FileManager> locker(this);
	return fSourceDomain->GetFile(path);
}


void
FileManager::SourceEntryLocated(const BString& path, const BString& locatedPath)
{
	AutoLocker<FileManager> locker(this);
	fSourceDomain->EntryLocated(path, locatedPath);
}


status_t
FileManager::LoadSourceFile(LocatableFile* file, SourceFile*& _sourceFile)
{
	AutoLocker<FileManager> locker(this);

	// get the path
	BString path;
	if (!file->GetLocatedPath(path))
		return B_ENTRY_NOT_FOUND;

	// we might already know the source file
	SourceFileEntry* entry = _LookupSourceFile(path);
	if (entry != NULL) {
		entry->file->AcquireReference();
		_sourceFile = entry->file;
		return B_OK;
	}

	// create the hash table entry
	entry = new(std::nothrow) SourceFileEntry(this, path);
	if (entry == NULL)
		return B_NO_MEMORY;

	// load the file
	SourceFile* sourceFile = new(std::nothrow) SourceFile(entry);
	if (sourceFile == NULL) {
		delete entry;
		return B_NO_MEMORY;
	}
	ObjectDeleter<SourceFile> sourceFileDeleter(sourceFile);

	entry->file = sourceFile;

	status_t error = sourceFile->Init(path);
	if (error != B_OK)
		return error;

	fSourceFiles->Insert(entry);

	_sourceFile = sourceFileDeleter.Detach();
	return B_OK;
}


FileManager::SourceFileEntry*
FileManager::_LookupSourceFile(const BString& path)
{
	SourceFileEntry* entry = fSourceFiles->Lookup(path);
	if (entry == NULL)
		return NULL;

	// the entry might be unused already -- in that case remove it
	if (entry->file->CountReferences() == 0) {
		fSourceFiles->Remove(entry);
		return NULL;
	}

	return entry;
}


void
FileManager::_SourceFileUnused(SourceFileEntry* entry)
{
	AutoLocker<FileManager> locker(this);

	SourceFileEntry* otherEntry = fSourceFiles->Lookup(entry->path);
	if (otherEntry == entry)
		fSourceFiles->Remove(entry);
}

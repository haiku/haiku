/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <fs_attr.h>
#include <String.h>

#include <AutoDeleter.h>
#include <HashString.h>

#include <util/OpenHashTable.h>

#include <package/hpkg/BlockBufferPoolNoLock.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReader.h>
#include <package/hpkg/StandardErrorOutput.h>
#include <package/hpkg/v1/PackageContentHandler.h>
#include <package/hpkg/v1/PackageDataReader.h>
#include <package/hpkg/v1/PackageEntry.h>
#include <package/hpkg/v1/PackageEntryAttribute.h>
#include <package/hpkg/v1//PackageReader.h>

#include "package.h"


using BPackageKit::BHPKG::BAbstractBufferedDataReader;
using BPackageKit::BHPKG::BBlockBufferPoolNoLock;
using BPackageKit::BHPKG::BBufferDataReader;
using BPackageKit::BHPKG::BBufferPool;
using BPackageKit::BHPKG::BDataReader;
using BPackageKit::BHPKG::BErrorOutput;
using BPackageKit::BHPKG::BFDDataReader;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BStandardErrorOutput;


struct VersionPolicyV1 {
	typedef BPackageKit::BHPKG::V1::BPackageContentHandler
		PackageContentHandler;
	typedef BPackageKit::BHPKG::V1::BPackageData PackageData;
	typedef BPackageKit::BHPKG::V1::BPackageEntry PackageEntry;
	typedef BPackageKit::BHPKG::V1::BPackageEntryAttribute
		PackageEntryAttribute;
	typedef BPackageKit::BHPKG::V1::BPackageReader PackageReader;
	typedef BDataReader HeapReaderBase;

	static inline size_t BufferSize()
	{
		return BPackageKit::BHPKG::V1::B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;
	}

	static inline const char* PackageInfoFileName()
	{
		return BPackageKit::BHPKG::V1::B_HPKG_PACKAGE_INFO_FILE_NAME;
	}

	static inline uint64 PackageDataCompressedSize(const PackageData& data)
	{
		return data.CompressedSize();
	}

	static inline uint64 PackageDataUncompressedSize(const PackageData& data)
	{
		return data.UncompressedSize();
	}

	static inline status_t InitReader(PackageReader& packageReader,
		const char* fileName)
	{
		return packageReader.Init(fileName);
	}

	static status_t GetHeapReader(PackageReader& packageReader,
		HeapReaderBase*& _heapReader, bool& _mustDelete)
	{
		_heapReader = new(std::nothrow) BFDDataReader(
			packageReader.PackageFileFD());
		_mustDelete = false;
		return _heapReader != NULL ? B_OK : B_NO_MEMORY;
	}

	static status_t CreatePackageDataReader(BBufferPool* bufferPool,
		HeapReaderBase* heapReader, const PackageData& data,
		BAbstractBufferedDataReader*& _reader)
	{
		return BPackageKit::BHPKG::V1::BPackageDataReaderFactory(bufferPool)
			.CreatePackageDataReader(heapReader, data, _reader);
	}
};

struct VersionPolicyV2 {
	typedef BPackageKit::BHPKG::BPackageContentHandler PackageContentHandler;
	typedef BPackageKit::BHPKG::BPackageData PackageData;
	typedef BPackageKit::BHPKG::BPackageEntry PackageEntry;
	typedef BPackageKit::BHPKG::BPackageEntryAttribute PackageEntryAttribute;
	typedef BPackageKit::BHPKG::BPackageReader PackageReader;
	typedef BAbstractBufferedDataReader HeapReaderBase;

	static inline size_t BufferSize()
	{
		return 64 * 1024;
	}

	static inline const char* PackageInfoFileName()
	{
		return BPackageKit::BHPKG::B_HPKG_PACKAGE_INFO_FILE_NAME;
	}

	static inline uint64 PackageDataCompressedSize(const PackageData& data)
	{
		return data.Size();
	}

	static inline uint64 PackageDataUncompressedSize(const PackageData& data)
	{
		return data.Size();
	}

	static inline status_t InitReader(PackageReader& packageReader,
		const char* fileName)
	{
		return packageReader.Init(fileName,
			BPackageKit::BHPKG
				::B_HPKG_READER_DONT_PRINT_VERSION_MISMATCH_MESSAGE);
	}

	static status_t GetHeapReader(PackageReader& packageReader,
		HeapReaderBase*& _heapReader, bool& _mustDelete)
	{
		_heapReader = packageReader.HeapReader();
		_mustDelete = false;
		return B_OK;
	}

	static status_t CreatePackageDataReader(BBufferPool* bufferPool,
		HeapReaderBase* heapReader, const PackageData& data,
		BAbstractBufferedDataReader*& _reader)
	{
		return BPackageKit::BHPKG::BPackageDataReaderFactory()
			.CreatePackageDataReader(heapReader, data, _reader);
	}
};


struct Entry {
	Entry(Entry* parent, char* name, bool implicit)
		:
		fParent(parent),
		fName(name),
		fImplicit(implicit),
		fSeen(false)
	{
	}

	~Entry()
	{
		_DeleteChildren();

		free(fName);
	}

	status_t Init()
	{
		return fChildren.Init();
	}

	static status_t Create(Entry* parent, const char* name, bool implicit,
		Entry*& _entry)
	{
		if (parent != NULL) {
			Entry* entryInParent = parent->FindChild(name);
			if (entryInParent != NULL) {
				_entry = entryInParent;
				return B_OK;
			}
		}

		char* clonedName = strdup(name);
		if (clonedName == NULL)
			return B_NO_MEMORY;

		Entry* entry = new(std::nothrow) Entry(parent, clonedName, implicit);
		if (entry == NULL) {
			free(clonedName);
			return B_NO_MEMORY;
		}

		status_t error = entry->Init();
		if (error != B_OK) {
			delete entry;
			return error;
		}

		if (parent != NULL)
			parent->fChildren.Insert(entry);

		_entry = entry;
		return B_OK;
	}

	Entry* Parent() const
	{
		return fParent;
	}

	const char* Name() const
	{
		return fName;
	}

	bool IsImplicit() const
	{
		return fImplicit;
	}

	void SetExplicit()
	{
		// remove all children and set this entry non-implicit
		_DeleteChildren();
		fImplicit = false;
	}

	void SetSeen()
	{
		fSeen = true;
	}

	bool Seen() const
	{
		return fSeen;
	}

	Entry* FindChild(const char* name) const
	{
		return fChildren.Lookup(name);
	}

private:
	struct ChildHashDefinition {
		typedef const char*		KeyType;
		typedef	Entry			ValueType;

		size_t HashKey(const char* key) const
		{
			return string_hash(key);
		}

		size_t Hash(const Entry* value) const
		{
			return HashKey(value->Name());
		}

		bool Compare(const char* key, const Entry* value) const
		{
			return strcmp(value->Name(), key) == 0;
		}

		Entry*& GetLink(Entry* value) const
		{
			return value->fHashTableNext;
		}
	};

	typedef BOpenHashTable<ChildHashDefinition> ChildTable;

private:
	void _DeleteChildren()
	{
		Entry* child = fChildren.Clear(true);
		while (child != NULL) {
			Entry* next = child->fHashTableNext;
			delete child;
			child = next;
		}
	}

public:
	Entry*		fHashTableNext;

private:
	Entry*		fParent;
	char*		fName;
	bool		fImplicit;
	bool		fSeen;
	ChildTable	fChildren;
};


template<typename VersionPolicy>
struct PackageContentExtractHandler : VersionPolicy::PackageContentHandler {
	PackageContentExtractHandler(BBufferPool* bufferPool,
	typename VersionPolicy::HeapReaderBase* heapReader)
		:
		fBufferPool(bufferPool),
		fPackageFileReader(heapReader),
		fDataBuffer(NULL),
		fDataBufferSize(0),
		fRootFilterEntry(NULL, NULL, true),
		fBaseDirectory(AT_FDCWD),
		fInfoFileName(NULL),
		fErrorOccurred(false)
	{
	}

	~PackageContentExtractHandler()
	{
		free(fDataBuffer);
	}

	status_t Init()
	{
		status_t error = fRootFilterEntry.Init();
		if (error != B_OK)
			return error;

		fDataBufferSize = 64 * 1024;
		fDataBuffer = malloc(fDataBufferSize);
		if (fDataBuffer == NULL)
			return B_NO_MEMORY;

		return B_OK;
	}

	void SetBaseDirectory(int fd)
	{
		fBaseDirectory = fd;
	}

	void SetPackageInfoFile(const char* infoFileName)
	{
		fInfoFileName = infoFileName;
	}

	void SetExtractAll()
	{
		fRootFilterEntry.SetExplicit();
	}

	status_t AddFilterEntry(const char* fileName)
	{
		// add all components of the path
		Entry* entry = &fRootFilterEntry;
		while (*fileName != 0) {
			const char* nextSlash = strchr(fileName, '/');
			// no slash, just add the file name
			if (nextSlash == NULL) {
				return _AddFilterEntry(entry, fileName, strlen(fileName),
					false, entry);
			}

			// find the start of the next component, skipping slashes
			const char* nextComponent = nextSlash + 1;
			while (*nextComponent == '/')
				nextComponent++;

			status_t error = _AddFilterEntry(entry, fileName,
				nextSlash - fileName, *nextComponent != '\0', entry);
			if (error != B_OK)
				return error;

			fileName = nextComponent;
		}

		return B_OK;
	}

	Entry* FindFilterEntry(const char* fileName)
	{
		// add all components of the path
		Entry* entry = &fRootFilterEntry;
		while (entry != NULL && *fileName != 0) {
			const char* nextSlash = strchr(fileName, '/');
			// no slash, just add the file name
			if (nextSlash == NULL)
				return entry->FindChild(fileName);

			// find the start of the next component, skipping slashes
			const char* nextComponent = nextSlash + 1;
			while (*nextComponent == '/')
				nextComponent++;

			BString componentName(fileName, nextSlash - fileName);
			entry = entry->FindChild(componentName);

			fileName = nextComponent;
		}

		return entry;
	}

	virtual status_t HandleEntry(typename VersionPolicy::PackageEntry* entry)
	{
		// create a token
		Token* token = new(std::nothrow) Token;
		if (token == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<Token> tokenDeleter(token);

		// check whether this entry shall be ignored or is implicit
		Entry* parentFilterEntry;
		bool implicit;
		if (entry->Parent() != NULL) {
			Token* parentToken = (Token*)entry->Parent()->UserToken();
			if (parentToken == NULL) {
				// parent is ignored, so ignore this entry, too
				return B_OK;
			}

			parentFilterEntry = parentToken->filterEntry;
			implicit = parentToken->implicit;
		} else {
			parentFilterEntry = &fRootFilterEntry;
			implicit = fRootFilterEntry.IsImplicit();
		}

		Entry* filterEntry = parentFilterEntry != NULL
			? parentFilterEntry->FindChild(entry->Name()) : NULL;

		if (implicit && filterEntry == NULL) {
			// parent is implicit and the filter doesn't include this entry
			// -- ignore it
			return B_OK;
		}

		// If the entry is in the filter, get its implicit flag.
		if (filterEntry != NULL) {
			implicit = filterEntry->IsImplicit();
			filterEntry->SetSeen();
		}

		token->filterEntry = filterEntry;
		token->implicit = implicit;

		// get parent FD and the entry name
		int parentFD;
		const char* entryName;
		_GetParentFDAndEntryName(entry, parentFD, entryName);

		// check whether something is in the way
		struct stat st;
		bool entryExists = fstatat(parentFD, entryName, &st,
			AT_SYMLINK_NOFOLLOW) == 0;
		if (entryExists) {
			if (S_ISREG(entry->Mode()) || S_ISLNK(entry->Mode())) {
				// If the entry in the way is a regular file or a symlink,
				// remove it, otherwise fail.
				if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
					fprintf(stderr, "Error: Can't create entry \"%s\", since "
						"something is in the way\n",
						_EntryPath(entry).String());
					return B_FILE_EXISTS;
				}

				if (unlinkat(parentFD, entryName, 0) != 0) {
					fprintf(stderr, "Error: Failed to unlink entry \"%s\": %s\n",
						_EntryPath(entry).String(), strerror(errno));
					return errno;
				}

				entryExists = false;
			} else if (S_ISDIR(entry->Mode())) {
				// If the entry in the way is a directory, merge, otherwise
				// fail.
				if (!S_ISDIR(st.st_mode)) {
					fprintf(stderr, "Error: Can't create directory \"%s\", "
						"since something is in the way\n",
						_EntryPath(entry).String());
					return B_FILE_EXISTS;
				}
			}
		}

		// create the entry
		int fd = -1;
		if (S_ISREG(entry->Mode())) {
			if (implicit) {
				fprintf(stderr, "Error: File \"%s\" was specified as a "
					"path directory component.\n", _EntryPath(entry).String());
				return B_BAD_VALUE;
			}

			// create the file
			fd = openat(parentFD, entryName, O_RDWR | O_CREAT | O_EXCL,
				S_IRUSR | S_IWUSR);
				// Note: We use read+write user permissions now -- so write
				// operations (e.g. attributes) won't fail, but set them to the
				// desired ones in HandleEntryDone().
			if (fd < 0) {
				fprintf(stderr, "Error: Failed to create file \"%s\": %s\n",
					_EntryPath(entry).String(), strerror(errno));
				return errno;
			}

			// write data
			status_t error = _ExtractFileData(fPackageFileReader, entry->Data(),
				fd);
			if (error != B_OK)
				return error;
		} else if (S_ISLNK(entry->Mode())) {
			if (implicit) {
				fprintf(stderr, "Error: Symlink \"%s\" was specified as a "
					"path directory component.\n", _EntryPath(entry).String());
				return B_BAD_VALUE;
			}

			// create the symlink
			const char* symlinkPath = entry->SymlinkPath();
			if (symlinkat(symlinkPath != NULL ? symlinkPath : "", parentFD,
					entryName) != 0) {
				fprintf(stderr, "Error: Failed to create symlink \"%s\": %s\n",
					_EntryPath(entry).String(), strerror(errno));
				return errno;
			}
// TODO: Set symlink permissions?
 		} else if (S_ISDIR(entry->Mode())) {
			// create the directory, if necessary
			if (!entryExists
				&& mkdirat(parentFD, entryName, S_IRWXU) != 0) {
				// Note: We use read+write+exec user permissions now -- so write
				// operations (e.g. attributes) won't fail, but set them to the
				// desired ones in HandleEntryDone().
				fprintf(stderr, "Error: Failed to create directory \"%s\": "
					"%s\n", _EntryPath(entry).String(), strerror(errno));
				return errno;
			}
		} else {
			fprintf(stderr, "Error: Invalid file type for entry \"%s\"\n",
				_EntryPath(entry).String());
			return B_BAD_DATA;
		}

		// If not done yet (symlink, dir), open the node -- we need the FD.
		if (fd < 0 && (!implicit || S_ISDIR(entry->Mode()))) {
			fd = openat(parentFD, entryName, O_RDONLY | O_NOTRAVERSE);
			if (fd < 0) {
				fprintf(stderr, "Error: Failed to open entry \"%s\": %s\n",
					_EntryPath(entry).String(), strerror(errno));
				return errno;
			}
		}
		token->fd = fd;

		// set the file times
		if (!entryExists && !implicit) {
			timespec times[2] = {entry->AccessTime(), entry->ModifiedTime()};
			futimens(fd, times);

			// set user/group
			// TODO:...
		}

		entry->SetUserToken(tokenDeleter.Detach());
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(
		typename VersionPolicy::PackageEntry* entry,
		typename VersionPolicy::PackageEntryAttribute* attribute)
	{
		// don't write attributes of ignored or implicit entries
		Token* token = (Token*)entry->UserToken();
		if (token == NULL || token->implicit)
			return B_OK;

		int entryFD = token->fd;

		// create the attribute
		int fd = fs_fopen_attr(entryFD, attribute->Name(), attribute->Type(),
			O_WRONLY | O_CREAT | O_TRUNC);
		if (fd < 0) {
			int parentFD;
			const char* entryName;
			_GetParentFDAndEntryName(entry, parentFD, entryName);

			fprintf(stderr, "Error: Failed to create attribute \"%s\" of "
				"file \"%s\": %s\n", attribute->Name(),
				_EntryPath(entry).String(), strerror(errno));
			return errno;
		}

		// write data
		status_t error = _ExtractFileData(fPackageFileReader, attribute->Data(),
			fd);

		fs_close_attr(fd);

		return error;
	}

	virtual status_t HandleEntryDone(
		typename VersionPolicy::PackageEntry* entry)
	{
		Token* token = (Token*)entry->UserToken();

		// set the node permissions for non-symlinks
		if (token != NULL && !S_ISLNK(entry->Mode())) {
			// get parent FD and entry name
			int parentFD;
			const char* entryName;
			_GetParentFDAndEntryName(entry, parentFD, entryName);

			if (fchmodat(parentFD, entryName, entry->Mode() & ALLPERMS,
					/*AT_SYMLINK_NOFOLLOW*/0) != 0) {
				fprintf(stderr, "Warning: Failed to set permissions of file "
					"\"%s\": %s\n", _EntryPath(entry).String(),
					strerror(errno));
			}
		}

		if (token != NULL) {
			delete token;
			entry->SetUserToken(NULL);
		}

		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	struct Token {
		Entry*	filterEntry;
		int		fd;
		bool	implicit;

		Token()
			:
			filterEntry(NULL),
			fd(-1),
			implicit(true)
		{
		}

		~Token()
		{
			if (fd >= 0)
				close(fd);
		}
	};

private:
	status_t _AddFilterEntry(Entry* parentEntry, const char* _name,
		size_t nameLength, bool implicit, Entry*& _entry)
	{
		BString name(_name, nameLength);
		if (name.IsEmpty())
			return B_NO_MEMORY;

		return Entry::Create(parentEntry, name.String(), implicit, _entry);
	}

	void _GetParentFDAndEntryName(typename VersionPolicy::PackageEntry* entry,
		int& _parentFD, const char*& _entryName)
	{
		_entryName = entry->Name();

		if (fInfoFileName != NULL
			&& strcmp(_entryName, VersionPolicy::PackageInfoFileName()) == 0) {
			_parentFD = AT_FDCWD;
			_entryName = fInfoFileName;
		} else {
			_parentFD = entry->Parent() != NULL
				? ((Token*)entry->Parent()->UserToken())->fd : fBaseDirectory;
		}
	}

	BString _EntryPath(const typename VersionPolicy::PackageEntry* entry)
	{
		BString path;

		if (const typename VersionPolicy::PackageEntry* parent
				= entry->Parent()) {
			path = _EntryPath(parent);
			path << '/';
		}

		path << entry->Name();
		return path;
	}

	status_t _ExtractFileData(
		typename VersionPolicy::HeapReaderBase* dataReader,
		const typename VersionPolicy::PackageData& data, int fd)
	{
		// create a PackageDataReader
		BAbstractBufferedDataReader* reader;
		status_t error = VersionPolicy::CreatePackageDataReader(fBufferPool,
			dataReader, data, reader);
		if (error != B_OK)
			return error;
		ObjectDeleter<BAbstractBufferedDataReader> readerDeleter(reader);

		// write the data
		off_t bytesRemaining = VersionPolicy::PackageDataUncompressedSize(data);
		off_t offset = 0;
		while (bytesRemaining > 0) {
			// read
			size_t toCopy = std::min((off_t)fDataBufferSize, bytesRemaining);
			error = reader->ReadData(offset, fDataBuffer, toCopy);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to read data: %s\n",
					strerror(error));
				return error;
			}

			// write
			ssize_t bytesWritten = write_pos(fd, offset, fDataBuffer, toCopy);
			if (bytesWritten < 0) {
				fprintf(stderr, "Error: Failed to write data: %s\n",
					strerror(errno));
				return errno;
			}
			if ((size_t)bytesWritten != toCopy) {
				fprintf(stderr, "Error: Failed to write all data (%zd of "
					"%zu)\n", bytesWritten, toCopy);
				return B_ERROR;
			}

			offset += toCopy;
			bytesRemaining -= toCopy;
		}

		return B_OK;
	}

private:
	BBufferPool*							fBufferPool;
	typename VersionPolicy::HeapReaderBase*	fPackageFileReader;
	void*									fDataBuffer;
	size_t									fDataBufferSize;
	Entry									fRootFilterEntry;
	int										fBaseDirectory;
	const char*								fInfoFileName;
	bool									fErrorOccurred;
};


template<typename VersionPolicy>
static void
do_extract(const char* packageFileName, const char* changeToDirectory,
	const char* packageInfoFileName, const char* const* explicitEntries,
	int explicitEntryCount, bool ignoreVersionError)
{
	// open package
	BStandardErrorOutput errorOutput;
	BBlockBufferPoolNoLock bufferPool(VersionPolicy::BufferSize(), 2);
	if (bufferPool.Init() != B_OK) {
		errorOutput.PrintError("Error: Out of memory!\n");
		exit(1);
	}

	typename VersionPolicy::PackageReader packageReader(&errorOutput);
	status_t error = VersionPolicy::InitReader(packageReader, packageFileName);
	if (error != B_OK) {
		if (ignoreVersionError && error == B_MISMATCHED_VALUES)
			return;
		exit(1);
	}

	typename VersionPolicy::HeapReaderBase* heapReader;
	bool mustDeleteHeapReader;
	error = VersionPolicy::GetHeapReader(packageReader, heapReader,
		mustDeleteHeapReader);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to create heap reader: \"%s\"\n",
			strerror(error));
		exit(1);
	}
	ObjectDeleter<BDataReader> heapReaderDeleter(
		mustDeleteHeapReader ? heapReader : NULL);

	PackageContentExtractHandler<VersionPolicy> handler(&bufferPool,
		heapReader);
	error = handler.Init();
	if (error != B_OK)
		exit(1);

	// If entries to extract have been specified explicitly, add those to the
	// filtered ones.
	if (explicitEntryCount > 0) {
		for (int i = 0; i < explicitEntryCount; i++) {
			const char* entryName = explicitEntries[i];
			if (entryName[0] == '\0' || entryName[0] == '/') {
				fprintf(stderr, "Error: Invalid entry name: \"%s\"\n",
					entryName);
				exit(1);
			}
			if (handler.AddFilterEntry(entryName) != B_OK)
				exit(1);
		}
	} else
		handler.SetExtractAll();

	// get the target directory, if requested
	if (changeToDirectory != NULL) {
		int currentDirFD = open(changeToDirectory, O_RDONLY);
		if (currentDirFD < 0) {
			fprintf(stderr, "Error: Failed to change the current working "
				"directory to \"%s\": %s\n", changeToDirectory,
				strerror(errno));
			exit(1);
		}

		handler.SetBaseDirectory(currentDirFD);
	}

	// If a package info file name is given, set it.
	if (packageInfoFileName != NULL)
		handler.SetPackageInfoFile(packageInfoFileName);

	// extract
	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		exit(1);

	// check whether all explicitly specified entries have been extracted
	if (explicitEntryCount > 0) {
		for (int i = 0; i < explicitEntryCount; i++) {
			if (Entry* entry = handler.FindFilterEntry(explicitEntries[i])) {
				if (!entry->Seen()) {
					fprintf(stderr, "Warning: Entry \"%s\" not found.\n",
						explicitEntries[i]);
				}
			}
		}
	}

	exit(0);
}


int
command_extract(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;
	const char* packageInfoFileName = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+C:hi:", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'i':
				packageInfoFileName = optarg;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// At least one argument should remain -- the package file name. Any further
	// arguments are the names of the entries to extract.
	if (optind + 1 > argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];
	const char* const* explicitEntries = argv + optind;
	int explicitEntryCount = argc - optind;
	do_extract<VersionPolicyV2>(packageFileName, changeToDirectory,
		packageInfoFileName, explicitEntries, explicitEntryCount, true);
	do_extract<VersionPolicyV1>(packageFileName, changeToDirectory,
		packageInfoFileName, explicitEntries, explicitEntryCount, false);

	return 0;
}

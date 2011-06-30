/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageWriterImpl.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <Path.h>

#include <AutoDeleter.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/Stacker.h>


using BPrivate::FileDescriptorCloser;


static const char* const kPublicDomainLicenseName = "Public Domain";


#include <typeinfo>

namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// minimum length of data we require before trying to zlib compress them
static const size_t kZlibCompressionSizeThreshold = 64;


// #pragma mark - Attributes


struct PackageWriterImpl::Attribute
	: public DoublyLinkedListLinkImpl<Attribute> {
	BHPKGAttributeID			id;
	AttributeValue				value;
	DoublyLinkedList<Attribute>	children;

	Attribute(BHPKGAttributeID id_ = B_HPKG_ATTRIBUTE_ID_ENUM_COUNT)
		:
		id(id_)
	{
	}

	~Attribute()
	{
		DeleteChildren();
	}

	void AddChild(Attribute* child)
	{
		children.Add(child);
	}

	void DeleteChildren()
	{
		while (Attribute* child = children.RemoveHead())
			delete child;
	}
};


// #pragma mark - Entry


struct PackageWriterImpl::Entry : DoublyLinkedListLinkImpl<Entry> {
	Entry(char* name, size_t nameLength, bool isImplicit)
		:
		fName(name),
		fNameLength(nameLength),
		fIsImplicit(isImplicit)
	{
	}

	~Entry()
	{
		DeleteChildren();
		free(fName);
	}

	static Entry* Create(const char* name, size_t nameLength, bool isImplicit)
	{
		char* clonedName = (char*)malloc(nameLength + 1);
		if (clonedName == NULL)
			throw std::bad_alloc();
		memcpy(clonedName, name, nameLength);
		clonedName[nameLength] = '\0';

		Entry* entry = new(std::nothrow) Entry(clonedName, nameLength,
			isImplicit);
		if (entry == NULL) {
			free(clonedName);
			throw std::bad_alloc();
		}

		return entry;
	}

	const char* Name() const
	{
		return fName;
	}

	bool IsImplicit() const
	{
		return fIsImplicit;
	}

	void SetImplicit(bool isImplicit)
	{
		fIsImplicit = isImplicit;
	}

	bool HasName(const char* name, size_t nameLength)
	{
		return nameLength == fNameLength
			&& strncmp(name, fName, nameLength) == 0;
	}

	void AddChild(Entry* child)
	{
		fChildren.Add(child);
	}

	void DeleteChildren()
	{
		while (Entry* child = fChildren.RemoveHead())
			delete child;
	}

	Entry* GetChild(const char* name, size_t nameLength) const
	{
		EntryList::ConstIterator it = fChildren.GetIterator();
		while (Entry* child = it.Next()) {
			if (child->HasName(name, nameLength))
				return child;
		}

		return NULL;
	}

	EntryList::ConstIterator ChildIterator() const
	{
		return fChildren.GetIterator();
	}

private:
	char*		fName;
	size_t		fNameLength;
	bool		fIsImplicit;
	EntryList	fChildren;
};


// #pragma mark - SubPathAdder


struct PackageWriterImpl::SubPathAdder {
	SubPathAdder(char* pathBuffer, const char* subPath)
		: fOriginalPathEnd(pathBuffer + strlen(pathBuffer))
	{
		strcat(pathBuffer, "/");
		strcat(pathBuffer, subPath);
	}

	~SubPathAdder()
	{
		*fOriginalPathEnd = '\0';
	}
private:
	char* fOriginalPathEnd;
};


// #pragma mark - PackageWriterImpl (Inline Methods)


template<typename Type>
inline PackageWriterImpl::Attribute*
PackageWriterImpl::_AddAttribute(BHPKGAttributeID attributeID, Type value)
{
	AttributeValue attributeValue;
	attributeValue.SetTo(value);
	return _AddAttribute(attributeID, attributeValue);
}


// #pragma mark - PackageWriterImpl


PackageWriterImpl::PackageWriterImpl(BPackageWriterListener* listener)
	:
	inherited(listener),
	fListener(listener),
	fDataBuffer(NULL),
	fDataBufferSize(2 * B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB),
	fRootEntry(NULL),
	fRootAttribute(NULL),
	fTopAttribute(NULL)
{
}


PackageWriterImpl::~PackageWriterImpl()
{
	delete fRootAttribute;

	delete fRootEntry;

	free(fDataBuffer);
}


status_t
PackageWriterImpl::Init(const char* fileName)
{
	try {
		return _Init(fileName);
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
PackageWriterImpl::AddEntry(const char* fileName)
{
	try {
		// if it's ".PackageInfo", parse it
		if (strcmp(fileName, B_HPKG_PACKAGE_INFO_FILE_NAME) == 0) {
			struct ErrorListener : public BPackageInfo::ParseErrorListener {
				ErrorListener(BPackageWriterListener* _listener)
					: listener(_listener) {}
				virtual void OnError(const BString& msg, int line, int col) {
					listener->PrintError("Parse error in %s(%d:%d) -> %s\n",
						B_HPKG_PACKAGE_INFO_FILE_NAME, line, col, msg.String());
				}
				BPackageWriterListener* listener;
			} errorListener(fListener);
			BEntry packageInfoEntry(fileName);
			status_t result = fPackageInfo.ReadFromConfigFile(packageInfoEntry,
				&errorListener);
			if (result != B_OK || (result = fPackageInfo.InitCheck()) != B_OK)
				return result;
			RegisterPackageInfo(PackageAttributes(), fPackageInfo);
		}

		return _RegisterEntry(fileName);
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
PackageWriterImpl::Finish()
{
	try {
		if (fPackageInfo.InitCheck() != B_OK) {
			fListener->PrintError("No package-info file found (%s)!\n",
				B_HPKG_PACKAGE_INFO_FILE_NAME);
			return B_BAD_DATA;
		}

		status_t result = _CheckLicenses();
		if (result != B_OK)
			return result;

		return _Finish();
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
PackageWriterImpl::_Init(const char* fileName)
{
	status_t result = inherited::Init(fileName, "package");
	if (result != B_OK)
		return result;

	// allocate data buffer
	fDataBuffer = malloc(fDataBufferSize);
	if (fDataBuffer == NULL)
		throw std::bad_alloc();

	if (fStringCache.Init() != B_OK)
		throw std::bad_alloc();

	// create entry list
	fRootEntry = new Entry(NULL, 0, true);

	fRootAttribute = new Attribute();

	fHeapOffset = fHeapEnd = sizeof(hpkg_header);
	fTopAttribute = fRootAttribute;

	return B_OK;
}


status_t
PackageWriterImpl::_CheckLicenses()
{
	BPath systemLicensePath;
	status_t result
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		= find_directory(B_SYSTEM_DATA_DIRECTORY, &systemLicensePath);
#else
		= systemLicensePath.SetTo(HAIKU_BUILD_SYSTEM_DATA_DIRECTORY);
#endif
	if (result != B_OK) {
		fListener->PrintError("unable to find system data path!\n");
		return result;
	}
	if ((result = systemLicensePath.Append("licenses")) != B_OK) {
		fListener->PrintError("unable to append to system data path!\n");
		return result;
	}

	BDirectory systemLicenseDir(systemLicensePath.Path());
	BDirectory packageLicenseDir("./data/licenses");

	const BObjectList<BString>& licenseList = fPackageInfo.LicenseList();
	for (int i = 0; i < licenseList.CountItems(); ++i) {
		const BString& licenseName = *licenseList.ItemAt(i);
		if (licenseName == kPublicDomainLicenseName)
			continue;

		BEntry license;
		if (systemLicenseDir.FindEntry(licenseName.String(), &license) == B_OK)
			continue;

		// license is not a system license, so it must be contained in package
		if (packageLicenseDir.FindEntry(licenseName.String(),
				&license) != B_OK) {
			fListener->PrintError("License '%s' isn't contained in package!\n",
				licenseName.String());
			return B_BAD_DATA;
		}
	}

	return B_OK;
}


status_t
PackageWriterImpl::_Finish()
{
	// write entries
	for (EntryList::ConstIterator it = fRootEntry->ChildIterator();
			Entry* entry = it.Next();) {
		char pathBuffer[B_PATH_NAME_LENGTH];
		pathBuffer[0] = '\0';
		_AddEntry(AT_FDCWD, entry, entry->Name(), pathBuffer);
	}

	off_t heapSize = fHeapEnd - sizeof(hpkg_header);

	hpkg_header header;

	// write the TOC and package attributes
	_WriteTOC(header);
	_WritePackageAttributes(header);

	off_t totalSize = fHeapEnd;

	fListener->OnPackageSizeInfo(sizeof(hpkg_header), heapSize,
		B_BENDIAN_TO_HOST_INT64(header.toc_length_compressed),
		B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed),
		totalSize);

	// prepare the header

	// general
	header.magic = B_HOST_TO_BENDIAN_INT32(B_HPKG_MAGIC);
	header.header_size = B_HOST_TO_BENDIAN_INT16(
		(uint16)sizeof(hpkg_header));
	header.version = B_HOST_TO_BENDIAN_INT16(B_HPKG_VERSION);
	header.total_size = B_HOST_TO_BENDIAN_INT64(totalSize);

	// write the header
	WriteBuffer(&header, sizeof(hpkg_header), 0);

	SetFinished(true);
	return B_OK;
}


status_t
PackageWriterImpl::_RegisterEntry(const char* fileName)
{
	if (*fileName == '\0') {
		fListener->PrintError("Invalid empty file name\n");
		return B_BAD_VALUE;
	}

	// add all components of the path
	Entry* entry = fRootEntry;
	while (*fileName != 0) {
		const char* nextSlash = strchr(fileName, '/');
		// no slash, just add the file name
		if (nextSlash == NULL) {
			entry = _RegisterEntry(entry, fileName, strlen(fileName), false);
			break;
		}

		// find the start of the next component, skipping slashes
		const char* nextComponent = nextSlash + 1;
		while (*nextComponent == '/')
			nextComponent++;

		if (nextSlash == fileName) {
			// the FS root
			entry = _RegisterEntry(entry, fileName, 1,
				*nextComponent != '\0');
		} else {
			entry = _RegisterEntry(entry, fileName, nextSlash - fileName,
				*nextComponent != '\0');
		}

		fileName = nextComponent;
	}

	return B_OK;
}


PackageWriterImpl::Entry*
PackageWriterImpl::_RegisterEntry(Entry* parent, const char* name,
	size_t nameLength, bool isImplicit)
{
	// check the component name -- don't allow "." or ".."
	if (name[0] == '.'
		&& (nameLength == 1 || (nameLength == 2 && name[1] == '.'))) {
		fListener->PrintError("Invalid file name: \".\" and \"..\" "
			"are not allowed as path components\n");
		throw status_t(B_BAD_VALUE);
	}

	// the entry might already exist
	Entry* entry = parent->GetChild(name, nameLength);
	if (entry != NULL) {
		// If the entry was implicit and is no longer, we mark it non-implicit
		// and delete all of it's children.
		if (entry->IsImplicit() && !isImplicit) {
			entry->DeleteChildren();
			entry->SetImplicit(false);
		}
	} else {
		// nope -- create it
		entry = Entry::Create(name, nameLength, isImplicit);
		parent->AddChild(entry);
	}

	return entry;
}


void
PackageWriterImpl::_WriteTOC(hpkg_header& header)
{
	// prepare the writer (zlib writer on top of a file writer)
	off_t startOffset = fHeapEnd;
	FDDataWriter realWriter(FD(), startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	// write the sections
	uint64 uncompressedStringsSize;
	uint64 uncompressedMainSize;
	int32 cachedStringsWritten
		= _WriteTOCSections(uncompressedStringsSize, uncompressedMainSize);

	// finish the writer
	zlibWriter.Finish();
	fHeapEnd = realWriter.Offset();
	SetDataWriter(NULL);

	off_t endOffset = fHeapEnd;

	fListener->OnTOCSizeInfo(uncompressedStringsSize, uncompressedMainSize,
		zlibWriter.BytesWritten());

	// update the header

	// TOC
	header.toc_compression = B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.toc_length_compressed = B_HOST_TO_BENDIAN_INT64(
		endOffset - startOffset);
	header.toc_length_uncompressed = B_HOST_TO_BENDIAN_INT64(
		zlibWriter.BytesWritten());

	// TOC subsections
	header.toc_strings_length = B_HOST_TO_BENDIAN_INT64(
		uncompressedStringsSize);
	header.toc_strings_count = B_HOST_TO_BENDIAN_INT64(cachedStringsWritten);
}


int32
PackageWriterImpl::_WriteTOCSections(uint64& _stringsSize, uint64& _mainSize)
{
	// write the cached strings
	uint64 cachedStringsOffset = DataWriter()->BytesWritten();
	int32 cachedStringsWritten = WriteCachedStrings(fStringCache, 2);

	// write the main TOC section
	uint64 mainOffset = DataWriter()->BytesWritten();
	_WriteAttributeChildren(fRootAttribute);

	_stringsSize = mainOffset - cachedStringsOffset;
	_mainSize = DataWriter()->BytesWritten() - mainOffset;

	return cachedStringsWritten;
}


void
PackageWriterImpl::_WriteAttributeChildren(Attribute* attribute)
{
	DoublyLinkedList<Attribute>::Iterator it
		= attribute->children.GetIterator();
	while (Attribute* child = it.Next()) {
		// write tag
		uint8 encoding = child->value.ApplicableEncoding();
		WriteUnsignedLEB128(HPKG_ATTRIBUTE_TAG_COMPOSE(child->id,
			child->value.type, encoding, !child->children.IsEmpty()));

		// write value
		WriteAttributeValue(child->value, encoding);

		if (!child->children.IsEmpty())
			_WriteAttributeChildren(child);
	}

	WriteUnsignedLEB128(0);
}


void
PackageWriterImpl::_WritePackageAttributes(hpkg_header& header)
{
	// write the package attributes (zlib writer on top of a file writer)
	off_t startOffset = fHeapEnd;
	FDDataWriter realWriter(FD(), startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	// write cached strings and package attributes tree
	uint32 stringsLengthUncompressed;
	uint32 stringsCount = WritePackageAttributes(PackageAttributes(),
		stringsLengthUncompressed);

	zlibWriter.Finish();
	fHeapEnd = realWriter.Offset();
	SetDataWriter(NULL);

	off_t endOffset = fHeapEnd;

	fListener->OnPackageAttributesSizeInfo(stringsCount,
		zlibWriter.BytesWritten());

	// update the header
	header.attributes_compression
		= B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.attributes_length_compressed
		= B_HOST_TO_BENDIAN_INT32(endOffset - startOffset);
	header.attributes_length_uncompressed
		= B_HOST_TO_BENDIAN_INT32(zlibWriter.BytesWritten());
	header.attributes_strings_count = B_HOST_TO_BENDIAN_INT32(stringsCount);
	header.attributes_strings_length
		= B_HOST_TO_BENDIAN_INT32(stringsLengthUncompressed);
}


void
PackageWriterImpl::_AddEntry(int dirFD, Entry* entry, const char* fileName,
	char* pathBuffer)
{
	bool isImplicitEntry = entry != NULL && entry->IsImplicit();

	SubPathAdder pathAdder(pathBuffer, fileName);
	if (!isImplicitEntry) {
		fListener->OnEntryAdded(pathBuffer + 1);
			// pathBuffer + 1 in order to skip leading slash
	}

	// open the node
	int fd = openat(dirFD, fileName,
		O_RDONLY | (isImplicitEntry ? 0 : O_NOTRAVERSE));
	if (fd < 0) {
		fListener->PrintError("Failed to open entry \"%s\": %s\n", fileName,
			strerror(errno));
		throw status_t(errno);
	}
	FileDescriptorCloser fdCloser(fd);

	// stat the node
	struct stat st;
	if (fstat(fd, &st) < 0) {
		fListener->PrintError("Failed to fstat() file \"%s\": %s\n", fileName,
			strerror(errno));
		throw status_t(errno);
	}

	// implicit entries must be directories
	if (isImplicitEntry && !S_ISDIR(st.st_mode)) {
		fListener->PrintError("Non-leaf path component \"%s\" is not a "
			"directory\n", fileName);
		throw status_t(B_BAD_VALUE);
	}

	// check/translate the node type
	uint8 fileType;
	uint32 defaultPermissions;
	if (S_ISREG(st.st_mode)) {
		fileType = B_HPKG_FILE_TYPE_FILE;
		defaultPermissions = B_HPKG_DEFAULT_FILE_PERMISSIONS;
	} else if (S_ISLNK(st.st_mode)) {
		fileType = B_HPKG_FILE_TYPE_SYMLINK;
		defaultPermissions = B_HPKG_DEFAULT_SYMLINK_PERMISSIONS;
	} else if (S_ISDIR(st.st_mode)) {
		fileType = B_HPKG_FILE_TYPE_DIRECTORY;
		defaultPermissions = B_HPKG_DEFAULT_DIRECTORY_PERMISSIONS;
	} else {
		// unsupported node type
		fListener->PrintError("Unsupported node type, entry: \"%s\"\n",
			fileName);
		throw status_t(B_UNSUPPORTED);
	}

	// add attribute entry
	Attribute* entryAttribute = _AddStringAttribute(
		B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY, fileName);
	Stacker<Attribute> entryAttributeStacker(fTopAttribute, entryAttribute);

	// add stat data
	if (fileType != B_HPKG_DEFAULT_FILE_TYPE)
		_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_TYPE, fileType);
	if (defaultPermissions != uint32(st.st_mode & ALLPERMS)) {
		_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_PERMISSIONS,
			uint32(st.st_mode & ALLPERMS));
	}
	_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_ATIME, uint32(st.st_atime));
	_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_MTIME, uint32(st.st_mtime));
#ifdef __HAIKU__
	_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_CRTIME, uint32(st.st_crtime));
#else
	_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_CRTIME, uint32(st.st_mtime));
#endif
	// TODO: File user/group!

	// add file data/symlink path
	if (S_ISREG(st.st_mode)) {
		// regular file -- add data
		if (st.st_size > 0) {
			BFDDataReader dataReader(fd);
			status_t error = _AddData(dataReader, st.st_size);
			if (error != B_OK)
				throw status_t(error);
		}
	} else if (S_ISLNK(st.st_mode)) {
		// symlink -- add link address
		char path[B_PATH_NAME_LENGTH + 1];
		ssize_t bytesRead = readlinkat(dirFD, fileName, path,
			B_PATH_NAME_LENGTH);
		if (bytesRead < 0) {
			fListener->PrintError("Failed to read symlink \"%s\": %s\n",
				fileName, strerror(errno));
			throw status_t(errno);
		}

		path[bytesRead] = '\0';
		_AddStringAttribute(B_HPKG_ATTRIBUTE_ID_SYMLINK_PATH, path);
	}

	// add attributes
	if (DIR* attrDir = fs_fopen_attr_dir(fd)) {
		CObjectDeleter<DIR, int> attrDirCloser(attrDir, fs_close_attr_dir);

		while (dirent* entry = fs_read_attr_dir(attrDir)) {
			attr_info attrInfo;
			if (fs_stat_attr(fd, entry->d_name, &attrInfo) < 0) {
				fListener->PrintError(
					"Failed to stat attribute \"%s\" of file \"%s\": %s\n",
					entry->d_name, fileName, strerror(errno));
				throw status_t(errno);
			}

			// create attribute entry
			Attribute* attributeAttribute = _AddStringAttribute(
				B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE, entry->d_name);
			Stacker<Attribute> attributeAttributeStacker(fTopAttribute,
				attributeAttribute);

			// add type
			_AddAttribute(B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE_TYPE,
				(uint32)attrInfo.type);

			// add data
			BAttributeDataReader dataReader(fd, entry->d_name, attrInfo.type);
			status_t error = _AddData(dataReader, attrInfo.size);
			if (error != B_OK)
				throw status_t(error);
		}
	}

	if (S_ISDIR(st.st_mode)) {
		// directory -- recursively add children
		if (isImplicitEntry) {
			// this is an implicit entry -- just add it's children
			for (EntryList::ConstIterator it = entry->ChildIterator();
					Entry* child = it.Next();) {
				_AddEntry(fd, child, child->Name(), pathBuffer);
			}
		} else {
			// we need to clone the directory FD for fdopendir()
			int clonedFD = dup(fd);
			if (clonedFD < 0) {
				fListener->PrintError(
					"Failed to dup() directory FD: %s\n", strerror(errno));
				throw status_t(errno);
			}

			DIR* dir = fdopendir(clonedFD);
			if (dir == NULL) {
				fListener->PrintError(
					"Failed to open directory \"%s\": %s\n", fileName,
					strerror(errno));
				close(clonedFD);
				throw status_t(errno);
			}
			CObjectDeleter<DIR, int> dirCloser(dir, closedir);

			while (dirent* entry = readdir(dir)) {
				// skip "." and ".."
				if (strcmp(entry->d_name, ".") == 0
					|| strcmp(entry->d_name, "..") == 0) {
					continue;
				}

				_AddEntry(fd, NULL, entry->d_name, pathBuffer);
			}
		}
	}
}


PackageWriterImpl::Attribute*
PackageWriterImpl::_AddAttribute(BHPKGAttributeID id,
	const AttributeValue& value)
{
	Attribute* attribute = new Attribute(id);

	attribute->value = value;
	fTopAttribute->AddChild(attribute);

	return attribute;
}


PackageWriterImpl::Attribute*
PackageWriterImpl::_AddStringAttribute(BHPKGAttributeID attributeID,
	const char* value)
{
	AttributeValue attributeValue;
	attributeValue.SetTo(fStringCache.Get(value));
	return _AddAttribute(attributeID, attributeValue);
}


PackageWriterImpl::Attribute*
PackageWriterImpl::_AddDataAttribute(BHPKGAttributeID attributeID,
	uint64 dataSize, uint64 dataOffset)
{
	AttributeValue attributeValue;
	attributeValue.SetToData(dataSize, dataOffset);
	return _AddAttribute(attributeID, attributeValue);
}


PackageWriterImpl::Attribute*
PackageWriterImpl::_AddDataAttribute(BHPKGAttributeID attributeID,
	uint64 dataSize, const uint8* data)
{
	AttributeValue attributeValue;
	attributeValue.SetToData(dataSize, data);
	return _AddAttribute(attributeID, attributeValue);
}


status_t
PackageWriterImpl::_AddData(BDataReader& dataReader, off_t size)
{
	// add short data inline
	if (size <= B_HPKG_MAX_INLINE_DATA_SIZE) {
		uint8 buffer[B_HPKG_MAX_INLINE_DATA_SIZE];
		status_t error = dataReader.ReadData(0, buffer, size);
		if (error != B_OK) {
			fListener->PrintError("Failed to read data: %s\n", strerror(error));
			return error;
		}

		_AddDataAttribute(B_HPKG_ATTRIBUTE_ID_DATA, size, buffer);
		return B_OK;
	}

	// longer data -- try to compress
	uint64 dataOffset = fHeapEnd;

	uint64 compression = B_HPKG_COMPRESSION_NONE;
	uint64 compressedSize;

	status_t error = _WriteZlibCompressedData(dataReader, size, dataOffset,
		compressedSize);
	if (error == B_OK) {
		compression = B_HPKG_COMPRESSION_ZLIB;
	} else {
		error = _WriteUncompressedData(dataReader, size, dataOffset);
		compressedSize = size;
	}
	if (error != B_OK)
		return error;

	fHeapEnd = dataOffset + compressedSize;

	// add data attribute
	Attribute* dataAttribute = _AddDataAttribute(B_HPKG_ATTRIBUTE_ID_DATA,
		compressedSize, dataOffset - fHeapOffset);
	Stacker<Attribute> attributeAttributeStacker(fTopAttribute, dataAttribute);

	// if compressed, add compression attributes
	if (compression != B_HPKG_COMPRESSION_NONE) {
		_AddAttribute(B_HPKG_ATTRIBUTE_ID_DATA_COMPRESSION, compression);
		_AddAttribute(B_HPKG_ATTRIBUTE_ID_DATA_SIZE, (uint64)size);
			// uncompressed size
	}

	return B_OK;
}


status_t
PackageWriterImpl::_WriteUncompressedData(BDataReader& dataReader, off_t size,
	uint64 writeOffset)
{
	// copy the data to the heap
	off_t readOffset = 0;
	off_t remainingSize = size;
	while (remainingSize > 0) {
		// read data
		size_t toCopy = std::min(remainingSize, (off_t)fDataBufferSize);
		status_t error = dataReader.ReadData(readOffset, fDataBuffer, toCopy);
		if (error != B_OK) {
			fListener->PrintError("Failed to read data: %s\n", strerror(error));
			return error;
		}

		// write to heap
		ssize_t bytesWritten = pwrite(FD(), fDataBuffer, toCopy, writeOffset);
		if (bytesWritten < 0) {
			fListener->PrintError("Failed to write data: %s\n",
				strerror(errno));
			return errno;
		}
		if ((size_t)bytesWritten != toCopy) {
			fListener->PrintError("Failed to write all data\n");
			return B_ERROR;
		}

		remainingSize -= toCopy;
		readOffset += toCopy;
		writeOffset += toCopy;
	}

	return B_OK;
}


status_t
PackageWriterImpl::_WriteZlibCompressedData(BDataReader& dataReader, off_t size,
	uint64 writeOffset, uint64& _compressedSize)
{
	// Use zlib compression only for data large enough.
	if (size < (off_t)kZlibCompressionSizeThreshold)
		return B_BAD_VALUE;

	// fDataBuffer is 2 * B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB, so split it into
	// two halves we can use for reading and compressing
	const size_t chunkSize = B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;
	uint8* inputBuffer = (uint8*)fDataBuffer;
	uint8* outputBuffer = (uint8*)fDataBuffer + chunkSize;

	// account for the offset table
	uint64 chunkCount = (size + (chunkSize - 1)) / chunkSize;
	off_t offsetTableOffset = writeOffset;
	uint64* offsetTable = NULL;
	if (chunkCount > 1) {
		offsetTable = new uint64[chunkCount - 1];
		writeOffset = offsetTableOffset + (chunkCount - 1) * sizeof(uint64);
	}
	ArrayDeleter<uint64> offsetTableDeleter(offsetTable);

	const uint64 dataOffset = writeOffset;
	const uint64 dataEndLimit = offsetTableOffset + size;

	// read the data, compress them and write them to the heap
	off_t readOffset = 0;
	off_t remainingSize = size;
	uint64 chunkIndex = 0;
	while (remainingSize > 0) {
		// read data
		size_t toCopy = std::min(remainingSize, (off_t)chunkSize);
		status_t error = dataReader.ReadData(readOffset, inputBuffer, toCopy);
		if (error != B_OK) {
			fListener->PrintError("Failed to read data: %s\n", strerror(error));
			return error;
		}

		// compress
		size_t compressedSize;
		error = ZlibCompressor::CompressSingleBuffer(inputBuffer, toCopy,
			outputBuffer, toCopy, compressedSize);

		const void* writeBuffer;
		size_t bytesToWrite;
		if (error == B_OK) {
			writeBuffer = outputBuffer;
			bytesToWrite = compressedSize;
		} else {
			if (error != B_BUFFER_OVERFLOW)
				return error;
			writeBuffer = inputBuffer;
			bytesToWrite = toCopy;
		}

		// check the total compressed data size
		if (writeOffset + bytesToWrite >= dataEndLimit)
			return B_BUFFER_OVERFLOW;

		if (chunkIndex > 0)
			offsetTable[chunkIndex - 1] = writeOffset - dataOffset;

		// write to heap
		ssize_t bytesWritten = pwrite(FD(), writeBuffer, bytesToWrite,
			writeOffset);
		if (bytesWritten < 0) {
			fListener->PrintError("Failed to write data: %s\n",
				strerror(errno));
			return errno;
		}
		if ((size_t)bytesWritten != bytesToWrite) {
			fListener->PrintError("Failed to write all data\n");
			return B_ERROR;
		}

		remainingSize -= toCopy;
		readOffset += toCopy;
		writeOffset += bytesToWrite;
		chunkIndex++;
	}

	// write the offset table
	if (chunkCount > 1) {
		size_t bytesToWrite = (chunkCount - 1) * sizeof(uint64);
		ssize_t bytesWritten = pwrite(FD(), offsetTable, bytesToWrite,
			offsetTableOffset);
		if (bytesWritten < 0) {
			fListener->PrintError("Failed to write data: %s\n",
				strerror(errno));
			return errno;
		}
		if ((size_t)bytesWritten != bytesToWrite) {
			fListener->PrintError("Failed to write all data\n");
			return B_ERROR;
		}
	}

	_compressedSize = writeOffset - offsetTableOffset;
	return B_OK;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit

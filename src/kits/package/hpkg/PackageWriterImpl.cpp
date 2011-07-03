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

#include <package/BlockBufferCacheNoLock.h>

#include <package/hpkg/PackageAttributeValue.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageData.h>
#include <package/hpkg/PackageDataReader.h>

#include <AutoDeleter.h>
#include <RangeArray.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/PackageReaderImpl.h>
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

	void RemoveChild(Attribute* child)
	{
		children.Remove(child);
	}

	void DeleteChildren()
	{
		while (Attribute* child = children.RemoveHead())
			delete child;
	}

	Attribute* FindEntryChild(const char* fileName) const
	{
		for (DoublyLinkedList<Attribute>::ConstIterator it
				= children.GetIterator(); Attribute* child = it.Next();) {
			if (child->id != B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY)
				continue;
			if (child->value.type != B_HPKG_ATTRIBUTE_TYPE_STRING)
				continue;
			const char* childName = child->value.string->string;
			if (strcmp(fileName, childName) == 0)
				return child;
		}

		return NULL;
	}

	Attribute* FindEntryChild(const char* fileName, size_t nameLength) const
	{
		BString name(fileName, nameLength);
		return (size_t)name.Length() == nameLength
			? FindEntryChild(name) : NULL;
	}

	Attribute* FindNodeAttributeChild(const char* attributeName) const
	{
		for (DoublyLinkedList<Attribute>::ConstIterator it
				= children.GetIterator(); Attribute* child = it.Next();) {
			if (child->id != B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE)
				continue;
			if (child->value.type != B_HPKG_ATTRIBUTE_TYPE_STRING)
				continue;
			const char* childName = child->value.string->string;
			if (strcmp(attributeName, childName) == 0)
				return child;
		}

		return NULL;
	}

	Attribute* ChildWithID(BHPKGAttributeID id) const
	{
		for (DoublyLinkedList<Attribute>::ConstIterator it
				= children.GetIterator(); Attribute* child = it.Next();) {
			if (child->id == id)
				return child;
		}

		return NULL;
	}
};


// #pragma mark - PackageContentHandler


struct PackageWriterImpl::PackageContentHandler
	: BLowLevelPackageContentHandler {
	PackageContentHandler(Attribute* rootAttribute, BErrorOutput* errorOutput,
		StringCache& stringCache, uint64 heapOffset)
		:
		fErrorOutput(errorOutput),
		fStringCache(stringCache),
		fRootAttribute(rootAttribute),
		fHeapOffset(heapOffset),
		fErrorOccurred(false)
	{
	}

static const char* AttributeNameForID(uint8 id)
{
	return BLowLevelPackageContentHandler::AttributeNameForID(id);
}

	virtual status_t HandleSectionStart(BHPKGPackageSectionID sectionID,
		bool& _handleSection)
	{
		// we're only interested in the TOC
		_handleSection = sectionID == B_HPKG_SECTION_PACKAGE_TOC;
		return B_OK;
	}

	virtual status_t HandleSectionEnd(BHPKGPackageSectionID sectionID)
	{
		return B_OK;
	}

	virtual status_t HandleAttribute(BHPKGAttributeID attributeID,
		const BPackageAttributeValue& value, void* parentToken, void*& _token)
	{
		if (fErrorOccurred)
			return B_OK;

		Attribute* parentAttribute = parentToken != NULL
			? (Attribute*)parentToken : fRootAttribute;

		Attribute* attribute = new Attribute(attributeID);
		parentAttribute->AddChild(attribute);

		switch (value.type) {
			case B_HPKG_ATTRIBUTE_TYPE_INT:
				attribute->value.SetTo(value.signedInt);
				break;

			case B_HPKG_ATTRIBUTE_TYPE_UINT:
				attribute->value.SetTo(value.unsignedInt);
				break;

			case B_HPKG_ATTRIBUTE_TYPE_STRING:
			{
				CachedString* string = fStringCache.Get(value.string);
				if (string == NULL)
					throw std::bad_alloc();
				attribute->value.SetTo(string);
				break;
			}

			case B_HPKG_ATTRIBUTE_TYPE_RAW:
				if (value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP) {
					attribute->value.SetToData(value.data.size,
						value.data.offset - fHeapOffset);
				} else if (value.encoding
						== B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE) {
					attribute->value.SetToData(value.data.size, value.data.raw);
				} else {
					fErrorOutput->PrintError("Invalid attribute value encoding "
						"%d (attribute %d)\n", value.encoding, attributeID);
					return B_BAD_DATA;
				}
				break;

			case B_HPKG_ATTRIBUTE_TYPE_INVALID:
			default:
				fErrorOutput->PrintError("Invalid attribute value type %d "
					"(attribute %d)\n", value.type, attributeID);
				return B_BAD_DATA;
		}

		_token = attribute;
		return B_OK;
	}

	virtual status_t HandleAttributeDone(BHPKGAttributeID attributeID,
		const BPackageAttributeValue& value, void* parentToken, void* token)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	BErrorOutput*	fErrorOutput;
	StringCache&	fStringCache;
	Attribute*		fRootAttribute;
	uint64			fHeapOffset;
	bool			fErrorOccurred;
};


// #pragma mark - Entry


struct PackageWriterImpl::Entry : DoublyLinkedListLinkImpl<Entry> {
	Entry(char* name, size_t nameLength, int fd, bool isImplicit)
		:
		fName(name),
		fNameLength(nameLength),
		fFD(fd),
		fIsImplicit(isImplicit)
	{
	}

	~Entry()
	{
		DeleteChildren();
		free(fName);
	}

	static Entry* Create(const char* name, size_t nameLength, int fd,
		bool isImplicit)
	{
		char* clonedName = (char*)malloc(nameLength + 1);
		if (clonedName == NULL)
			throw std::bad_alloc();
		memcpy(clonedName, name, nameLength);
		clonedName[nameLength] = '\0';

		Entry* entry = new(std::nothrow) Entry(clonedName, nameLength, fd,
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

	int FD() const
	{
		return fFD;
	}

	void SetFD(int fd)
	{
		fFD = fd;
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
	int			fFD;
	bool		fIsImplicit;
	EntryList	fChildren;
};


// #pragma mark - SubPathAdder


struct PackageWriterImpl::SubPathAdder {
	SubPathAdder(BErrorOutput* errorOutput, char* pathBuffer,
		const char* subPath)
		:
		fOriginalPathEnd(pathBuffer + strlen(pathBuffer))
	{
		if (fOriginalPathEnd != pathBuffer)
			strlcat(pathBuffer, "/", B_PATH_NAME_LENGTH);

		if (strlcat(pathBuffer, subPath, B_PATH_NAME_LENGTH)
				>= B_PATH_NAME_LENGTH) {
			*fOriginalPathEnd = '\0';
			errorOutput->PrintError("Path too long: \"%s/%s\"\n", pathBuffer,
				subPath);
			throw status_t(B_BUFFER_OVERFLOW);
		}
	}

	~SubPathAdder()
	{
		*fOriginalPathEnd = '\0';
	}

private:
	char* fOriginalPathEnd;
};


struct PackageWriterImpl::HeapAttributeOffsetter {
	HeapAttributeOffsetter(const RangeArray<off_t>& ranges,
		const Array<off_t>& deltas)
		:
		fRanges(ranges),
		fDeltas(deltas)
	{
	}

	void ProcessAttribute(Attribute* attribute)
	{
		// If the attribute refers to a heap value, adjust it
		AttributeValue& value = attribute->value;

		if (value.type == B_HPKG_ATTRIBUTE_TYPE_RAW
			&& value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP) {
			off_t delta = fDeltas[fRanges.InsertionIndex(value.data.offset)];
			value.data.offset -= delta;
		}

		// recurse
		for (DoublyLinkedList<Attribute>::Iterator it
					= attribute->children.GetIterator();
				Attribute* child = it.Next();) {
			ProcessAttribute(child);
		}
	}

private:
	const RangeArray<off_t>&	fRanges;
	const Array<off_t>&			fDeltas;
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
	fHeapRangesToRemove(NULL),
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
PackageWriterImpl::Init(const char* fileName, uint32 flags)
{
	try {
		return _Init(fileName, flags);
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
PackageWriterImpl::AddEntry(const char* fileName, int fd)
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

			if (fd >= 0) {
				// a file descriptor is given -- read the config from there
				// stat the file to get the file size
				struct stat st;
				if (fstat(fd, &st) != 0)
					return errno;

				BString packageInfoString;
				char* buffer = packageInfoString.LockBuffer(st.st_size);
				if (buffer == NULL)
					return B_NO_MEMORY;

				ssize_t result = read_pos(fd, 0, buffer, st.st_size);
				if (result < 0) {
					packageInfoString.UnlockBuffer(0);
					return errno;
				}

				buffer[st.st_size] = '\0';
				packageInfoString.UnlockBuffer(st.st_size);

				result = fPackageInfo.ReadFromConfigString(packageInfoString,
					&errorListener);
				if (result != B_OK)
					return result;
			} else {
				// use the file name
				BEntry packageInfoEntry(fileName);
				status_t result = fPackageInfo.ReadFromConfigFile(
					packageInfoEntry, &errorListener);
				if (result != B_OK
					|| (result = fPackageInfo.InitCheck()) != B_OK) {
					return result;
				}
			}
		}

		return _RegisterEntry(fileName, fd);
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
		RangeArray<off_t> heapRangesToRemove;
		fHeapRangesToRemove = &heapRangesToRemove;

		if ((Flags() & B_HPKG_WRITER_UPDATE_PACKAGE) != 0) {
			_UpdateCheckEntryCollisions();

			if (fPackageInfo.InitCheck() != B_OK)
				_UpdateReadPackageInfo();
		}

		if (fPackageInfo.InitCheck() != B_OK) {
			fListener->PrintError("No package-info file found (%s)!\n",
				B_HPKG_PACKAGE_INFO_FILE_NAME);
			return B_BAD_DATA;
		}

		RegisterPackageInfo(PackageAttributes(), fPackageInfo);

		status_t result = _CheckLicenses();
		if (result != B_OK)
			return result;

		if ((Flags() & B_HPKG_WRITER_UPDATE_PACKAGE) != 0)
			_CompactHeap();

		fHeapRangesToRemove = NULL;

		return _Finish();
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
PackageWriterImpl::_Init(const char* fileName, uint32 flags)
{
	status_t result = inherited::Init(fileName, "package", flags);
	if (result != B_OK)
		return result;

	// allocate data buffer
	fDataBuffer = malloc(fDataBufferSize);
	if (fDataBuffer == NULL)
		throw std::bad_alloc();

	if (fStringCache.Init() != B_OK)
		throw std::bad_alloc();

	// create entry list
	fRootEntry = new Entry(NULL, 0, -1, true);

	fRootAttribute = new Attribute();

	fHeapOffset = fHeapEnd = sizeof(hpkg_header);
	fTopAttribute = fRootAttribute;

	// in update mode, parse the TOC
	if ((Flags() & B_HPKG_WRITER_UPDATE_PACKAGE) != 0) {
		PackageReaderImpl packageReader(fListener);
		result = packageReader.Init(FD(), false);
		if (result != B_OK)
			return result;

		fHeapOffset = packageReader.HeapOffset();
		fHeapEnd = fHeapOffset + packageReader.HeapSize();

		PackageContentHandler handler(fRootAttribute, fListener, fStringCache,
			fHeapOffset);

		result = packageReader.ParseContent(&handler);
		if (result != B_OK)
			return result;

		if ((uint64)fHeapOffset > packageReader.HeapOffset()) {
			fListener->PrintError("Unexpected heap offset in package file.\n");
			return B_BAD_DATA;
		}
	}

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

	const BObjectList<BString>& licenseList = fPackageInfo.LicenseList();
	for (int i = 0; i < licenseList.CountItems(); ++i) {
		const BString& licenseName = *licenseList.ItemAt(i);
		if (licenseName == kPublicDomainLicenseName)
			continue;

		BEntry license;
		if (systemLicenseDir.FindEntry(licenseName.String(), &license) == B_OK)
			continue;

		// license is not a system license, so it must be contained in package
		BString licensePath("data/licenses/");
		licensePath << licenseName;

		if (!_IsEntryInPackage(licensePath)) {
			fListener->PrintError("License '%s' isn't contained in package!\n",
				licenseName.String());
			return B_BAD_DATA;
		}
	}

	return B_OK;
}


bool
PackageWriterImpl::_IsEntryInPackage(const char* fileName)
{
	const char* originalFileName = fileName;

	// find the closest ancestor of the entry that is in the added entries
	bool added = false;
	Entry* entry = fRootEntry;
	while (entry != NULL) {
		if (!entry->IsImplicit()) {
			added = true;
			break;
		}

		if (*fileName == '\0')
			break;

		const char* nextSlash = strchr(fileName, '/');

		if (nextSlash == NULL) {
			// no slash, just the file name
			size_t length = strlen(fileName);
			entry  = entry->GetChild(fileName, length);
			fileName += length;
			continue;
		}

		// find the start of the next component, skipping slashes
		const char* nextComponent = nextSlash + 1;
		while (*nextComponent == '/')
			nextComponent++;

		entry = entry->GetChild(fileName, nextSlash - fileName);

		fileName = nextComponent;
	}

	if (added) {
		// the entry itself or one of its ancestors has been added to the
		// package explicitly -- stat it, to see, if it exists
		struct stat st;
		if (entry->FD() >= 0) {
			if (fstatat(entry->FD(), *fileName != '\0' ? fileName : NULL, &st,
					AT_SYMLINK_NOFOLLOW) == 0) {
				return true;
			}
		} else {
			if (lstat(originalFileName, &st) == 0)
				return true;
		}
	}

	// In update mode the entry might already be in the package.
	Attribute* attribute = fRootAttribute;
	fileName = originalFileName;

	while (attribute != NULL) {
		if (*fileName == '\0')
			return true;

		const char* nextSlash = strchr(fileName, '/');

		if (nextSlash == NULL) {
			// no slash, just the file name
			return attribute->FindEntryChild(fileName) != NULL;
		}

		// find the start of the next component, skipping slashes
		const char* nextComponent = nextSlash + 1;
		while (*nextComponent == '/')
			nextComponent++;

		attribute = attribute->FindEntryChild(fileName, nextSlash - fileName);

		fileName = nextComponent;
	}

	return false;
}


void
PackageWriterImpl::_UpdateReadPackageInfo()
{
	// get the .PackageInfo entry attribute
	Attribute* attribute = fRootAttribute->FindEntryChild(
		B_HPKG_PACKAGE_INFO_FILE_NAME);
	if (attribute == NULL) {
		fListener->PrintError("No %s in package file.\n",
			B_HPKG_PACKAGE_INFO_FILE_NAME);
		throw status_t(B_BAD_DATA);
	}

	// get the data attribute
	Attribute* dataAttribute = attribute->ChildWithID(B_HPKG_ATTRIBUTE_ID_DATA);
	if (dataAttribute == NULL)  {
		fListener->PrintError("%s entry in package file doesn't have data.\n",
			B_HPKG_PACKAGE_INFO_FILE_NAME);
		throw status_t(B_BAD_DATA);
	}

	AttributeValue& value = dataAttribute->value;
	if (value.type != B_HPKG_ATTRIBUTE_TYPE_RAW) {
		fListener->PrintError("%s entry in package file has an invalid data "
			"attribute (not of type raw).\n", B_HPKG_PACKAGE_INFO_FILE_NAME);
		throw status_t(B_BAD_DATA);
	}

	BPackageData data;
	if (value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE)
		data.SetData(value.data.size, value.data.raw);
	else
		data.SetData(value.data.size, value.data.offset + fHeapOffset);

	// get the compression
	uint8 compression = B_HPKG_DEFAULT_DATA_COMPRESSION;
	if (Attribute* compressionAttribute = dataAttribute->ChildWithID(
			B_HPKG_ATTRIBUTE_ID_DATA_COMPRESSION)) {
		if (compressionAttribute->value.type != B_HPKG_ATTRIBUTE_TYPE_UINT) {
			fListener->PrintError("%s entry in package file has an invalid "
				"data compression attribute (not of type uint).\n",
				B_HPKG_PACKAGE_INFO_FILE_NAME);
			throw status_t(B_BAD_DATA);
		}
		compression = compressionAttribute->value.unsignedInt;
	}

	data.SetCompression(compression);

	// get the size
	uint64 size;
	Attribute* sizeAttribute = dataAttribute->ChildWithID(
		B_HPKG_ATTRIBUTE_ID_DATA_SIZE);
	if (sizeAttribute == NULL) {
		size = value.data.size;
	} else if (sizeAttribute->value.type != B_HPKG_ATTRIBUTE_TYPE_UINT) {
		fListener->PrintError("%s entry in package file has an invalid data "
			"size attribute (not of type uint).\n",
			B_HPKG_PACKAGE_INFO_FILE_NAME);
		throw status_t(B_BAD_DATA);
	} else
		size = sizeAttribute->value.unsignedInt;

	data.SetUncompressedSize(size);

	// get the chunk size
	uint64 chunkSize = compression == B_HPKG_COMPRESSION_ZLIB
		? B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB : 0;
	if (Attribute* chunkSizeAttribute = dataAttribute->ChildWithID(
			B_HPKG_ATTRIBUTE_ID_DATA_CHUNK_SIZE)) {
		if (chunkSizeAttribute->value.type != B_HPKG_ATTRIBUTE_TYPE_UINT) {
			fListener->PrintError("%s entry in package file has an invalid "
				"data chunk size attribute (not of type uint).\n",
				B_HPKG_PACKAGE_INFO_FILE_NAME);
			throw status_t(B_BAD_DATA);
		}
		chunkSize = chunkSizeAttribute->value.unsignedInt;
	}

	data.SetChunkSize(chunkSize);

	// read the value into a string
	BString valueString;
	char* valueBuffer = valueString.LockBuffer(size);
	if (valueBuffer == NULL)
		throw std::bad_alloc();

	if (value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE) {
		// data encoded inline -- just copy to buffer
		if (size != value.data.size) {
			fListener->PrintError("%s entry in package file has an invalid "
				"data attribute (mismatching size).\n",
				B_HPKG_PACKAGE_INFO_FILE_NAME);
			throw status_t(B_BAD_DATA);
		}
		memcpy(valueBuffer, value.data.raw, value.data.size);
	} else {
		// data on heap -- read from there
		BBlockBufferCacheNoLock	bufferCache(16 * 1024, 1);
		status_t error = bufferCache.Init();
		if (error != B_OK) {
			fListener->PrintError("Failed to initialize buffer cache: %s\n",
				strerror(error));
			throw status_t(error);
		}

		// create a BPackageDataReader
		BFDDataReader packageFileReader(FD());
		BPackageDataReader* reader;
		error = BPackageDataReaderFactory(&bufferCache)
			.CreatePackageDataReader(&packageFileReader, data, reader);
		if (error != B_OK) {
			fListener->PrintError("Failed to create package data reader: %s\n",
				strerror(error));
			throw status_t(error);
		}
		ObjectDeleter<BPackageDataReader> readerDeleter(reader);

		// read the data
		error = reader->ReadData(0, valueBuffer, size);
		if (error != B_OK) {
			fListener->PrintError("Failed to read data of %s entry in package "
				"file: %s\n", B_HPKG_PACKAGE_INFO_FILE_NAME, strerror(error));
			throw status_t(error);
		}
	}

	valueString.UnlockBuffer();

	// parse the package info
	status_t error = fPackageInfo.ReadFromConfigString(valueString);
	if (error != B_OK) {
		fListener->PrintError("Failed to parse package info data from package "
			"file: %s\n", strerror(error));
		throw status_t(error);
	}
}


void
PackageWriterImpl::_UpdateCheckEntryCollisions()
{
	for (EntryList::ConstIterator it = fRootEntry->ChildIterator();
			Entry* entry = it.Next();) {
		char pathBuffer[B_PATH_NAME_LENGTH];
		pathBuffer[0] = '\0';
		_UpdateCheckEntryCollisions(fRootAttribute, AT_FDCWD, entry,
			entry->Name(), pathBuffer);
	}
}


void
PackageWriterImpl::_UpdateCheckEntryCollisions(Attribute* parentAttribute,
	int dirFD, Entry* entry, const char* fileName, char* pathBuffer)
{
	bool isImplicitEntry = entry != NULL && entry->IsImplicit();

	SubPathAdder pathAdder(fListener, pathBuffer, fileName);

	// Check wether there's an entry attribute for this entry. If not, we can
	// ignore this entry.
	Attribute* entryAttribute = parentAttribute->FindEntryChild(fileName);
	if (entryAttribute == NULL)
		return;

	// open the node
	int fd;
	FileDescriptorCloser fdCloser;

	if (entry != NULL && entry->FD() >= 0) {
		// a file descriptor is already given -- use that
		fd = entry->FD();
	} else {
		fd = openat(dirFD, fileName,
			O_RDONLY | (isImplicitEntry ? 0 : O_NOTRAVERSE));
		if (fd < 0) {
			fListener->PrintError("Failed to open entry \"%s\": %s\n",
				pathBuffer, strerror(errno));
			throw status_t(errno);
		}
		fdCloser.SetTo(fd);
	}

	// stat the node
	struct stat st;
	if (fstat(fd, &st) < 0) {
		fListener->PrintError("Failed to fstat() file \"%s\": %s\n", pathBuffer,
			strerror(errno));
		throw status_t(errno);
	}

	// implicit entries must be directories
	if (isImplicitEntry && !S_ISDIR(st.st_mode)) {
		fListener->PrintError("Non-leaf path component \"%s\" is not a "
			"directory.\n", pathBuffer);
		throw status_t(B_BAD_VALUE);
	}

	// get the pre-existing node's file type
	uint32 preExistingFileType = B_HPKG_DEFAULT_FILE_TYPE;
	if (Attribute* fileTypeAttribute
			= entryAttribute->ChildWithID(B_HPKG_ATTRIBUTE_ID_FILE_TYPE)) {
		if (fileTypeAttribute->value.type == B_HPKG_ATTRIBUTE_TYPE_UINT)
			preExistingFileType = fileTypeAttribute->value.unsignedInt;
	}

	// Compare the node type with that of the pre-existing one.
	if (!S_ISDIR(st.st_mode)) {
		// the pre-existing must not a directory either -- we'll remove it
		if (preExistingFileType == B_HPKG_FILE_TYPE_DIRECTORY) {
			fListener->PrintError("Specified file \"%s\" clashes with an "
				"archived directory.\n", pathBuffer);
			throw status_t(B_BAD_VALUE);
		}

		if ((Flags() & B_HPKG_WRITER_FORCE_ADD) == 0) {
			fListener->PrintError("Specified file \"%s\" clashes with an "
				"archived file.\n", pathBuffer);
			throw status_t(B_FILE_EXISTS);
		}

		parentAttribute->RemoveChild(entryAttribute);
		_AttributeRemoved(entryAttribute);

		return;
	}

	// the pre-existing entry needs to be a directory too -- we will merge
	if (preExistingFileType != B_HPKG_FILE_TYPE_DIRECTORY) {
		fListener->PrintError("Specified directory \"%s\" clashes with an "
			"archived non-directory.\n", pathBuffer);
		throw status_t(B_BAD_VALUE);
	}

	// directory -- recursively add children
	if (isImplicitEntry) {
		// this is an implicit entry -- just check the child entries
		for (EntryList::ConstIterator it = entry->ChildIterator();
				Entry* child = it.Next();) {
			_UpdateCheckEntryCollisions(entryAttribute, fd, child,
				child->Name(), pathBuffer);
		}
	} else {
		// explicitly specified directory -- we need to read the directory

		// first we check for colliding node attributes, though
		if (DIR* attrDir = fs_fopen_attr_dir(fd)) {
			CObjectDeleter<DIR, int> attrDirCloser(attrDir, fs_close_attr_dir);

			while (dirent* entry = fs_read_attr_dir(attrDir)) {
				attr_info attrInfo;
				if (fs_stat_attr(fd, entry->d_name, &attrInfo) < 0) {
					fListener->PrintError(
						"Failed to stat attribute \"%s\" of directory \"%s\": "
						"%s\n", entry->d_name, pathBuffer, strerror(errno));
					throw status_t(errno);
				}

				// check whether the attribute exists
				Attribute* attributeAttribute
					= entryAttribute->FindNodeAttributeChild(entry->d_name);
				if (attributeAttribute == NULL)
					continue;

				if ((Flags() & B_HPKG_WRITER_FORCE_ADD) == 0) {
					fListener->PrintError("Attribute \"%s\" of specified "
						"directory \"%s\" clashes with an archived "
						"attribute.\n", pathBuffer);
					throw status_t(B_FILE_EXISTS);
				}

				// remove it
				entryAttribute->RemoveChild(attributeAttribute);
				_AttributeRemoved(attributeAttribute);
			}
		}

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
				"Failed to open directory \"%s\": %s\n", pathBuffer,
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
			_UpdateCheckEntryCollisions(entryAttribute, fd, NULL, entry->d_name,
				pathBuffer);
		}
	}
}


void
PackageWriterImpl::_CompactHeap()
{
	int32 count = fHeapRangesToRemove->CountRanges();
	if (count == 0)
		return;

	// compute the move deltas for the ranges
	Array<off_t> deltas;
	off_t delta = 0;
	for (int32 i = 0; i < count; i++) {
		if (!deltas.Add(delta))
			throw std::bad_alloc();

		delta += fHeapRangesToRemove->RangeAt(i).size;
	}

	if (!deltas.Add(delta))
		throw std::bad_alloc();

	// offset the attributes
	HeapAttributeOffsetter(*fHeapRangesToRemove, deltas).ProcessAttribute(
		fRootAttribute);

	// move the heap chunks in the file around
	off_t chunkOffset = fHeapOffset;
	delta = 0;

	for (int32 i = 0; i < count; i++) {
		const Range<off_t>& range = fHeapRangesToRemove->RangeAt(i);

		if (delta > 0 && chunkOffset < range.offset) {
			// move chunk
			_MoveHeapChunk(chunkOffset, chunkOffset - delta,
				range.offset - chunkOffset);
		}

		chunkOffset = range.EndOffset();
		delta += range.size;
	}

	// move the final chunk
	if (delta > 0 && chunkOffset < fHeapEnd) {
		_MoveHeapChunk(chunkOffset, chunkOffset - delta,
			fHeapEnd - chunkOffset);
	}

	fHeapEnd -= delta;
}


void
PackageWriterImpl::_MoveHeapChunk(off_t fromOffset, off_t toOffset, off_t size)
{
	// convert heap offsets to file offsets
	fromOffset += fHeapOffset;
	toOffset += fHeapOffset;

	while (size > 0) {
		size_t toCopy = std::min(size, (off_t)fDataBufferSize);

		// read data into buffer
		ssize_t bytesRead = read_pos(FD(), fromOffset, fDataBuffer, toCopy);
		if (bytesRead < 0) {
			fListener->PrintError("Failed to read from package file: %s\n",
				strerror(errno));
			throw status_t(errno);
		}
		if ((size_t)bytesRead < toCopy) {
			fListener->PrintError("Failed to read from package file (wanted "
				"%zu bytes, got %zd).\n", toCopy, bytesRead);
			throw status_t(B_IO_ERROR);
		}

		// write data to target offset
		ssize_t bytesWritten = write_pos(FD(), toOffset, fDataBuffer, toCopy);
		if (bytesWritten < 0) {
			fListener->PrintError("Failed to write to package file: %s\n",
				strerror(errno));
			throw status_t(errno);
		}
		if ((size_t)bytesWritten < toCopy) {
			fListener->PrintError("Failed to write to package file.\n");
			throw status_t(B_IO_ERROR);
		}

		fromOffset += toCopy;
		toOffset += toCopy;
		size -= toCopy;
	}
}


void
PackageWriterImpl::_AttributeRemoved(Attribute* attribute)
{
	AttributeValue& value = attribute->value;
	if (value.type == B_HPKG_ATTRIBUTE_TYPE_RAW
		&& value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP) {
		if (!fHeapRangesToRemove->AddRange(value.data.offset, value.data.size))
			throw std::bad_alloc();
	}

	for (DoublyLinkedList<Attribute>::Iterator it
				= attribute->children.GetIterator();
			Attribute* child = it.Next();) {
		_AttributeRemoved(child);
	}
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

	off_t heapSize = fHeapEnd - fHeapOffset;

	hpkg_header header;

	// write the TOC and package attributes
	_WriteTOC(header);
	_WritePackageAttributes(header);

	off_t totalSize = fHeapEnd;

	// Truncate the file to the size it is supposed to have. In update mode, it
	// can be greater when one or more files are shrunk. In creation mode, when
	// writing compressed TOC or package attributes yields a larger size than
	// uncompressed, the file size may also be greater than it should be.
	if (ftruncate(FD(), totalSize) != 0) {
		fListener->PrintError("Failed to truncate package file to new "
			"size: %s\n", strerror(errno));
		return errno;
	}

	fListener->OnPackageSizeInfo(fHeapOffset, heapSize,
		B_BENDIAN_TO_HOST_INT64(header.toc_length_compressed),
		B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed),
		totalSize);

	// prepare the header

	// general
	header.magic = B_HOST_TO_BENDIAN_INT32(B_HPKG_MAGIC);
	header.header_size = B_HOST_TO_BENDIAN_INT16((uint16)fHeapOffset);
	header.version = B_HOST_TO_BENDIAN_INT16(B_HPKG_VERSION);
	header.total_size = B_HOST_TO_BENDIAN_INT64(totalSize);

	// write the header
	WriteBuffer(&header, sizeof(hpkg_header), 0);

	SetFinished(true);
	return B_OK;
}


status_t
PackageWriterImpl::_RegisterEntry(const char* fileName, int fd)
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
			entry = _RegisterEntry(entry, fileName, strlen(fileName), fd,
				false);
			break;
		}

		// find the start of the next component, skipping slashes
		const char* nextComponent = nextSlash + 1;
		while (*nextComponent == '/')
			nextComponent++;

		bool lastComponent = *nextComponent != '\0';

		if (nextSlash == fileName) {
			// the FS root
			entry = _RegisterEntry(entry, fileName, 1, lastComponent ? fd : -1,
				lastComponent);
		} else {
			entry = _RegisterEntry(entry, fileName, nextSlash - fileName,
				lastComponent ? fd : -1, lastComponent);
		}

		fileName = nextComponent;
	}

	return B_OK;
}


PackageWriterImpl::Entry*
PackageWriterImpl::_RegisterEntry(Entry* parent, const char* name,
	size_t nameLength, int fd, bool isImplicit)
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
			entry->SetFD(fd);
		}
	} else {
		// nope -- create it
		entry = Entry::Create(name, nameLength, fd, isImplicit);
		parent->AddChild(entry);
	}

	return entry;
}


void
PackageWriterImpl::_WriteTOC(hpkg_header& header)
{
	// prepare the writer (zlib writer on top of a file writer)
	off_t startOffset = fHeapEnd;

	// write the sections
	uint32 compression = B_HPKG_COMPRESSION_ZLIB;
	uint64 uncompressedStringsSize;
	uint64 uncompressedMainSize;
	uint64 tocUncompressedSize;
	int32 cachedStringsWritten = _WriteTOCCompressed(uncompressedStringsSize,
		uncompressedMainSize, tocUncompressedSize);

	off_t endOffset = fHeapEnd;

	if (endOffset - startOffset >= (off_t)tocUncompressedSize) {
		// the compressed section isn't shorter -- write uncompressed
		fHeapEnd = startOffset;
		compression = B_HPKG_COMPRESSION_NONE;
		cachedStringsWritten = _WriteTOCUncompressed(uncompressedStringsSize,
			uncompressedMainSize, tocUncompressedSize);

		endOffset = fHeapEnd;
	}

	fListener->OnTOCSizeInfo(uncompressedStringsSize, uncompressedMainSize,
		tocUncompressedSize);

	// update the header

	// TOC
	header.toc_compression = B_HOST_TO_BENDIAN_INT32(compression);
	header.toc_length_compressed = B_HOST_TO_BENDIAN_INT64(
		endOffset - startOffset);
	header.toc_length_uncompressed = B_HOST_TO_BENDIAN_INT64(
		tocUncompressedSize);

	// TOC subsections
	header.toc_strings_length = B_HOST_TO_BENDIAN_INT64(
		uncompressedStringsSize);
	header.toc_strings_count = B_HOST_TO_BENDIAN_INT64(cachedStringsWritten);
}


int32
PackageWriterImpl::_WriteTOCCompressed(uint64& _uncompressedStringsSize,
	uint64& _uncompressedMainSize, uint64& _tocUncompressedSize)
{
	FDDataWriter realWriter(FD(), fHeapEnd, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	// write the sections
	int32 cachedStringsWritten
		= _WriteTOCSections(_uncompressedStringsSize, _uncompressedMainSize);

	// finish the writer
	zlibWriter.Finish();
	fHeapEnd = realWriter.Offset();
	SetDataWriter(NULL);

	_tocUncompressedSize = zlibWriter.BytesWritten();
	return cachedStringsWritten;
}


int32
PackageWriterImpl::_WriteTOCUncompressed(uint64& _uncompressedStringsSize,
	uint64& _uncompressedMainSize, uint64& _tocUncompressedSize)
{
	FDDataWriter realWriter(FD(), fHeapEnd, fListener);
	SetDataWriter(&realWriter);

	// write the sections
	int32 cachedStringsWritten
		= _WriteTOCSections(_uncompressedStringsSize, _uncompressedMainSize);

	fHeapEnd = realWriter.Offset();
	SetDataWriter(NULL);

	_tocUncompressedSize = realWriter.BytesWritten();
	return cachedStringsWritten;
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

	uint32 compression = B_HPKG_COMPRESSION_ZLIB;
	uint32 stringsLengthUncompressed;
	uint32 attributesLengthUncompressed;
	uint32 stringsCount = _WritePackageAttributesCompressed(
		stringsLengthUncompressed, attributesLengthUncompressed);

	off_t endOffset = fHeapEnd;

	if ((off_t)attributesLengthUncompressed <= endOffset - startOffset) {
		// the compressed section isn't shorter -- write uncompressed
		fHeapEnd = startOffset;
		compression = B_HPKG_COMPRESSION_NONE;
		stringsCount = _WritePackageAttributesUncompressed(
			stringsLengthUncompressed, attributesLengthUncompressed);

		endOffset = fHeapEnd;
	}

	fListener->OnPackageAttributesSizeInfo(stringsCount,
		attributesLengthUncompressed);

	// update the header
	header.attributes_compression = B_HOST_TO_BENDIAN_INT32(compression);
	header.attributes_length_compressed
		= B_HOST_TO_BENDIAN_INT32(endOffset - startOffset);
	header.attributes_length_uncompressed
		= B_HOST_TO_BENDIAN_INT32(attributesLengthUncompressed);
	header.attributes_strings_count = B_HOST_TO_BENDIAN_INT32(stringsCount);
	header.attributes_strings_length
		= B_HOST_TO_BENDIAN_INT32(stringsLengthUncompressed);
}


uint32
PackageWriterImpl::_WritePackageAttributesCompressed(
	uint32& _stringsLengthUncompressed, uint32& _attributesLengthUncompressed)
{
	off_t startOffset = fHeapEnd;
	FDDataWriter realWriter(FD(), startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	// write cached strings and package attributes tree
	uint32 stringsCount = WritePackageAttributes(PackageAttributes(),
		_stringsLengthUncompressed);

	zlibWriter.Finish();
	fHeapEnd = realWriter.Offset();
	SetDataWriter(NULL);

	_attributesLengthUncompressed = zlibWriter.BytesWritten();
	return stringsCount;
}


uint32
PackageWriterImpl::_WritePackageAttributesUncompressed(
	uint32& _stringsLengthUncompressed, uint32& _attributesLengthUncompressed)
{
	off_t startOffset = fHeapEnd;
	FDDataWriter realWriter(FD(), startOffset, fListener);

	SetDataWriter(&realWriter);

	// write cached strings and package attributes tree
	uint32 stringsCount = WritePackageAttributes(PackageAttributes(),
		_stringsLengthUncompressed);

	fHeapEnd = realWriter.Offset();
	SetDataWriter(NULL);

	_attributesLengthUncompressed = realWriter.BytesWritten();
	return stringsCount;
}


void
PackageWriterImpl::_AddEntry(int dirFD, Entry* entry, const char* fileName,
	char* pathBuffer)
{
	bool isImplicitEntry = entry != NULL && entry->IsImplicit();

	SubPathAdder pathAdder(fListener, pathBuffer, fileName);
	if (!isImplicitEntry)
		fListener->OnEntryAdded(pathBuffer);

	// open the node
	int fd;
	FileDescriptorCloser fdCloser;

	if (entry != NULL && entry->FD() >= 0) {
		// a file descriptor is already given -- use that
		fd = entry->FD();
	} else {
		fd = openat(dirFD, fileName,
			O_RDONLY | (isImplicitEntry ? 0 : O_NOTRAVERSE));
		if (fd < 0) {
			fListener->PrintError("Failed to open entry \"%s\": %s\n",
				pathBuffer, strerror(errno));
			throw status_t(errno);
		}
		fdCloser.SetTo(fd);
	}

	// stat the node
	struct stat st;
	if (fstat(fd, &st) < 0) {
		fListener->PrintError("Failed to fstat() file \"%s\": %s\n", pathBuffer,
			strerror(errno));
		throw status_t(errno);
	}

	// implicit entries must be directories
	if (isImplicitEntry && !S_ISDIR(st.st_mode)) {
		fListener->PrintError("Non-leaf path component \"%s\" is not a "
			"directory\n", pathBuffer);
		throw status_t(B_BAD_VALUE);
	}

	// In update mode we don't need to add an entry attribute for an implicit
	// directory, if there already is one.
	if (isImplicitEntry && (Flags() & B_HPKG_WRITER_UPDATE_PACKAGE) != 0) {
		Attribute* entryAttribute = fTopAttribute->FindEntryChild(fileName);
		if (entryAttribute != NULL) {
			Stacker<Attribute> entryAttributeStacker(fTopAttribute,
				entryAttribute);
			_AddDirectoryChildren(entry, fd, pathBuffer);
			return;
		}
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
			pathBuffer);
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
				pathBuffer, strerror(errno));
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
					entry->d_name, pathBuffer, strerror(errno));
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

	if (S_ISDIR(st.st_mode))
		_AddDirectoryChildren(entry, fd, pathBuffer);
}


void
PackageWriterImpl::_AddDirectoryChildren(Entry* entry, int fd, char* pathBuffer)
{
	// directory -- recursively add children
	if (entry != NULL && entry->IsImplicit()) {
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
				"Failed to open directory \"%s\": %s\n", pathBuffer,
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

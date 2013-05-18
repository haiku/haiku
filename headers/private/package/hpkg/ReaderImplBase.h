/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_


#include <errno.h>
#include <sys/stat.h>

#include <ByteOrder.h>
#include <SupportDefs.h>

#include <util/SinglyLinkedList.h>

#include <package/hpkg/PackageAttributeValue.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {

namespace BHPKG {


class BErrorOutput;


namespace BPrivate {


class PackageFileHeapReader;


struct PackageFileSection {
	uint32			uncompressedLength;
	uint8*			data;
	uint64			offset;
	uint64			currentOffset;
	uint64			stringsLength;
	uint64			stringsCount;
	char**			strings;
	const char*		name;

	PackageFileSection(const char* _name)
		:
		data(NULL),
		strings(NULL),
		name(_name)
	{
	}

	~PackageFileSection()
	{
		delete[] strings;
		delete[] data;
	}
};


class ReaderImplBase {
protected:
								ReaderImplBase(const char* fileType,
									BErrorOutput* errorOutput);
	virtual						~ReaderImplBase();

			int					FD() const;

			BErrorOutput*		ErrorOutput() const;

			PackageFileHeapReader* HeapReader() const
									{ return fHeapReader; }

protected:
			class AttributeHandlerContext;
			class AttributeHandler;
			class IgnoreAttributeHandler;
			class PackageVersionAttributeHandler;
			class PackageResolvableAttributeHandler;
			class PackageResolvableExpressionAttributeHandler;
			class PackageAttributeHandler;
			class LowLevelAttributeHandler;

			typedef BPackageAttributeValue AttributeValue;
			typedef SinglyLinkedList<AttributeHandler> AttributeHandlerList;

protected:
			template<typename Header, uint32 kMagic, uint16 kVersion>
			status_t			Init(int fd, bool keepFD, Header& header);
			status_t			InitHeapReader(uint32 compression,
									uint32 chunkSize, off_t offset,
									uint64 compressedSize,
									uint64 uncompressedSize);
			status_t			InitSection(PackageFileSection& section,
									uint64 endOffset, uint64 length,
									uint64 maxSaneLength, uint64 stringsLength,
									uint64 stringsCount);
			status_t			PrepareSection(PackageFileSection& section);

			status_t			ParseStrings();

			status_t			ParsePackageAttributesSection(
									AttributeHandlerContext* context,
									AttributeHandler* rootAttributeHandler);
			status_t			ParseAttributeTree(
									AttributeHandlerContext* context,
									bool& _sectionHandled);

	virtual	status_t			ReadAttributeValue(uint8 type, uint8 encoding,
									AttributeValue& _value);

			status_t			ReadUnsignedLEB128(uint64& _value);

			status_t			ReadBuffer(off_t offset, void* buffer,
									size_t size);
			status_t			ReadSection(const PackageFileSection& section);

	inline	AttributeHandler*	CurrentAttributeHandler() const;
	inline	void				PushAttributeHandler(
									AttributeHandler* handler);
	inline	AttributeHandler*	PopAttributeHandler();
	inline	void				ClearAttributeHandlerStack();

	inline	PackageFileSection*	CurrentSection();
	inline	void				SetCurrentSection(PackageFileSection* section);

protected:
			PackageFileSection	fPackageAttributesSection;

private:
			status_t			_Init(int fd, bool keepFD);

			status_t			_ParseAttributeTree(
									AttributeHandlerContext* context);

	template<typename Type>
	inline	status_t			_Read(Type& _value);

			status_t			_ReadSectionBuffer(void* buffer, size_t size);

			status_t			_ReadAttribute(uint8& _id,
									AttributeValue& _value,
									bool* _hasChildren = NULL,
									uint64* _tag = NULL);

			status_t			_ReadString(const char*& _string,
									size_t* _stringLength = NULL);

private:
			const char*			fFileType;
			BErrorOutput*		fErrorOutput;
			int					fFD;
			bool				fOwnsFD;

			PackageFileHeapReader* fHeapReader;

			PackageFileSection*	fCurrentSection;

			AttributeHandlerList fAttributeHandlerStack;

			uint8*				fScratchBuffer;
			size_t				fScratchBufferSize;
};


// #pragma mark - attribute handlers


class ReaderImplBase::AttributeHandlerContext {
public:
			BErrorOutput*			errorOutput;
			union {
				BPackageContentHandler*			packageContentHandler;
				BLowLevelPackageContentHandler*	lowLevelHandler;
			};
			bool					hasLowLevelHandler;
		
			BHPKGPackageSectionID	section;

public:
								AttributeHandlerContext(
									BErrorOutput* errorOutput,
									BPackageContentHandler*
										packageContentHandler,
									BHPKGPackageSectionID section);
								AttributeHandlerContext(
									BErrorOutput* errorOutput,
									BLowLevelPackageContentHandler*
										lowLevelHandler,
									BHPKGPackageSectionID section);

			void				ErrorOccurred();
};


class ReaderImplBase::AttributeHandler
	: public SinglyLinkedListLinkImpl<AttributeHandler> {
public:
	virtual						~AttributeHandler();

			void				SetLevel(int level);
	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

	virtual	status_t			Delete(AttributeHandlerContext* context);

protected:
			int					fLevel;
};


class ReaderImplBase::IgnoreAttributeHandler : public AttributeHandler {
};


class ReaderImplBase::PackageVersionAttributeHandler : public AttributeHandler {
public:
								PackageVersionAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue,
									BPackageVersionData& versionData,
									bool notify);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

	virtual	status_t			Delete(AttributeHandlerContext* context);

private:
			BPackageInfoAttributeValue& fPackageInfoValue;
			BPackageVersionData& fPackageVersionData;
			bool				fNotify;
};


class ReaderImplBase::PackageResolvableAttributeHandler
	: public AttributeHandler {
public:
								PackageResolvableAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

	virtual	status_t			Delete(AttributeHandlerContext* context);

private:
			BPackageInfoAttributeValue& fPackageInfoValue;
};


class ReaderImplBase::PackageResolvableExpressionAttributeHandler
	: public AttributeHandler {
public:
								PackageResolvableExpressionAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

	virtual	status_t			Delete(AttributeHandlerContext* context);

private:
			BPackageInfoAttributeValue& fPackageInfoValue;
};


class ReaderImplBase::PackageAttributeHandler : public AttributeHandler {
public:
	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

private:
			BPackageInfoAttributeValue fPackageInfoValue;
};


class ReaderImplBase::LowLevelAttributeHandler : public AttributeHandler {
public:
								LowLevelAttributeHandler();
								LowLevelAttributeHandler(uint8 id,
									const BPackageAttributeValue& value,
									void* parentToken, void* token);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);
	virtual	status_t			Delete(AttributeHandlerContext* context);

private:
			void*				fParentToken;
			void*				fToken;
			uint8				fID;
			AttributeValue		fValue;
};


// #pragma mark - template and inline methods


template<typename Header, uint32 kMagic, uint16 kVersion>
status_t
ReaderImplBase::Init(int fd, bool keepFD, Header& header)
{
	status_t error = _Init(fd, keepFD);
	if (error != B_OK)
		return error;

	// stat the file
	struct stat st;
	if (fstat(FD(), &st) < 0) {
		ErrorOutput()->PrintError("Error: Failed to access %s file: %s\n",
			fFileType, strerror(errno));
		return errno;
	}

	// read the header
	if ((error = ReadBuffer(0, &header, sizeof(header))) != B_OK)
		return error;

	// check the header

	// magic
	if (B_BENDIAN_TO_HOST_INT32(header.magic) != kMagic) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Invalid "
			"magic\n", fFileType);
		return B_BAD_DATA;
	}

	// version
	if (B_BENDIAN_TO_HOST_INT16(header.version) != kVersion) {
		ErrorOutput()->PrintError("Error: Invalid/unsupported %s file "
			"version (%d)\n", fFileType,
			B_BENDIAN_TO_HOST_INT16(header.version));
		return B_MISMATCHED_VALUES;
	}

	// header size
	uint64 heapOffset = B_BENDIAN_TO_HOST_INT16(header.header_size);
	if (heapOffset < (off_t)sizeof(header)) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Invalid header "
			"size (%" B_PRIu64 ")\n", fFileType, heapOffset);
		return B_BAD_DATA;
	}

	// total size
	uint64 totalSize = B_BENDIAN_TO_HOST_INT64(header.total_size);
	if (totalSize != (uint64)st.st_size) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Total size in "
			"header (%" B_PRIu64 ") doesn't agree with total file size (%"
			B_PRIdOFF ")\n", fFileType, totalSize, st.st_size);
		return B_BAD_DATA;
	}

	// heap size
	uint64 compressedHeapSize
		= B_BENDIAN_TO_HOST_INT64(header.heap_size_compressed);
	if (compressedHeapSize > totalSize
		|| heapOffset > totalSize - compressedHeapSize) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Heap size in "
			"header (%" B_PRIu64 ") doesn't agree with total file size (%"
			B_PRIu64 ") and heap offset (%" B_PRIu64 ")\n", fFileType,
			compressedHeapSize, totalSize, heapOffset);
		return B_BAD_DATA;
	}

	error = InitHeapReader(
		B_BENDIAN_TO_HOST_INT32(header.heap_compression),
		B_BENDIAN_TO_HOST_INT32(header.heap_chunk_size), heapOffset,
		compressedHeapSize,
		B_BENDIAN_TO_HOST_INT64(header.heap_size_uncompressed));
	if (error != B_OK)
		return error;

	return B_OK;
}


inline int
ReaderImplBase::FD() const
{
	return fFD;
}


inline BErrorOutput*
ReaderImplBase::ErrorOutput() const
{
	return fErrorOutput;
}


PackageFileSection*
ReaderImplBase::CurrentSection()
{
	return fCurrentSection;
}


void
ReaderImplBase::SetCurrentSection(PackageFileSection* section)
{
	fCurrentSection = section;
}


template<typename Type>
status_t
ReaderImplBase::_Read(Type& _value)
{
	return _ReadSectionBuffer(&_value, sizeof(Type));
}


inline ReaderImplBase::AttributeHandler*
ReaderImplBase::CurrentAttributeHandler() const
{
	return fAttributeHandlerStack.Head();
}


inline void
ReaderImplBase::PushAttributeHandler(AttributeHandler* handler)
{
	fAttributeHandlerStack.Add(handler);
}


inline ReaderImplBase::AttributeHandler*
ReaderImplBase::PopAttributeHandler()
{
	return fAttributeHandlerStack.RemoveHead();
}


inline void
ReaderImplBase::ClearAttributeHandlerStack()
{
	fAttributeHandlerStack.MakeEmpty();
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_

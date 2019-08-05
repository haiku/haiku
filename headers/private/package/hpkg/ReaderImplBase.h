/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__READER_IMPL_BASE_H_


#include <errno.h>
#include <sys/stat.h>

#include <ByteOrder.h>
#include <DataIO.h>

#include <Array.h>
#include <util/SinglyLinkedList.h>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageAttributeValue.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageInfoAttributeValue.h>


class BCompressionAlgorithm;
class BDecompressionParameters;


namespace BPackageKit {

namespace BHPKG {


class BAbstractBufferedDataReader;
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
public:
	inline	const PackageFileSection& PackageAttributesSection() const
									{ return fPackageAttributesSection; }

protected:
								ReaderImplBase(const char* fileType,
									BErrorOutput* errorOutput);
	virtual						~ReaderImplBase();

			BPositionIO*		File() const;

			BErrorOutput*		ErrorOutput() const;

			uint16				MinorFormatVersion() const
									{ return fMinorFormatVersion; }

			uint64				UncompressedHeapSize() const;

			PackageFileHeapReader* RawHeapReader() const
									{ return fRawHeapReader; }
			BAbstractBufferedDataReader* HeapReader() const
									{ return fHeapReader; }
									// equals RawHeapReader(), if uncached

			BAbstractBufferedDataReader* DetachHeapReader(
									PackageFileHeapReader*& _rawHeapReader);
									// Detaches both raw and (if applicable)
									// cached heap reader. The called gains
									// ownership of both. The file may need to
									// be set on the raw heap reader, if it
									// shall be used after destroying this
									// object and Init() has been called with
									// keepFile == true.

protected:
			class AttributeHandlerContext;
			class AttributeHandler;
			class IgnoreAttributeHandler;
			class PackageInfoAttributeHandlerBase;
			class PackageVersionAttributeHandler;
			class PackageResolvableAttributeHandler;
			class PackageResolvableExpressionAttributeHandler;
			class GlobalWritableFileInfoAttributeHandler;
			class UserSettingsFileInfoAttributeHandler;
			class UserAttributeHandler;
			class PackageAttributeHandler;
			class LowLevelAttributeHandler;

			typedef BPackageAttributeValue AttributeValue;
			typedef SinglyLinkedList<AttributeHandler> AttributeHandlerList;

protected:
			template<typename Header, uint32 kMagic, uint16 kVersion,
				uint16 kMinorVersion>
			status_t			Init(BPositionIO* file, bool keepFile,
									Header& header, uint32 flags);
			status_t			InitHeapReader(uint32 compression,
									uint32 chunkSize, off_t offset,
									uint64 compressedSize,
									uint64 uncompressedSize);
	virtual	status_t			CreateCachedHeapReader(
									PackageFileHeapReader* heapReader,
									BAbstractBufferedDataReader*&
										_cachedReader);
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
			status_t			_Init(BPositionIO* file, bool keepFile);

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
			BPositionIO*		fFile;
			bool				fOwnsFile;
			uint16				fMinorFormatVersion;
			uint16				fCurrentMinorFormatVersion;

			PackageFileHeapReader* fRawHeapReader;
			BAbstractBufferedDataReader* fHeapReader;

			PackageFileSection*	fCurrentSection;

			AttributeHandlerList fAttributeHandlerStack;
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
			bool					ignoreUnknownAttributes;

			BHPKGPackageSectionID	section;

public:
								AttributeHandlerContext(
									BErrorOutput* errorOutput,
									BPackageContentHandler*
										packageContentHandler,
									BHPKGPackageSectionID section,
									bool ignoreUnknownAttributes);
								AttributeHandlerContext(
									BErrorOutput* errorOutput,
									BLowLevelPackageContentHandler*
										lowLevelHandler,
									BHPKGPackageSectionID section,
									bool ignoreUnknownAttributes);

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

	virtual	status_t			NotifyDone(AttributeHandlerContext* context);

	virtual	status_t			Delete(AttributeHandlerContext* context);

protected:
			int					fLevel;
};


class ReaderImplBase::IgnoreAttributeHandler : public AttributeHandler {
};


class ReaderImplBase::PackageInfoAttributeHandlerBase
	: public AttributeHandler {
private:
	typedef	AttributeHandler	super;
public:
								PackageInfoAttributeHandlerBase(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			NotifyDone(AttributeHandlerContext* context);

protected:
			BPackageInfoAttributeValue& fPackageInfoValue;
};


class ReaderImplBase::PackageVersionAttributeHandler
	: public PackageInfoAttributeHandlerBase {
private:
	typedef	PackageInfoAttributeHandlerBase	super;
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

	virtual	status_t			NotifyDone(AttributeHandlerContext* context);

private:
			BPackageVersionData& fPackageVersionData;
			bool				fNotify;
};


class ReaderImplBase::PackageResolvableAttributeHandler
	: public PackageInfoAttributeHandlerBase {
private:
	typedef	PackageInfoAttributeHandlerBase	super;
public:
								PackageResolvableAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);
};


class ReaderImplBase::PackageResolvableExpressionAttributeHandler
	: public PackageInfoAttributeHandlerBase {
private:
	typedef	PackageInfoAttributeHandlerBase	super;
public:
								PackageResolvableExpressionAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);
};


class ReaderImplBase::GlobalWritableFileInfoAttributeHandler
	: public PackageInfoAttributeHandlerBase {
private:
	typedef	PackageInfoAttributeHandlerBase	super;
public:
								GlobalWritableFileInfoAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);
};


class ReaderImplBase::UserSettingsFileInfoAttributeHandler
	: public PackageInfoAttributeHandlerBase {
private:
	typedef	PackageInfoAttributeHandlerBase	super;
public:
								UserSettingsFileInfoAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);
};


class ReaderImplBase::UserAttributeHandler
	: public PackageInfoAttributeHandlerBase {
private:
	typedef	PackageInfoAttributeHandlerBase	super;
public:
								UserAttributeHandler(
									BPackageInfoAttributeValue&
										packageInfoValue);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

	virtual	status_t			NotifyDone(AttributeHandlerContext* context);

private:
			Array<const char*>	fGroups;
};


class ReaderImplBase::PackageAttributeHandler : public AttributeHandler {
private:
	typedef	AttributeHandler	super;
public:
	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

private:
			BPackageInfoAttributeValue fPackageInfoValue;
};


class ReaderImplBase::LowLevelAttributeHandler : public AttributeHandler {
private:
	typedef	AttributeHandler	super;
public:
								LowLevelAttributeHandler();
								LowLevelAttributeHandler(uint8 id,
									const BPackageAttributeValue& value,
									void* parentToken, void* token);

	virtual	status_t			HandleAttribute(
									AttributeHandlerContext* context, uint8 id,
									const AttributeValue& value,
									AttributeHandler** _handler);

	virtual	status_t			NotifyDone(AttributeHandlerContext* context);

private:
			void*				fParentToken;
			void*				fToken;
			uint8				fID;
			AttributeValue		fValue;
};


// #pragma mark - template and inline methods


template<typename Header, uint32 kMagic, uint16 kVersion, uint16 kMinorVersion>
status_t
ReaderImplBase::Init(BPositionIO* file, bool keepFile, Header& header, uint32 flags)
{
	status_t error = _Init(file, keepFile);
	if (error != B_OK)
		return error;

	// get the file size
	off_t fileSize;
	error = fFile->GetSize(&fileSize);
	if (error != B_OK) {
		if (error != B_NOT_SUPPORTED) {
			ErrorOutput()->PrintError(
				"Error: Failed to get size of %s file: %s\n",
				fFileType, strerror(error));
			return error;
		}

		// Might be a stream. We only use the file size for checking the total
		// heap size, which we don't have to do.
		fileSize = -1;
	}

	// validate file is longer than header (when not a stream)
	if (fileSize >= 0 && fileSize < (off_t)sizeof(header)) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Length shorter than "
			"header!\n", fFileType);
		return B_BAD_DATA;
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
		if ((flags & B_HPKG_READER_DONT_PRINT_VERSION_MISMATCH_MESSAGE) == 0) {
			ErrorOutput()->PrintError("Error: Invalid/unsupported %s file "
				"version (%d)\n", fFileType,
				B_BENDIAN_TO_HOST_INT16(header.version));
		}
		return B_MISMATCHED_VALUES;
	}

	fMinorFormatVersion = B_BENDIAN_TO_HOST_INT16(header.minor_version);
	fCurrentMinorFormatVersion = kMinorVersion;

	// header size
	uint64 heapOffset = B_BENDIAN_TO_HOST_INT16(header.header_size);
	if (heapOffset < (off_t)sizeof(header)) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Invalid header "
			"size (%" B_PRIu64 ")\n", fFileType, heapOffset);
		return B_BAD_DATA;
	}

	// total size
	uint64 totalSize = B_BENDIAN_TO_HOST_INT64(header.total_size);
	if (fileSize >= 0 && totalSize != (uint64)fileSize) {
		ErrorOutput()->PrintError("Error: Invalid %s file: Total size in "
			"header (%" B_PRIu64 ") doesn't agree with total file size (%"
			B_PRIdOFF ")\n", fFileType, totalSize, fileSize);
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
		B_BENDIAN_TO_HOST_INT16(header.heap_compression),
		B_BENDIAN_TO_HOST_INT32(header.heap_chunk_size), heapOffset,
		compressedHeapSize,
		B_BENDIAN_TO_HOST_INT64(header.heap_size_uncompressed));
	if (error != B_OK)
		return error;

	return B_OK;
}


inline BPositionIO*
ReaderImplBase::File() const
{
	return fFile;
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

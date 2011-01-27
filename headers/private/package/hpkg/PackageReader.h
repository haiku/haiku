/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_READER_H
#define PACKAGE_READER_H


#include <SupportDefs.h>

#include <util/SinglyLinkedList.h>

#include <package/hpkg/PackageAttributeValue.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


class ErrorOutput;
class PackageEntry;
class PackageEntryAttribute;


class LowLevelPackageContentHandler {
public:
	virtual						~LowLevelPackageContentHandler();

	virtual	status_t			HandleAttribute(const char* attributeName,
									const PackageAttributeValue& value,
									void* parentToken, void*& _token) = 0;
	virtual	status_t			HandleAttributeDone(const char* attributeName,
									const PackageAttributeValue& value,
									void* token) = 0;

	virtual	void				HandleErrorOccurred() = 0;
};


class PackageContentHandler {
public:
	virtual						~PackageContentHandler();

	virtual	status_t			HandleEntry(PackageEntry* entry) = 0;
	virtual	status_t			HandleEntryAttribute(PackageEntry* entry,
									PackageEntryAttribute* attribute) = 0;
	virtual	status_t			HandleEntryDone(PackageEntry* entry) = 0;

	virtual	void				HandleErrorOccurred() = 0;
};


class PackageReader {
public:
								PackageReader(ErrorOutput* errorOutput);
								~PackageReader();

			status_t			Init(const char* fileName);
			status_t			Init(int fd, bool keepFD);
			status_t			ParseContent(
									PackageContentHandler* contentHandler);
			status_t			ParseContent(
									LowLevelPackageContentHandler*
										contentHandler);

			int					PackageFileFD()	{ return fFD; }

private:
			struct AttributeType;
			struct AttributeTypeReference;
			struct AttributeHandlerContext;
			struct AttributeHandler;
			struct IgnoreAttributeHandler;
			struct DataAttributeHandler;
			struct AttributeAttributeHandler;
			struct EntryAttributeHandler;
			struct RootAttributeHandler;
			struct PackageAttributeHandler;
			struct PackageContentListHandler;

			typedef PackageAttributeValue AttributeValue;
			typedef SinglyLinkedList<AttributeHandler> AttributeHandlerList;

private:
			status_t			_Init(const char* fileName);

			const char*			_CheckCompression(uint32 compression,
									uint64 compressedLength,
									uint64 uncompressedLength) const;

			status_t			_ParseTOCAttributeTypes();
			status_t			_ParseTOCStrings();

			status_t			_ParseContent(AttributeHandlerContext* context,
									AttributeHandler* rootAttributeHandler);
			status_t			_ParseAttributeTree(
									AttributeHandlerContext* context);

			status_t			_ReadAttributeValue(uint8 type, uint8 encoding,
									AttributeValue& _value);

			status_t			_ReadUnsignedLEB128(uint64& _value);
			status_t			_ReadString(const char*& _string,
									size_t* _stringLength = NULL);

	template<typename Type>
	inline	status_t			_Read(Type& _value);

			status_t			_GetTOCBuffer(size_t size,
									const void*& _buffer);
			status_t			_ReadTOCBuffer(void* buffer, size_t size);

			status_t			_ReadBuffer(off_t offset, void* buffer,
									size_t size);
			status_t			_ReadCompressedBuffer(off_t offset,
									void* buffer, size_t compressedSize,
									size_t uncompressedSize,
									uint32 compression);

	static	int8				_GetStandardIndex(const AttributeType* type);

	inline	AttributeHandler*	_CurrentAttributeHandler() const;
	inline	void				_PushAttributeHandler(
									AttributeHandler* handler);
	inline	AttributeHandler*	_PopAttributeHandler();

private:
			ErrorOutput*		fErrorOutput;
			int					fFD;
			bool				fOwnsFD;

			uint64				fTotalSize;
			uint64				fHeapOffset;
			uint64				fHeapSize;

			uint32				fTOCCompression;
			uint64				fTOCCompressedLength;
			uint64				fTOCUncompressedLength;
			uint64				fTOCSectionOffset;
			uint64				fTOCAttributeTypesLength;
			uint64				fTOCAttributeTypesCount;
			uint64				fTOCStringsLength;
			uint64				fTOCStringsCount;

			uint32				fPackageAttributesCompression;
			uint32				fPackageAttributesCompressedLength;
			uint32				fPackageAttributesUncompressedLength;
			uint64				fPackageAttributesOffset;

			uint8*				fTOCSection;
			uint64				fCurrentTOCOffset;
			AttributeTypeReference* fAttributeTypes;
			char**				fStrings;

			AttributeHandlerList* fAttributeHandlerStack;

			uint8*				fScratchBuffer;
			size_t				fScratchBufferSize;
};


template<typename Type>
status_t
PackageReader::_Read(Type& _value)
{
	return _ReadTOCBuffer(&_value, sizeof(Type));
}


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// PACKAGE_READER_H

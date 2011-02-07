/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_READER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_READER_IMPL_H_


#include <SupportDefs.h>

#include <util/SinglyLinkedList.h>

#include <package/hpkg/PackageAttributeValue.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>


namespace BPackageKit {

namespace BHPKG {


class BPackageEntry;
class BPackageEntryAttribute;
class BErrorOutput;


namespace BPrivate {


class PackageReaderImpl {
public:
								PackageReaderImpl(
									BErrorOutput* errorOutput);
								~PackageReaderImpl();

			status_t			Init(const char* fileName);
			status_t			Init(int fd, bool keepFD);
			status_t			ParseContent(
									BPackageContentHandler* contentHandler);
			status_t			ParseContent(BLowLevelPackageContentHandler*
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

			struct SectionInfo {
				uint32			compression;
				uint32			compressedLength;
				uint32			uncompressedLength;
				uint8*			data;
				uint64			offset;
				uint64			currentOffset;
				uint64			stringsLength;
				uint64			stringsCount;
				char**			strings;
				const char*		name;

				SectionInfo(const char* _name)
					:
					data(NULL),
					strings(NULL),
					name(_name)
				{
				}

				~SectionInfo()
				{
					delete[] strings;
					delete[] data;
				}
			};

			typedef BPackageAttributeValue AttributeValue;
			typedef SinglyLinkedList<AttributeHandler> AttributeHandlerList;

private:
			status_t			_Init(const char* fileName);

			const char*			_CheckCompression(
									const SectionInfo& section) const;

			status_t			_ParseTOCAttributeTypes();

			status_t			_ParseStrings();

			status_t			_ParseContent(AttributeHandlerContext* context,
									AttributeHandler* rootAttributeHandler);
			status_t			_ParseAttributeTree(
									AttributeHandlerContext* context);

			status_t			_ParsePackageAttributes(
									AttributeHandlerContext* context);
			status_t			_ParsePackageVersion(
									BPackageVersionData& _version,
									const char* major = NULL);
			status_t			_ParsePackageProvides(
									BPackageResolvableData& _resolvable,
									BPackageResolvableType providesType);
			status_t			_ParsePackageResolvableExpression(
									BPackageResolvableExpressionData&
										_resolvableExpression,
									const char* resolvableName,
									bool hasChildren);

			status_t			_ReadPackageAttribute(uint8& _id,
									AttributeValue& _value,
									bool* _hasChildren = NULL,
									uint64* _tag = NULL);

			status_t			_ReadAttributeValue(uint8 type, uint8 encoding,
									AttributeValue& _value);

			status_t			_ReadUnsignedLEB128(uint64& _value);
			status_t			_ReadString(const char*& _string,
									size_t* _stringLength = NULL);

	template<typename Type>
	inline	status_t			_Read(Type& _value);

			status_t			_GetTOCBuffer(size_t size,
									const void*& _buffer);
			status_t			_ReadSectionBuffer(void* buffer, size_t size);

			status_t			_ReadBuffer(off_t offset, void* buffer,
									size_t size);
			status_t			_ReadCompressedBuffer(
									const SectionInfo& section);

	static	int8				_GetStandardIndex(const AttributeType* type);

	inline	AttributeHandler*	_CurrentAttributeHandler() const;
	inline	void				_PushAttributeHandler(
									AttributeHandler* handler);
	inline	AttributeHandler*	_PopAttributeHandler();

private:
			BErrorOutput*		fErrorOutput;
			int					fFD;
			bool				fOwnsFD;

			uint64				fTotalSize;
			uint64				fHeapOffset;
			uint64				fHeapSize;

			uint64				fTOCAttributeTypesLength;
			uint64				fTOCAttributeTypesCount;

			SectionInfo			fTOCSection;
			SectionInfo			fPackageAttributesSection;
			SectionInfo*		fCurrentSection;

			AttributeTypeReference* fAttributeTypes;

			AttributeHandlerList* fAttributeHandlerStack;

			uint8*				fScratchBuffer;
			size_t				fScratchBufferSize;
};


template<typename Type>
status_t
PackageReaderImpl::_Read(Type& _value)
{
	return _ReadSectionBuffer(&_value, sizeof(Type));
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_READER_IMPL_H_

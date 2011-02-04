/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_WRITER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_WRITER_IMPL_H_


#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <package/PackageInfo.h>
#include <package/hpkg/PackageWriter.h>
#include <package/hpkg/Strings.h>


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


struct hpkg_header;

class PackageWriterImpl {
public:
								PackageWriterImpl(
									BPackageWriterListener* listener);
								~PackageWriterImpl();

			status_t			Init(const char* fileName);
			status_t			AddEntry(const char* fileName);
			status_t			Finish();

private:
			struct AttributeValue;
			struct AttributeTypeKey;
			struct AttributeType;
			struct AttributeTypeHashDefinition;
			struct Attribute;
			struct AttributeTypeUsageGreater;
			struct Entry;
			struct DataWriter;
			struct DummyDataWriter;
			struct FDDataWriter;
			struct SubPathAdder;
			struct ZlibDataWriter;

			typedef BOpenHashTable<AttributeTypeHashDefinition>
				AttributeTypeTable;
			typedef DoublyLinkedList<Entry> EntryList;

private:
			status_t			_Init(const char* fileName);
			status_t			_Finish();

			status_t			_RegisterEntry(const char* fileName);
			Entry*				_RegisterEntry(Entry* parent,
									const char* name, size_t nameLength,
									bool isImplicit);

			void				_WriteTOC(hpkg_header& header);
			int32				_WriteTOCSections(uint64& _attributeTypesSize,
									uint64& _stringsSize, uint64& _mainSize);
			void				_WriteAttributeTypes();
			int32				_WriteCachedStrings();
			void				_WriteAttributeChildren(Attribute* attribute);

			void				_WritePackageAttributes(hpkg_header& header);

			void				_WriteAttributeValue(
									const AttributeValue& value,
									uint8 encoding);
			void				_WriteUnsignedLEB128(uint64 value);
	inline	void				_WriteString(const char* string);

			void				_WritePackageVersion(
									const BPackageVersion& version);
			void				_WritePackageResolvableExpressionList(
									const BObjectList<
										BPackageResolvableExpression>& list,
									uint8 id);

	template<typename Type>
	inline	void				_Write(const Type& value);

			void				_WriteBuffer(const void* buffer, size_t size,
									off_t offset);

			void				_AddEntry(int dirFD, Entry* entry,
									const char* fileName, char* pathBuffer);

			Attribute*			_AddAttribute(const char* attributeName,
									const AttributeValue& value);

	template<typename Type>
	inline	Attribute*			_AddAttribute(const char* attributeName,
									Type value);

			Attribute*			_AddStringAttribute(const char* attributeName,
									const char* value);
			Attribute*			_AddDataAttribute(const char* attributeName,
									uint64 dataSize, uint64 dataOffset);
			Attribute*			_AddDataAttribute(const char* attributeName,
									uint64 dataSize, const uint8* data);

			CachedString*		_GetCachedString(const char* value);
			AttributeType*		_GetAttributeType(const char* attributeName,
									uint8 type);

			status_t			_AddData(BDataReader& dataReader, off_t size);
			status_t			_WriteUncompressedData(BDataReader& dataReader,
									off_t size, uint64 writeOffset);
			status_t			_WriteZlibCompressedData(BDataReader& dataReader,
									off_t size, uint64 writeOffset,
									uint64& _compressedSize);

private:
			BPackageWriterListener*	fListener;

			const char*			fFileName;
			int					fFD;
			bool				fFinished;
			off_t				fHeapOffset;
			off_t				fHeapEnd;
			void*				fDataBuffer;
			size_t				fDataBufferSize;

			BPackageInfo		fPackageInfo;

			DataWriter*			fDataWriter;

			Entry*				fRootEntry;

			Attribute*			fRootAttribute;
			Attribute*			fTopAttribute;

			CachedStringTable*	fCachedStrings;
			AttributeTypeTable*	fAttributeTypes;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_WRITER_IMPL_H_

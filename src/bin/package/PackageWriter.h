/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_WRITER_H
#define PACKAGE_WRITER_H


#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "Strings.h"


class DataReader;


class PackageWriter {
public:
								PackageWriter();
								~PackageWriter();

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

			void				_WriteAttributeTypes();
			int32				_WriteCachedStrings();
			void				_WriteAttributeChildren(Attribute* attribute);

			void				_WriteAttributeValue(
									const AttributeValue& value,
									uint8 encoding);
			void				_WriteUnsignedLEB128(uint64 value);
	inline	void				_WriteString(const char* string);

	template<typename Type>
	inline	void				_Write(const Type& value);

			void				_WriteBuffer(const void* buffer, size_t size,
									off_t offset);
	inline	void				_WriteBuffer(const void* buffer, size_t size);

			void				_AddEntry(int dirFD, Entry* entry,
									const char* fileName);

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

			status_t			_AddData(DataReader& dataReader, off_t size);
			status_t			_WriteUncompressedData(DataReader& dataReader,
									off_t size, uint64 writeOffset);
			status_t			_WriteZlibCompressedData(DataReader& dataReader,
									off_t size, uint64 writeOffset,
									uint64& _compressedSize);

private:
			const char*			fFileName;
			int					fFD;
			bool				fFinished;
			off_t				fHeapOffset;
			off_t				fHeapEnd;
			void*				fDataBuffer;
			size_t				fDataBufferSize;

			Entry*				fRootEntry;

			Attribute*			fRootAttribute;
			Attribute*			fTopAttribute;

			CachedStringTable*	fCachedStrings;
			AttributeTypeTable*	fAttributeTypes;
};


template<typename Type>
inline void
PackageWriter::_Write(const Type& value)
{
	_WriteBuffer(&value, sizeof(Type));
}


inline void
PackageWriter::_WriteString(const char* string)
{
	_WriteBuffer(string, strlen(string) + 1);
}


inline void
PackageWriter::_WriteBuffer(const void* buffer, size_t size)
{
	_WriteBuffer(buffer, size, fHeapEnd);
	fHeapEnd += size;
}


#endif	// PACKAGE_WRITER_H

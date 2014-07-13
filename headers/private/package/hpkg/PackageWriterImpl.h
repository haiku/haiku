/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_WRITER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_WRITER_IMPL_H_


#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <String.h>

#include <package/hpkg/PackageWriter.h>
#include <package/hpkg/Strings.h>
#include <package/hpkg/WriterImplBase.h>


namespace BPrivate {
	template<typename Value> class RangeArray;
}


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;
class BPackageWriterParameters;


namespace BPrivate {


struct hpkg_header;


class PackageWriterImpl : public WriterImplBase {
	typedef	WriterImplBase		inherited;

public:
								PackageWriterImpl(
									BPackageWriterListener* listener);
								~PackageWriterImpl();

			status_t			Init(const char* fileName,
									const BPackageWriterParameters& parameters);
			status_t			Init(BPositionIO* file, bool keepFile,
									const BPackageWriterParameters& parameters);
			status_t			SetInstallPath(const char* installPath);
			void				SetCheckLicenses(bool checkLicenses);
			status_t			AddEntry(const char* fileName, int fd = -1);
			status_t			Finish();

			status_t			Recompress(BPositionIO* inputFile);
									// to be called after Init(); no Finish()

private:
			struct Attribute;
			struct PackageContentHandler;
			struct Entry;
			struct SubPathAdder;
			struct HeapAttributeOffsetter;

			typedef DoublyLinkedList<Entry> EntryList;

private:
			status_t			_Init(BPositionIO* file, bool keepFile,
									const char* fileName,
									const BPackageWriterParameters& parameters);
			status_t			_Finish();

			status_t			_Recompress(BPositionIO* inputFile);

			status_t			_RegisterEntry(const char* fileName, int fd);
			Entry*				_RegisterEntry(Entry* parent,
									const char* name, size_t nameLength, int fd,
									bool isImplicit);

			status_t			_CheckLicenses();
			bool				_IsEntryInPackage(const char* fileName);

			void				_UpdateReadPackageInfo();
			void				_UpdateCheckEntryCollisions();
			void				_UpdateCheckEntryCollisions(
									Attribute* parentAttribute, int dirFD,
									Entry* entry, const char* fileName,
									char* pathBuffer);
			void				_CompactHeap();
			void				_AttributeRemoved(Attribute* attribute);

			void				_WriteTOC(hpkg_header& header, uint64& _length);
			void				_WriteAttributeChildren(Attribute* attribute);

			void				_WritePackageAttributes(hpkg_header& header,
									uint64& _length);
			uint32				_WritePackageAttributesCompressed(
									uint32& _stringsLengthUncompressed,
									uint32& _attributesLengthUncompressed);

			void				_AddEntry(int dirFD, Entry* entry,
									const char* fileName, char* pathBuffer);
			void				_AddDirectoryChildren(Entry* entry, int fd,
									char* pathBuffer);

			Attribute*			_AddAttribute(BHPKGAttributeID attributeID,
									const AttributeValue& value);

	template<typename Type>
	inline	Attribute*			_AddAttribute(BHPKGAttributeID attributeID,
									Type value);

			Attribute*			_AddStringAttribute(
									BHPKGAttributeID attributeID,
									const char* value);
			Attribute*			_AddDataAttribute(BHPKGAttributeID attributeID,
									uint64 dataSize, uint64 dataOffset);
			Attribute*			_AddDataAttribute(BHPKGAttributeID attributeID,
									uint64 dataSize, const uint8* data);

			status_t			_AddData(BDataReader& dataReader, off_t size);

private:
			BPackageWriterListener*	fListener;

			off_t				fHeapOffset;
			uint16				fHeaderSize;

			::BPrivate::RangeArray<uint64>* fHeapRangesToRemove;

			Entry*				fRootEntry;

			Attribute*			fRootAttribute;
			Attribute*			fTopAttribute;

			StringCache			fStringCache;

			BPackageInfo		fPackageInfo;
			BString				fInstallPath;
			bool				fCheckLicenses;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_WRITER_IMPL_H_

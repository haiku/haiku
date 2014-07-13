/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_READER_IMPL_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_READER_IMPL_H_


#include <package/hpkg/ReaderImplBase.h>


namespace BPackageKit {

namespace BHPKG {


class BPackageEntry;
class BPackageEntryAttribute;


namespace BPrivate {


struct hpkg_header;
class PackageWriterImpl;


class PackageReaderImpl : public ReaderImplBase {
	typedef	ReaderImplBase		inherited;
public:
								PackageReaderImpl(BErrorOutput* errorOutput);
								~PackageReaderImpl();

			status_t			Init(const char* fileName, uint32 flags);
			status_t			Init(int fd, bool keepFD, uint32 flags);
			status_t			Init(BPositionIO* file, bool keepFile,
									uint32 flags, hpkg_header* _header = NULL);
			status_t			ParseContent(
									BPackageContentHandler* contentHandler);
			status_t			ParseContent(BLowLevelPackageContentHandler*
										contentHandler);

			BPositionIO*		PackageFile() const;

			uint64				HeapOffset() const;
			uint64				HeapSize() const;

			PackageFileHeapReader* RawHeapReader() const
									{ return inherited::RawHeapReader(); }
			BAbstractBufferedDataReader* HeapReader() const
									{ return inherited::HeapReader(); }

	inline	const PackageFileSection& TOCSection() const
									{ return fTOCSection; }

protected:
								// from ReaderImplBase
	virtual	status_t			ReadAttributeValue(uint8 type, uint8 encoding,
									AttributeValue& _value);

private:
			struct AttributeAttributeHandler;
			struct EntryAttributeHandler;
			struct RootAttributeHandler;

			friend class PackageWriterImpl;

private:
			status_t			_PrepareSections();

			status_t			_ParseTOC(AttributeHandlerContext* context,
									AttributeHandler* rootAttributeHandler);

			status_t			_GetTOCBuffer(size_t size,
									const void*& _buffer);
private:
			uint64				fHeapOffset;
			uint64				fHeapSize;

			PackageFileSection	fTOCSection;
};


inline BPositionIO*
PackageReaderImpl::PackageFile() const
{
	return File();
}


inline uint64
PackageReaderImpl::HeapOffset() const
{
	return fHeapOffset;
}


inline uint64
PackageReaderImpl::HeapSize() const
{
	return fHeapSize;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_READER_IMPL_H_

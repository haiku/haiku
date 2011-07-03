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


class PackageReaderImpl : public ReaderImplBase {
	typedef	ReaderImplBase		inherited;
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

			int					PackageFileFD() const;

			uint64				HeapOffset() const;
			uint64				HeapSize() const;

protected:
								// from ReaderImplBase
	virtual	status_t			ReadAttributeValue(uint8 type, uint8 encoding,
									AttributeValue& _value);

private:
			struct DataAttributeHandler;
			struct AttributeAttributeHandler;
			struct EntryAttributeHandler;
			struct RootAttributeHandler;

private:
			status_t			_ParseTOC(AttributeHandlerContext* context,
									AttributeHandler* rootAttributeHandler);

			status_t			_GetTOCBuffer(size_t size,
									const void*& _buffer);
private:
			uint64				fTotalSize;
			uint64				fHeapOffset;
			uint64				fHeapSize;

			SectionInfo			fTOCSection;
};


inline int
PackageReaderImpl::PackageFileFD() const
{
	return FD();
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

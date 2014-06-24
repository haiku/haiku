/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageDataReader.h>

#include <string.h>

#include <algorithm>
#include <new>

#include <package/hpkg/HPKGDefsPrivate.h>
#include <package/hpkg/PackageData.h>


namespace BPackageKit {

namespace BHPKG {


using namespace BPrivate;


// #pragma mark - PackageDataHeapReader


class PackageDataHeapReader : public BAbstractBufferedDataReader {
public:
	PackageDataHeapReader(BAbstractBufferedDataReader* dataReader,
		const BPackageData& data)
		:
		fDataReader(dataReader),
		fOffset(data.Offset()),
		fSize(data.Size())
	{
	}

	virtual status_t ReadData(off_t offset, void* buffer, size_t size)
	{
		if (size == 0)
			return B_OK;

		if (offset < 0)
			return B_BAD_VALUE;

		if ((uint64)offset > fSize || size > fSize - offset)
			return B_BAD_VALUE;

		return fDataReader->ReadData(fOffset + offset, buffer, size);
	}

	virtual status_t ReadDataToOutput(off_t offset, size_t size,
		BDataIO* output)
	{
		if (size == 0)
			return B_OK;

		if (offset < 0)
			return B_BAD_VALUE;

		if ((uint64)offset > fSize || size > fSize - offset)
			return B_BAD_VALUE;

		return fDataReader->ReadDataToOutput(fOffset + offset, size, output);
	}

private:
	BAbstractBufferedDataReader*	fDataReader;
	uint64							fOffset;
	uint64							fSize;
};


// #pragma mark - PackageDataInlineReader


class PackageDataInlineReader : public BBufferDataReader {
public:
	PackageDataInlineReader(const BPackageData& data)
		:
		BBufferDataReader(fData.InlineData(), data.Size()),
		fData(data)
	{
	}

private:
	BPackageData	fData;
};


// #pragma mark - BPackageDataReaderFactory


BPackageDataReaderFactory::BPackageDataReaderFactory()
{
}


status_t
BPackageDataReaderFactory::CreatePackageDataReader(
	BAbstractBufferedDataReader* dataReader, const BPackageData& data,
	BAbstractBufferedDataReader*& _reader)
{
	BAbstractBufferedDataReader* reader;
	if (data.IsEncodedInline())
		reader = new(std::nothrow) PackageDataInlineReader(data);
	else
		reader = new(std::nothrow) PackageDataHeapReader(dataReader, data);

	if (reader == NULL)
		return B_NO_MEMORY;

	_reader = reader;
	return B_OK;
}


}	// namespace BHPKG

}	// namespace BPackageKit

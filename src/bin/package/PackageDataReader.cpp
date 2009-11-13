/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageDataReader.h"

#include <new>

#include <haiku_package.h>

#include "PackageData.h"


// #pragma mark - PackageDataReader


PackageDataReader::PackageDataReader(DataReader* dataReader)
	:
	fDataReader(dataReader)
{
}


PackageDataReader::~PackageDataReader()
{
}


// #pragma mark - UncompressedPackageDataReader


class UncompressedPackageDataReader : public PackageDataReader {
public:
	UncompressedPackageDataReader(DataReader* dataReader)
		:
		PackageDataReader(dataReader)
	{
	}

	status_t Init(const PackageData& data)
	{
		fOffset = data.Offset();
		fSize = data.UncompressedSize();
		return B_OK;
	}

	virtual uint64 Size() const
	{
		return fSize;
	}

	virtual size_t BlockSize() const
	{
		// TODO: Some other value?
		return 64 * 1024;
	}

	virtual status_t ReadData(off_t offset, void* buffer, size_t size)
	{
		if (size == 0)
			return B_OK;

		if (offset < 0)
			return B_BAD_VALUE;

		if ((uint64)offset > fSize || size > fSize - offset)
			return B_ERROR;

		return fDataReader->ReadData(fOffset + offset, buffer, size);
	}

private:
	uint64	fOffset;
	uint64	fSize;
};


// #pragma mark - PackageDataReaderFactory


/*static*/ status_t
PackageDataReaderFactory::CreatePackageDataReader(DataReader* dataReader,
	const PackageData& data, PackageDataReader*& _reader)
{
	PackageDataReader* reader;

	switch (data.Compression()) {
		case B_HPKG_COMPRESSION_NONE:
			reader = new(std::nothrow) UncompressedPackageDataReader(
				dataReader);
			break;
		case B_HPKG_COMPRESSION_ZLIB:
			// TODO:...
			return B_UNSUPPORTED;;
		default:
			return B_BAD_VALUE;
	}

	if (reader == NULL)
		return B_NO_MEMORY;

	status_t error = reader->Init(data);
	if (error != B_OK) {
		delete reader;
		return error;
	}

	_reader = reader;
	return B_OK;
}

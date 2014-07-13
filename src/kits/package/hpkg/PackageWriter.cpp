/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageWriter.h>

#include <new>

#include <package/hpkg/PackageWriterImpl.h>


namespace BPackageKit {

namespace BHPKG {


// #pragma mark - BPackageWriterParameters


BPackageWriterParameters::BPackageWriterParameters()
	:
	fFlags(0),
	fCompression(B_HPKG_COMPRESSION_ZLIB),
	fCompressionLevel(B_HPKG_COMPRESSION_LEVEL_BEST)
{
}


BPackageWriterParameters::~BPackageWriterParameters()
{
}


uint32
BPackageWriterParameters::Flags() const
{
	return fFlags;
}


void
BPackageWriterParameters::SetFlags(uint32 flags)
{
	fFlags = flags;
}


uint32
BPackageWriterParameters::Compression() const
{
	return fCompression;
}


void
BPackageWriterParameters::SetCompression(uint32 compression)
{
	fCompression = compression;
}


int32
BPackageWriterParameters::CompressionLevel() const
{
	return fCompressionLevel;
}


void
BPackageWriterParameters::SetCompressionLevel(int32 compressionLevel)
{
	fCompressionLevel = compressionLevel;
}


// #pragma mark - BPackageWriter


BPackageWriter::BPackageWriter(BPackageWriterListener* listener)
	:
	fImpl(new (std::nothrow) PackageWriterImpl(listener))
{
}


BPackageWriter::~BPackageWriter()
{
	delete fImpl;
}


status_t
BPackageWriter::Init(const char* fileName,
	const BPackageWriterParameters* parameters)
{
	if (fImpl == NULL)
		return B_NO_MEMORY;

	BPackageWriterParameters defaultParameters;

	return fImpl->Init(fileName,
		parameters != NULL ? *parameters : defaultParameters);
}


status_t
BPackageWriter::Init(BPositionIO* file, bool keepFile,
	const BPackageWriterParameters* parameters)
{
	if (fImpl == NULL)
		return B_NO_MEMORY;

	BPackageWriterParameters defaultParameters;

	return fImpl->Init(file, keepFile,
		parameters != NULL ? *parameters : defaultParameters);
}


status_t
BPackageWriter::SetInstallPath(const char* installPath)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->SetInstallPath(installPath);
}


void
BPackageWriter::SetCheckLicenses(bool checkLicenses)
{
	if (fImpl != NULL)
		fImpl->SetCheckLicenses(checkLicenses);
}


status_t
BPackageWriter::AddEntry(const char* fileName, int fd)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->AddEntry(fileName, fd);
}


status_t
BPackageWriter::Finish()
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Finish();
}


status_t
BPackageWriter::Recompress(BPositionIO* inputFile)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Recompress(inputFile);
}


}	// namespace BHPKG

}	// namespace BPackageKit

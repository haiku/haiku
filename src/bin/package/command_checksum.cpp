/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <File.h>
#include <String.h>

#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageWriter.h>

#include <DataPositionIOWrapper.h>
#include <FdIO.h>
#include <SHA256.h>

#include "package.h"
#include "PackageWriterListener.h"


using BPackageKit::BHPKG::BPackageWriter;
using BPackageKit::BHPKG::BPackageWriterListener;
using BPackageKit::BHPKG::BPackageWriterParameters;


struct ChecksumIO : BDataIO {
	ChecksumIO()
	{
		fChecksummer.Init();
	}

	virtual ssize_t Write(const void* buffer, size_t size)
	{
		if (size > 0)
			fChecksummer.Update(buffer, size);
		return (ssize_t)size;
	}

	BString Digest()
	{
		const uint8* digest = fChecksummer.Digest();
		BString hex;
		size_t length = fChecksummer.DigestLength();
		char* buffer = hex.LockBuffer(length * 2);
		if (buffer == NULL)
		{
			throw std::bad_alloc();
		}

		for (size_t i = 0; i < length; i++)
			snprintf(buffer + 2 * i, 3, "%02x", digest[i]);

		hex.UnlockBuffer();
		return hex;
	}

private:
	SHA256	fChecksummer;
};


static BPositionIO*
create_stdio(bool isInput)
{
	BFdIO* dataIO = new BFdIO(isInput ? 0 : 1, false);
	return new BDataPositionIOWrapper(dataIO);
}


int
command_checksum(int argc, const char* const* argv)
{
	bool quiet = false;
	bool verbose = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "quiet", no_argument, 0, 'q' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+hqv",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			case 'q':
				quiet = true;
				break;

			case 'v':
				verbose = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The optional remaining argument is the package file.
	if (argc - optind > 1)
		print_usage_and_exit(true);

	const char* packageFileName = optind < argc ? argv[optind++] : NULL;

	// open the input package
	status_t error = B_OK;
	BPositionIO* inputFile;
	if (packageFileName == NULL || strcmp(packageFileName, "-") == 0) {
		inputFile = create_stdio(true);
	} else {
		BFile* inputFileFile = new BFile;
		error = inputFileFile->SetTo(packageFileName, O_RDONLY);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open input file \"%s\": %s\n",
				packageFileName, strerror(error));
			return 1;
		}
		inputFile = inputFileFile;
	}

	// write the output package to a BDataIO that computes the checksum
	BPackageWriterParameters writerParameters;
	writerParameters.SetCompressionLevel(0);
	writerParameters.SetCompression(
		BPackageKit::BHPKG::B_HPKG_COMPRESSION_NONE);

	PackageWriterListener listener(verbose, quiet);
	BPackageWriter packageWriter(&listener);
	ChecksumIO outputFile;
	error = packageWriter.Init(new BDataPositionIOWrapper(&outputFile), true,
		&writerParameters);
	if (error != B_OK)
		return 1;

	error = packageWriter.Recompress(inputFile);
	if (error != B_OK)
		return 1;

	printf("%s\n", outputFile.Digest().String());

	return 0;
}

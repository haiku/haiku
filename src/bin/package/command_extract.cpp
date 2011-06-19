/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <fs_attr.h>

#include <AutoDeleter.h>

#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReader.h>
#include <package/BlockBufferCacheNoLock.h>

#include "package.h"
#include "StandardErrorOutput.h"


using namespace BPackageKit::BHPKG;
using BPackageKit::BBlockBufferCacheNoLock;


struct PackageContentExtractHandler : BPackageContentHandler {
	PackageContentExtractHandler(int packageFileFD)
		:
		fBufferCache(B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB, 2),
		fPackageFileReader(packageFileFD),
		fDataBuffer(NULL),
		fDataBufferSize(0),
		fErrorOccurred(false)
	{
	}

	~PackageContentExtractHandler()
	{
		free(fDataBuffer);
	}

	status_t Init()
	{
		status_t error = fBufferCache.Init();
		if (error != B_OK)
			return error;

		fDataBufferSize = 64 * 1024;
		fDataBuffer = malloc(fDataBufferSize);
		if (fDataBuffer == NULL)
			return B_NO_MEMORY;

		return B_OK;
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		// create a token
		Token* token = new(std::nothrow) Token;
		if (token == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<Token> tokenDeleter(token);

		// get parent FD
		int parentFD = AT_FDCWD;
		if (entry->Parent() != NULL)
			parentFD = ((Token*)entry->Parent()->UserToken())->fd;

		// check whether something is in the way
		struct stat st;
		bool entryExists = fstatat(parentFD, entry->Name(), &st,
			AT_SYMLINK_NOFOLLOW) == 0;
		if (entryExists) {
			if (S_ISREG(entry->Mode()) || S_ISLNK(entry->Mode())) {
				// If the entry in the way is a regular file or a symlink,
				// remove it, otherwise fail.
				if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
					fprintf(stderr, "Error: Can't create entry \"%s\", since "
						"something is in the way\n", entry->Name());
					return B_FILE_EXISTS;
				}

				if (unlinkat(parentFD, entry->Name(), 0) != 0) {
					fprintf(stderr, "Error: Failed to unlink entry \"%s\": %s\n",
						entry->Name(), strerror(errno));
					return errno;
				}

				entryExists = false;
			} else if (S_ISDIR(entry->Mode())) {
				// If the entry in the way is a directory, merge, otherwise
				// fail.
				if (!S_ISDIR(st.st_mode)) {
					fprintf(stderr, "Error: Can't create directory \"%s\", "
						"since something is in the way\n", entry->Name());
					return B_FILE_EXISTS;
				}
			}
		}

		// create the entry
		int fd = -1;
		if (S_ISREG(entry->Mode())) {
			// create the file
			fd = openat(parentFD, entry->Name(), O_RDWR | O_CREAT | O_EXCL,
				entry->Mode() & ALLPERMS);
			if (fd < 0) {
				fprintf(stderr, "Error: Failed to create file \"%s\": %s\n",
					entry->Name(), strerror(errno));
				return errno;
			}

			// write data
			status_t error;
			const BPackageData& data = entry->Data();
			if (data.IsEncodedInline()) {
				BBufferDataReader dataReader(data.InlineData(),
					data.CompressedSize());
				error = _ExtractFileData(&dataReader, data, fd);
			} else
				error = _ExtractFileData(&fPackageFileReader, data, fd);

			if (error != B_OK)
				return error;
		} else if (S_ISLNK(entry->Mode())) {
			// create the symlink
			const char* symlinkPath = entry->SymlinkPath();
			if (symlinkat(symlinkPath != NULL ? symlinkPath : "", parentFD,
					entry->Name()) != 0) {
				fprintf(stderr, "Error: Failed to create symlink \"%s\": %s\n",
					entry->Name(), strerror(errno));
				return errno;
			}
// TODO: Set symlink permissions?
 		} else if (S_ISDIR(entry->Mode())) {
			// create the directory, if necessary
			if (!entryExists
				&& mkdirat(parentFD, entry->Name(), entry->Mode() & ALLPERMS)
					!= 0) {
				fprintf(stderr, "Error: Failed to create directory \"%s\": "
					"%s\n", entry->Name(), strerror(errno));
				return errno;
			}
		} else {
			fprintf(stderr, "Error: Invalid file type for entry \"%s\"\n",
				entry->Name());
			return B_BAD_DATA;
		}

		// If not done yet (symlink, dir), open the node -- we need the FD.
		if (fd < 0) {
			fd = openat(parentFD, entry->Name(), O_RDONLY | O_NOTRAVERSE);
			if (fd < 0) {
				fprintf(stderr, "Error: Failed to open entry \"%s\": %s\n",
					entry->Name(), strerror(errno));
				return errno;
			}
		}
		token->fd = fd;

		// set the file times
		if (!entryExists) {
			timespec times[2] = {entry->AccessTime(), entry->ModifiedTime()};
			futimens(fd, times);

			// set user/group
			// TODO:...
		}

		entry->SetUserToken(tokenDeleter.Detach());
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		int entryFD = ((Token*)entry->UserToken())->fd;

		// create the attribute
		int fd = fs_fopen_attr(entryFD, attribute->Name(), attribute->Type(),
			O_WRONLY | O_CREAT | O_TRUNC);
		if (fd < 0) {
				fprintf(stderr, "Error: Failed to create attribute \"%s\" of "
					"file \"%s\": %s\n", attribute->Name(), entry->Name(),
					strerror(errno));
				return errno;
		}

		// write data
		status_t error;
		const BPackageData& data = attribute->Data();
		if (data.IsEncodedInline()) {
			BBufferDataReader dataReader(data.InlineData(),
				data.CompressedSize());
			error = _ExtractFileData(&dataReader, data, fd);
		} else
			error = _ExtractFileData(&fPackageFileReader, data, fd);

		fs_close_attr(fd);

		return error;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		if (Token* token = (Token*)entry->UserToken()) {
			delete token;
			entry->SetUserToken(NULL);
		}

		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	struct Token {
		int	fd;

		Token()
			:
			fd(-1)
		{
		}

		~Token()
		{
			if (fd >= 0)
				close(fd);
		}
	};

private:
	status_t _ExtractFileData(BDataReader* dataReader, const BPackageData& data,
		int fd)
	{
		// create a BPackageDataReader
		BPackageDataReader* reader;
		status_t error = BPackageDataReaderFactory(&fBufferCache)
			.CreatePackageDataReader(dataReader, data, reader);
		if (error != B_OK)
			return error;
		ObjectDeleter<BPackageDataReader> readerDeleter(reader);

		// write the data
		off_t bytesRemaining = data.UncompressedSize();
		off_t offset = 0;
		while (bytesRemaining > 0) {
			// read
			size_t toCopy = std::min((off_t)fDataBufferSize, bytesRemaining);
			error = reader->ReadData(offset, fDataBuffer, toCopy);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to read data: %s\n",
					strerror(error));
				return error;
			}

			// write
			ssize_t bytesWritten = write_pos(fd, offset, fDataBuffer, toCopy);
			if (bytesWritten < 0) {
				fprintf(stderr, "Error: Failed to write data: %s\n",
					strerror(errno));
				return errno;
			}
			if ((size_t)bytesWritten != toCopy) {
				fprintf(stderr, "Error: Failed to write all data (%zd of "
					"%zu)\n", bytesWritten, toCopy);
				return B_ERROR;
			}

			offset += toCopy;
			bytesRemaining -= toCopy;
		}

		return B_OK;
	}

private:
	BBlockBufferCacheNoLock	fBufferCache;
	BFDDataReader			fPackageFileReader;
	void*					fDataBuffer;
	size_t					fDataBufferSize;
	bool					fErrorOccurred;
};


int
command_extract(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+C:h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One argument should remain -- the package file name.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];

	// open package
	StandardErrorOutput errorOutput;
	BPackageReader packageReader(&errorOutput);
	status_t error = packageReader.Init(packageFileName);
printf("Init(): %s\n", strerror(error));
	if (error != B_OK)
		return 1;

	// change directory, if requested
	if (changeToDirectory != NULL) {
		if (chdir(changeToDirectory) != 0) {
			fprintf(stderr, "Error: Failed to change the current working "
				"directory to \"%s\": %s\n", changeToDirectory,
				strerror(errno));
		}
	}

	// extract
	PackageContentExtractHandler handler(packageReader.PackageFileFD());
	error = handler.Init();
	if (error != B_OK)
		return 1;

	error = packageReader.ParseContent(&handler);
printf("ParseContent(): %s\n", strerror(error));
	if (error != B_OK)
		return 1;

	return 0;
}

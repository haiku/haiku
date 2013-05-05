/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UnzipEngine.h"

#include <new>

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Node.h>
#include <Path.h>
#include <String.h>

#include "CommandPipe.h"
#include "SemaphoreLocker.h"
#include "ProgressReporter.h"


using std::nothrow;


UnzipEngine::UnzipEngine(ProgressReporter* reporter,
		sem_id cancelSemaphore)
	:
	fPackage(""),
	fRetrievingListing(false),

	fBytesToUncompress(0),
	fBytesUncompressed(0),
	fLastBytesUncompressed(0),
	fItemsToUncompress(0),
	fItemsUncompressed(0),
	fLastItemsUncompressed(0),

	fProgressReporter(reporter),
	fCancelSemaphore(cancelSemaphore)
{
}


UnzipEngine::~UnzipEngine()
{
}


status_t
UnzipEngine::SetTo(const char* pathToPackage, const char* destinationFolder)
{
	fPackage = pathToPackage;
	fDestinationFolder = destinationFolder;

	fEntrySizeMap.Clear();

	fBytesToUncompress = 0;
	fBytesUncompressed = 0;
	fLastBytesUncompressed = 0;
	fItemsToUncompress = 0;
	fItemsUncompressed = 0;
	fLastItemsUncompressed = 0;

	BPrivate::BCommandPipe commandPipe;
	status_t ret = commandPipe.AddArg("unzip");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-l");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fPackage.String());
	if (ret != B_OK)
		return ret;

	// Launch the unzip thread and start reading the stdout and stderr output.
	FILE* stdOutAndErrPipe = NULL;
	thread_id unzipThread = commandPipe.PipeInto(&stdOutAndErrPipe);
	if (unzipThread < 0)
		return (status_t)unzipThread;

	fRetrievingListing = true;
	ret = commandPipe.ReadLines(stdOutAndErrPipe, this);
	fRetrievingListing = false;

	printf("%" B_PRIu64 " items in %" B_PRIdOFF " bytes\n", fItemsToUncompress,
		fBytesToUncompress);

	return ret;
}


status_t
UnzipEngine::UnzipPackage()
{
	if (fItemsToUncompress == 0)
		return B_NO_INIT;

	BPrivate::BCommandPipe commandPipe;
	status_t ret = commandPipe.AddArg("unzip");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-o");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fPackage.String());
	if (ret == B_OK)
		ret = commandPipe.AddArg("-d");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fDestinationFolder.String());
	if (ret != B_OK) {
		fprintf(stderr, "Faild to construct argument list for unzip "
			"process: %s\n", strerror(ret));
		return ret;
	}

	// Launch the unzip thread and start reading the stdout and stderr output.
	FILE* stdOutAndErrPipe = NULL;
	thread_id unzipThread = commandPipe.PipeInto(&stdOutAndErrPipe);
	if (unzipThread < 0)
		return (status_t)unzipThread;

	ret = commandPipe.ReadLines(stdOutAndErrPipe, this);
	if (ret != B_OK) {
		fprintf(stderr, "Piping the unzip process failed: %s\n",
			strerror(ret));
		return ret;
	}

	// Add the contents of a potentially existing .OptionalPackageDescription
	// to the COPYRIGHTS attribute of AboutSystem.
	BPath descriptionPath(fDestinationFolder.String(),
		".OptionalPackageDescription");
	ret = descriptionPath.InitCheck();
	if (ret != B_OK) {
		fprintf(stderr, "Failed to construct path to "
			".OptionalPackageDescription: %s\n", strerror(ret));
		return ret;
	}

	BEntry descriptionEntry(descriptionPath.Path());
	if (!descriptionEntry.Exists())
		return B_OK;

	BFile descriptionFile(&descriptionEntry, B_READ_ONLY);
	ret = descriptionFile.InitCheck();
	if (ret != B_OK) {
		fprintf(stderr, "Failed to construct file to "
			".OptionalPackageDescription: %s\n", strerror(ret));
		return ret;
	}

	BPath aboutSystemPath(fDestinationFolder.String(),
		"system/apps/AboutSystem");
	ret = aboutSystemPath.InitCheck();
	if (ret != B_OK) {
		fprintf(stderr, "Failed to construct path to AboutSystem: %s\n",
			strerror(ret));
		return ret;
	}

	BNode aboutSystemNode(aboutSystemPath.Path());
	ret = aboutSystemNode.InitCheck();
	if (ret != B_OK) {
		fprintf(stderr, "Failed to construct node to AboutSystem: %s\n",
			strerror(ret));
		return ret;
	}

	const char* kCopyrightsAttrName = "COPYRIGHTS";

	BString copyrightAttr;
	ret = aboutSystemNode.ReadAttrString(kCopyrightsAttrName, &copyrightAttr);
	if (ret != B_OK && ret != B_ENTRY_NOT_FOUND) {
		fprintf(stderr, "Failed to read current COPYRIGHTS attribute from "
			"AboutSystem: %s\n", strerror(ret));
		return ret;
	}

	// Append the contents of the current optional package description to
	// the existing COPYRIGHTS attribute from AboutSystem
	size_t bufferSize = 2048;
	char buffer[bufferSize + 1];
	buffer[bufferSize] = '\0';
	while (true) {
		ssize_t read = descriptionFile.Read(buffer, bufferSize);
		if (read > 0) {
			int32 length = copyrightAttr.Length();
			if (read < (ssize_t)bufferSize)
				buffer[read] = '\0';
			int32 bufferLength = strlen(buffer);
				// Should be "read", but maybe we have a zero in the
				// buffer in which case the next check would be fooled.
			copyrightAttr << buffer;
			if (copyrightAttr.Length() != length + bufferLength) {
				fprintf(stderr, "Failed to append buffer to COPYRIGHTS "
					"attribute.\n");
				return B_NO_MEMORY;
			}
		} else
			break;
	}

	if (copyrightAttr[copyrightAttr.Length() - 1] != '\n')
		copyrightAttr << '\n\n';
	else
		copyrightAttr << '\n';

	ret = aboutSystemNode.WriteAttrString(kCopyrightsAttrName, &copyrightAttr);
	if (ret != B_OK && ret != B_ENTRY_NOT_FOUND) {
		fprintf(stderr, "Failed to read current COPYRIGHTS attribute from "
			"AboutSystem: %s\n", strerror(ret));
		return ret;
	}

	// Don't leave the .OptionalPackageDescription behind.
	descriptionFile.Unset();
	descriptionEntry.Remove();

	return B_OK;
}


// #pragma mark -


bool
UnzipEngine::IsCanceled()
{
	if (fCancelSemaphore < 0)
		return false;

	SemaphoreLocker locker(fCancelSemaphore);
	return !locker.IsLocked();
}


status_t
UnzipEngine::ReadLine(const BString& line)
{
	if (fRetrievingListing)
		return _ReadLineListing(line);
	else
		return _ReadLineExtract(line);
}


status_t
UnzipEngine::_ReadLineListing(const BString& line)
{
	static const char* kListingFormat = "%llu  %s %s   %s\n";

	const char* string = line.String();
	while (string[0] == ' ')
		string++;

	uint64 bytes;
	char date[16];
	char time[16];
	char path[1024];
	if (sscanf(string, kListingFormat, &bytes, &date, &time, &path) == 4) {
		fBytesToUncompress += bytes;

		BString itemPath(path);
		BString itemName(path);
		int leafPos = itemPath.FindLast('/');
		if (leafPos >= 0)
			itemName = itemPath.String() + leafPos + 1;

		// We check if the target folder exists and don't increment
		// the item count in that case. Unzip won't report on folders that did
		// not need to be created. This may mess up our current item count.
		uint32 itemCount = 1;
		if (bytes == 0 && itemName.Length() == 0) {
			// a folder?
			BPath destination(fDestinationFolder.String());
			if (destination.Append(itemPath.String()) == B_OK) {
				BEntry test(destination.Path());
				if (test.Exists() && test.IsDirectory()) {
//					printf("ignoring %s\n", itemPath.String());
					itemCount = 0;
				}
			}
		}

		fItemsToUncompress += itemCount;

//		printf("item %s with %llu bytes to %s\n", itemName.String(),
//			bytes, itemPath.String());

		fEntrySizeMap.Put(itemName.String(), bytes);
	} else {
//		printf("listing not understood: %s", string);
	}

	return B_OK;
}


status_t
UnzipEngine::_ReadLineExtract(const BString& line)
{
	const char* kCreatingFormat = "   creating:";
	const char* kInflatingFormat = "  inflating:";
	const char* kLinkingFormat = "    linking:";
	if (line.FindFirst(kCreatingFormat) == 0
		|| line.FindFirst(kInflatingFormat) == 0
		|| line.FindFirst(kLinkingFormat) == 0) {

		fItemsUncompressed++;

		BString itemPath;

		int pos = line.FindLast(" -> ");
		if (pos > 0)
			line.CopyInto(itemPath, 13, pos - 13);
		else 
			line.CopyInto(itemPath, 13, line.CountChars() - 14);

		itemPath.Trim();
		pos = itemPath.FindLast('/');
		BString itemName = itemPath.String() + pos + 1;
		itemPath.Truncate(pos);

		off_t bytes = 0;
		if (fEntrySizeMap.ContainsKey(itemName.String())) {
			bytes = fEntrySizeMap.Get(itemName.String());
			fBytesUncompressed += bytes;
		}

//		printf("%llu extracted %s to %s (%llu)\n", fItemsUncompressed,
//			itemName.String(), itemPath.String(), bytes);

		_UpdateProgress(itemName.String(), itemPath.String());
	} else {
//		printf("ignored: '%s'\n", line.String());
	}

	return B_OK;
}


void
UnzipEngine::_UpdateProgress(const char* item, const char* targetFolder)
{
	if (fProgressReporter == NULL)
		return;

	uint64 items = 0;
	if (fLastItemsUncompressed < fItemsUncompressed) {
		items = fItemsUncompressed - fLastItemsUncompressed;
		fLastItemsUncompressed = fItemsUncompressed;
	}

	off_t bytes = 0;
	if (fLastBytesUncompressed < fBytesUncompressed) {
		bytes = fBytesUncompressed - fLastBytesUncompressed;
		fLastBytesUncompressed = fBytesUncompressed;
	}

	fProgressReporter->ItemsWritten(items, bytes, item, targetFolder);
}

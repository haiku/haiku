/*
 * Copyright 2008-2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */

#include "UnzipEngine.h"

#include <new>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include <Directory.h>
#include <fs_attr.h>
#include <NodeInfo.h>
#include <Path.h>
#include <String.h>
#include <SymLink.h>

#include "CommandPipe.h"
#include "SemaphoreLocker.h"


using std::nothrow;


UnzipEngine::UnzipEngine(const BMessenger& messenger, BMessage* message)
	:
	fPackage(""),

	fBytesToUncompress(0),
	fBytesUncompressed(0),
	fItemsToUncompress(0),
	fItemsUncompressed(0),

	fCurrentTargetFolder(NULL),
	fCurrentItem(NULL),

	fMessenger(messenger),
	fMessage(message)
{
}


UnzipEngine::~UnzipEngine()
{
	delete fMessage;
}


status_t
UnzipEngine::SetTo(const char* pathToPackage)
{
	fPackage = pathToPackage;

	BPrivate::BCommandPipe commandPipe;
	status_t ret = commandPipe.AddArg("unzip");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-Z");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-t");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fPackage.String());
	if (ret != B_OK)
		return ret;

	// Launch the unzip thread and start reading the stdout and stderr output.
	FILE* stdOutAndErrPipe = NULL;
	thread_id unzipThread = commandPipe.PipeInto(&stdOutAndErrPipe);
	if (unzipThread < 0)
		return (status_t)unzipThread;

	BString result = commandPipe.ReadLines(stdOutAndErrPipe);
	static const char* kListingFormat = "%llu files, %llu bytes uncompressed, "
		"%llu bytes compressed: %f%%";

	uint64 bytesCompressed;
	float compresssionRatio;
	if (sscanf(result.String(), kListingFormat, &fItemsToUncompress,
		&fBytesToUncompress, &bytesCompressed, &compresssionRatio) != 4) {
		fBytesToUncompress = 0;
		fItemsToUncompress = 0;
		fprintf(stderr, "error reading command output: %s\n", result.String());
		return B_ERROR;
	}

	printf("%s: %llu items in %llu bytes\n", pathToPackage, fItemsToUncompress,
		fBytesToUncompress);

	return B_OK;
}


status_t
UnzipEngine::UnzipPackage(const char* destinationFolder,
	sem_id cancelSemaphore)
{
	if (fItemsToUncompress == 0)
		return B_NO_INIT;

	if (fMessage) {
		BMessage message(*fMessage);
		message.AddString("status", "Extracting package.");
		fMessenger.SendMessage(&message);
	}

	BPrivate::BCommandPipe commandPipe;
	status_t ret = commandPipe.AddArg("unzip");
	if (ret == B_OK)
		ret = commandPipe.AddArg("-o");
	if (ret == B_OK)
		ret = commandPipe.AddArg(fPackage.String());
	if (ret == B_OK)
		ret = commandPipe.AddArg("-d");
	if (ret == B_OK)
		ret = commandPipe.AddArg(destinationFolder);
	if (ret == B_OK)
		ret = commandPipe.AddArg("-x");
	if (ret == B_OK)
		ret = commandPipe.AddArg(".OptionalPackageDescription");
	if (ret != B_OK)
		return ret;

	// Launch the unzip thread and start reading the stdout and stderr output.
	FILE* stdOutAndErrPipe = NULL;
	thread_id unzipThread = commandPipe.PipeInto(&stdOutAndErrPipe);
	if (unzipThread < 0)
		return (status_t)unzipThread;

	return _ReadFromPipe(stdOutAndErrPipe, commandPipe, cancelSemaphore);
}


// #pragma mark -


status_t
UnzipEngine::_ReadFromPipe(FILE* stdOutAndErrPipe,
	BPrivate::BCommandPipe& commandPipe, sem_id cancelSemaphore)
{
	class LineReader : public BPrivate::BCommandPipe::LineReader {
	public:
		LineReader(sem_id cancelSemaphore, off_t bytesToUncompress,
				uint64 itemsToUncompress, BMessenger& messenger,
				const BMessage* message)
			:
			fCancelSemaphore(cancelSemaphore),

			fBytesToUncompress(itemsToUncompress),
			fBytesUncompressed(0),
			fItemsToUncompress(itemsToUncompress),
			fItemsUncompressed(0),

			fMessenger(messenger),
			fMessage(message)
		{
		}

		virtual bool IsCanceled()
		{
			if (fCancelSemaphore < 0)
				return false;

			SemaphoreLocker locker(fCancelSemaphore);
			return !locker.IsLocked();
		}

		virtual status_t ReadLine(const BString& line)
		{
			char item[1024];
			char linkTarget[256];
			const char* kInflatingFormat = "   inflating: %s\n";
			const char* kLinkingFormat = "     linking: %s -> %s\n";
			if (sscanf(line.String(), kInflatingFormat, &item) == 1
				|| sscanf(line.String(), kLinkingFormat, &item,
					&linkTarget) == 2) {
				BString itemPath(item);
				int pos = itemPath.FindLast('/');
				BString itemName = itemPath.String() + pos + 1;
				itemPath.Truncate(pos);
				printf("extracted %s to %s\n", itemName.String(),
					itemPath.String());
			} else {
				printf("ignored: %s", line.String());
			}

			return B_OK;
		}

	private:
		sem_id			fCancelSemaphore;

		off_t			fBytesToUncompress;
		off_t			fBytesUncompressed;
		uint64			fItemsToUncompress;
		uint64			fItemsUncompressed;

		BMessenger&		fMessenger;
		const BMessage*	fMessage;
	};

	LineReader lineReader(cancelSemaphore, fBytesToUncompress,
		fItemsToUncompress, fMessenger, fMessage);

	return commandPipe.ReadLines(stdOutAndErrPipe, &lineReader);
}


void
UnzipEngine::_UpdateProgress()
{
	if (fMessage != NULL) {
		BMessage message(*fMessage);
		float progress = 100.0 * fBytesUncompressed / fBytesToUncompress;
		message.AddFloat("progress", progress);
		message.AddInt32("current", fItemsUncompressed);
		message.AddInt32("maximum", fItemsToUncompress);
		message.AddString("item", fCurrentItem);
		message.AddString("folder", fCurrentTargetFolder);
		fMessenger.SendMessage(&message);
	}
}

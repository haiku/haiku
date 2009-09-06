/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef UNZIP_ENGINE_H
#define UNZIP_ENGINE_H


#include <stdio.h>

#include <Messenger.h>
#include <String.h>

#include "CommandPipe.h"
#include "HashMap.h"
#include "HashString.h"

class ProgressReporter;


class UnzipEngine : private BCommandPipe::LineReader {
public:
								UnzipEngine(ProgressReporter* reporter,
									sem_id cancelSemaphore = -1);
	virtual						~UnzipEngine();

			status_t			SetTo(const char* pathToPackage,
									const char* destinationFolder);

	inline	off_t				BytesToUncompress() const
									{ return fBytesToUncompress; }
	inline	uint64				ItemsToUncompress() const
									{ return fItemsToUncompress; }

			status_t			UnzipPackage();

private:
	// BCommandPipe::LineReader
			friend class BCommandPipe;

	virtual	bool				IsCanceled();
	virtual	status_t			ReadLine(const BString& line);

			status_t			_ReadLineListing(const BString& line);
			status_t			_ReadLineExtract(const BString& line);

			void				_UpdateProgress(const char* item,
									const char* targetFolder);

private:
			BString				fPackage;
			BString				fDestinationFolder;
			bool				fRetrievingListing;

			typedef HashMap<HashString, off_t> EntrySizeMap;
			EntrySizeMap		fEntrySizeMap;

			off_t				fBytesToUncompress;
			off_t				fBytesUncompressed;
			off_t				fLastBytesUncompressed;
			uint64				fItemsToUncompress;
			uint64				fItemsUncompressed;
			uint64				fLastItemsUncompressed;

			ProgressReporter*	fProgressReporter;
			sem_id				fCancelSemaphore;
};


#endif // UNZIP_ENGINE_H

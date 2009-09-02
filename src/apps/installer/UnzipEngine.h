/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef UNZIP_ENGINE_H
#define UNZIP_ENGINE_H


#include <stdio.h>

#include <Messenger.h>
#include <String.h>

namespace BPrivate {
	class BCommandPipe;
}
class BMessage;
class BMessenger;


class UnzipEngine {
public:
								UnzipEngine(const BMessenger& messenger,
									BMessage* message);
	virtual						~UnzipEngine();

			status_t			SetTo(const char* pathToPackage);

			status_t			UnzipPackage(const char* destinationFolder,
									sem_id cancelSemaphore = -1);

private:
			status_t			_ReadFromPipe(FILE* stdOutAndErrPipe,
									BPrivate::BCommandPipe& commandPipe,
									sem_id cancelSemaphore);
			void				_UpdateProgress();

private:
			BString				fPackage;

			off_t				fBytesToUncompress;
			off_t				fBytesUncompressed;
			uint64				fItemsToUncompress;
			uint64				fItemsUncompressed;

			const char*			fCurrentTargetFolder;
			const char*			fCurrentItem;

			BMessenger			fMessenger;
			BMessage*			fMessage;
};


#endif // UNZIP_ENGINE_H

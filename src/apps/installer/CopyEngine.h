/*
 * Copyright 2008-2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef COPY_ENGINE_H
#define COPY_ENGINE_H


#include <stdlib.h>

#include <Entry.h>
#include <File.h>
#include <Messenger.h>

#include "BlockingQueue.h"

class BFile;
class BLocker;
class BMessage;
class BMessenger;

class CopyEngine {
public:
								CopyEngine(const BMessenger& messenger,
									BMessage* message);
	virtual						~CopyEngine();

			status_t			CopyFolder(const char* source,
									const char* destination,
									BLocker* locker = NULL);

			status_t			CopyFile(const BEntry& entry,
									const BEntry& destination,
									BLocker* locker = NULL);

private:
			status_t			_CollectCopyInfo(const char* source,
									int32& level);
			status_t			_CopyFolder(const char* source,
									const char* destination,
									int32& level,
									BLocker* locker = NULL);

			bool				_ShouldCopyEntry(const char* name,
									const struct stat& statInfo,
									int32 level) const;

			void				_UpdateProgress();

	static	int32				_WriteThreadEntry(void* cookie);
			void				_WriteThread();

private:
			enum {
				BUFFER_COUNT	= 16,
				BUFFER_SIZE		= 1024 * 1024
			};
			struct Buffer {
				Buffer(BFile* file)
					: file(file)
					, buffer(malloc(BUFFER_SIZE))
					, size(BUFFER_SIZE)
					, validBytes(0)
					, deleteFile(false)
				{
				}
				~Buffer()
				{
					if (deleteFile)
						delete file;
					free(buffer);
				}
				BFile*			file;
				void*			buffer;
				size_t			size;
				size_t			validBytes;
				bool			deleteFile;
			};

	BlockingQueue<Buffer>		fBufferQueue;

			thread_id			fWriterThread;
	volatile bool				fQuitting;

			off_t				fBytesRead;
			uint64				fItemsCopied;
			bigtime_t			fTimeRead;

			off_t				fBytesWritten;
			bigtime_t			fTimeWritten;

			off_t				fBytesToCopy;
			uint64				fItemsToCopy;

			const char*			fCurrentTargetFolder;
			const char*			fCurrentItem;

			BMessenger			fMessenger;
			BMessage*			fMessage;
};


#endif // COPY_ENGINE_2_H

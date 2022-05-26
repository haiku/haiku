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
#include <String.h>

#include "BlockingQueue.h"

class BFile;
class ProgressReporter;


class CopyEngine {
public:
			class EntryFilter;

public:
								CopyEngine(ProgressReporter* reporter,
									EntryFilter* entryFilter);
	virtual						~CopyEngine();

			void				ResetTargets(const char* source);
			status_t			CollectTargets(const char* source,
									sem_id cancelSemaphore = -1);

			status_t			Copy(const char* source,
									const char* destination,
									sem_id cancelSemaphore = -1,
									bool copyAttributes = true);

	static	status_t			RemoveFolder(BEntry& entry);

private:
			status_t			_CollectCopyInfo(const char* source,
									sem_id cancelSemaphore, off_t& bytesToCopy,
									uint64& itemsToCopy);
			status_t			_Copy(BEntry& source, BEntry& destination,
									sem_id cancelSemaphore,
									bool copyAttributes);
			status_t			_CopyData(const BEntry& entry,
									const BEntry& destination,
									sem_id cancelSemaphore = -1);

			const char*			_RelativeEntryPath(
									const char* absoluteSourcePath) const;

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
					:
					file(file),
					buffer(malloc(BUFFER_SIZE)),
					size(BUFFER_SIZE),
					validBytes(0),
					deleteFile(false)
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

private:
	BlockingQueue<Buffer>		fBufferQueue;

			thread_id			fWriterThread;
	volatile bool				fQuitting;

			BString				fAbsoluteSourcePath;

			off_t				fBytesRead;
			off_t				fLastBytesRead;
			uint64				fItemsCopied;
			uint64				fLastItemsCopied;
			bigtime_t			fTimeRead;

			off_t				fBytesWritten;
			bigtime_t			fTimeWritten;

			const char*			fCurrentTargetFolder;
			const char*			fCurrentItem;

			ProgressReporter*	fProgressReporter;
			EntryFilter*		fEntryFilter;
};


class CopyEngine::EntryFilter {
public:
	virtual						~EntryFilter();

	virtual	bool				ShouldCopyEntry(const BEntry& entry,
									const char* path,
									const struct stat& statInfo) const = 0;
};


#endif // COPY_ENGINE_H

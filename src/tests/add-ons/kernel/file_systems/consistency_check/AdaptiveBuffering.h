/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ADAPTIVE_BUFFERING_H
#define ADAPTIVE_BUFFERING_H


#include <OS.h>


class AdaptiveBuffering {
public:
							AdaptiveBuffering(size_t initialBufferSize,
								size_t maxBufferSize, uint32 count);
	virtual					~AdaptiveBuffering();

	virtual status_t		Init();

	virtual status_t		Read(uint8* buffer, size_t* _length);
	virtual status_t		Write(uint8* buffer, size_t length);

			status_t		Run();

private:
			void			_QuitWriter();
			status_t		_Writer();
	static	status_t		_Writer(void* self);

			thread_id		fWriterThread;
			uint8**			fBuffers;
			size_t*			fReadBytes;
			uint32			fBufferCount;
			uint32			fReadIndex;
			uint32			fWriteIndex;
			uint32			fReadCount;
			uint32			fWriteCount;
			size_t			fMaxBufferSize;
			size_t			fCurrentBufferSize;
			sem_id			fReadSem;
			sem_id			fWriteSem;
			sem_id			fFinishedSem;
			status_t		fWriteStatus;
			uint32			fWriteTime;
			bool			fFinished;
			bool			fQuit;
};

#endif	// ADAPTIVE_BUFFERING_H

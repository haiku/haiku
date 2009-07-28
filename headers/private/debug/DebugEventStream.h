/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEBUG_EVENT_STREAM_H
#define _DEBUG_EVENT_STREAM_H

#include <SupportDefs.h>


class BDataIO;


struct debug_event_stream_header {
	char	signature[32];
	uint32	version;
	uint32	flags;
	uint32	event_mask;
	uint32	reserved;
};


// signature and version
#define B_DEBUG_EVENT_STREAM_SIGNATURE	"Haiku debug events"
#define B_DEBUG_EVENT_STREAM_VERSION	1


// flags
enum {
	B_DEBUG_EVENT_STREAM_FLAG_HOST_ENDIAN		= 0x00000001,
	B_DEBUG_EVENT_STREAM_FLAG_SWAPPED_ENDIAN	= 0x01000000,

	B_DEBUG_EVENT_STREAM_FLAG_ZIPPED			= 0x00000002
};


class BDebugEventInputStream {
public:
								BDebugEventInputStream();
								~BDebugEventInputStream();

			status_t			SetTo(BDataIO* stream);
			status_t			SetTo(const void* data, size_t size,
									bool takeOverOwnership);
			void				Unset();

			status_t			Seek(off_t streamOffset);

			ssize_t				ReadNextEvent(uint32* _event, uint32* _cpu,
									const void** _buffer,
									off_t* _streamOffset = NULL);

private:
			status_t			_Init();
			ssize_t				_Read(void* buffer, size_t size);
			status_t			_GetData(size_t size);

private:
			BDataIO*			fStream;
			uint32				fFlags;
			uint32				fEventMask;
			uint8*				fBuffer;
			size_t				fBufferCapacity;
			size_t				fBufferSize;
			size_t				fBufferPosition;
			off_t				fStreamPosition;
			bool				fOwnsBuffer;
};


class BDebugEventOutputStream {
public:
								BDebugEventOutputStream();
								~BDebugEventOutputStream();

			status_t			SetTo(BDataIO* stream, uint32 flags,
									uint32 eventMask);
			void				Unset();

			status_t 			Write(const void* buffer, size_t size);
			status_t			Flush();

private:
			BDataIO*			fStream;
			uint32				fFlags;
};


#endif	// _DEBUG_EVENT_STREAM_H

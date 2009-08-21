/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BUFFER_H
#define _BUFFER_H


#include <MediaDefs.h>


struct _shared_buffer_list;


struct buffer_clone_info {
								buffer_clone_info();
								~buffer_clone_info();

			media_buffer_id		buffer;
			area_id				area;
			size_t				offset;
			size_t				size;
			int32				flags;

private:
			uint32				_reserved_[4];
};


class BBuffer {
public:
	// flags
	enum {
		B_F1_BUFFER = 0x1,
		B_F2_BUFFER = 0x2,
		B_SMALL_BUFFER = 0x80000000
	};

			void*				Data();
			size_t				SizeAvailable();
			size_t				SizeUsed();
			void				SetSizeUsed(size_t used);
			uint32				Flags();

			void				Recycle();
			buffer_clone_info	CloneInfo() const;

			media_buffer_id		ID();
			media_type			Type();
			media_header*		Header();
			media_audio_header*	AudioHeader();
			media_video_header*	VideoHeader();

			size_t				Size();

private:
	friend struct _buffer_id_cache;
	friend struct _shared_buffer_list;
	friend class BMediaRoster;
	friend class BBufferProducer;
	friend class BBufferConsumer;
	friend class BBufferGroup;
	friend class BSmallBuffer;

	explicit					BBuffer(const buffer_clone_info& info);
								~BBuffer();

								BBuffer();
								BBuffer(const BBuffer& other);
			BBuffer&			operator=(const BBuffer& other);
									// not implemented

			void				SetHeader(const media_header* header);

			media_header		fMediaHeader;
			_shared_buffer_list* fBufferList;
			area_id				fArea;
			void*				fData;
			size_t				fOffset;
			size_t				fSize;
			media_buffer_id		fBufferID;
			int32				fFlags;

			uint32				_reserved[11];
};


class BSmallBuffer : public BBuffer {
public:
								BSmallBuffer();

	static	size_t				SmallBufferSizeLimit();
};


#endif	// _BUFFER_H

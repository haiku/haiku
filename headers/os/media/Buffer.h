/*******************************************************************************
/
/	File:			Buffer.h
/
/   Description:   A BBuffer is a container of media data in the Media Kit
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_BUFFER_H)
#define _BUFFER_H

#include <MediaDefs.h>


/*** A BBuffer is not the same thing as the area segment that gets cloned ***/
/*** For each buffer that gets created, a BBuffer object is created to represent ***/
/*** it in each participant address space. ***/


struct _shared_buffer_list;

struct buffer_clone_info {
	buffer_clone_info();
	~buffer_clone_info();
	media_buffer_id	buffer;
	area_id		area;
	size_t		offset;
	size_t		size;
	int32		flags;
private:
	uint32 _reserved_[4];
};


class BBuffer
{
public:

		void * Data();	/* returns NULL if buffer not correctly initialized */
		size_t SizeAvailable();	//	total size of buffer (how much data can it hold)
		size_t SizeUsed();		//	how much was written (how much data does it hold)
		void SetSizeUsed(
				size_t size_used);
		uint32 Flags();

		void Recycle();
		buffer_clone_info CloneInfo() const;

		media_buffer_id ID();	/* 0 if not registered */
		media_type Type();
		media_header * Header();
		media_audio_header * AudioHeader();
		media_video_header * VideoHeader();

		enum {	/* for flags */
			B_F1_BUFFER = 0x1,
			B_F2_BUFFER = 0x2,
			B_SMALL_BUFFER = 0x80000000
		};

		size_t Size();			//	deprecated; use SizeAvailable()

private:

		friend struct			_buffer_id_cache;
		friend struct			_shared_buffer_list;
		friend class 			BMediaRoster;
		friend class 			BBufferProducer;
		friend class 			BBufferConsumer;	/* for buffer receiving */
		friend class 			BBufferGroup;
		friend class			BSmallBuffer;

		explicit BBuffer(const buffer_clone_info & info);
		~BBuffer();	/* BBuffer is NOT a virtual class!!! */

		BBuffer(); /* not implemented */
		BBuffer(const BBuffer & clone); /* not implemented */
		BBuffer & operator=(const BBuffer & clone); /* not implemented */

		void					SetHeader(const media_header *header);

		media_header			fMediaHeader;
		_shared_buffer_list *	fBufferList;
		area_id 				fArea;
		void * 					fData;
		size_t 					fOffset;
		size_t 					fSize;
		media_buffer_id 		fBufferID;
		int32 					fFlags;

		uint32 			_reserved_buffer_[11];

};


class BSmallBuffer : public BBuffer
{
public:
							BSmallBuffer();

static	size_t				SmallBufferSizeLimit();

};

#endif /* _BUFFER_H */


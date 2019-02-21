/*
 * Copyright 2019, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DVD_MEDIA_IO_H
#define _DVD_MEDIA_IO_H


#include <AdapterIO.h>
#include <String.h>
#include <Url.h>

#include <dvdnav/dvdnav.h>


using BCodecKit::BAdapterIO;
using BCodecKit::BInputAdapter;


class DVDMediaIO : public BAdapterIO
{
public:
										DVDMediaIO(const char* path);
	virtual								~DVDMediaIO();

	virtual	ssize_t						WriteAt(off_t position,
											const void* buffer,
											size_t size);

	virtual	status_t					SetSize(off_t size);

	virtual status_t					Open();

			void						LoopThread();

	virtual	status_t					SeekRequested(off_t position);

			void						HandleDVDEvent(int event, int len);

			void						MouseMoved(uint32 x, uint32 y);
			void						MouseDown(uint32 x, uint32 y);

private:
			static int32				_LoopThread(void* data);

			thread_id					fLoopThread;
			bool						fExit;

			BInputAdapter*				fInputAdapter;

			const char*					fPath;
			dvdnav_t*					fDvdNav;
			uint8_t*					fBuffer;
};


#endif

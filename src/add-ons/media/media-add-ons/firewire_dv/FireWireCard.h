/*
 * FireWire DV media addon for Haiku
 *
 * Copyright (c) 2008, JiSheng Zhang (jszhang3@mail.ustc.edu.cn)
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef __FIREWIRE_CARD_H
#define __FIREWIRE_CARD_H


#include <SupportDefs.h>


class FireWireCard {
public:
	enum fmt_type {
		FMT_MPEGTS = 0,
		FMT_DV = 1,
	};
			FireWireCard(const char* path);
			~FireWireCard();

	status_t	InitCheck();

	status_t	DetectRecvFn();
	ssize_t		Read(void** buffer);
	status_t	Extract(void* dest, void** src, ssize_t* sizeUsed);

	void		GetBufInfo(size_t* rbufsize, int* rcount);

private:
	ssize_t		DvRead(void** buffer);
	status_t	DvExtract(void* dest, void** src, ssize_t* sizeUsed);
	ssize_t		MpegtsRead(void** buffer);
	status_t	MpegtsExtract(void* dest, void** src,
				ssize_t* sizeUsed);

	status_t	fInitStatus;
	int		fDev;
	void*		fBuf;
	void*		fPad;
	size_t		fRbufSize;
	int		fRcount;
	fmt_type	fFormat;

};

#endif

/*
 * FireWire DV media addon for Haiku
 *
 * Copyright (c) 2008, JiSheng Zhang (jszhang3@mail.ustc.edu.cn)
 * Distributed under the terms of the MIT License.
 *
 */
/*
 * Copyright (C) 2003
 * 	Hidetoshi Shimokawa. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *	This product includes software developed by Hidetoshi Shimokawa.
 *
 * 4. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "FireWireCard.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <OS.h>
#include <stdint.h>
#include <errno.h>

#include "glue.h"

#define TAG	(1<<6)
#define CHANNEL	63

/* for DV format */
#define FIX_FRAME	1

struct frac {
	int n,d;
};

struct frac frame_cycle[2]  = {
	{8000*100, 2997},	/* NTSC 8000 cycle / 29.97 Hz */
	{320, 1},		/* PAL  8000 cycle / 25 Hz */
};
int npackets[] = {
	250		/* NTSC */,
	300		/* PAL */
};
struct frac pad_rate[2]  = {
	{203, 2997},	/* = (8000 - 29.97 * 250)/(29.97 * 250) */
	{1, 15},	/* = (8000 - 25 * 300)/(25 * 300) */
};
const char *system_name[] = {"NTSC", "PAL"};
int frame_rate[] = {30, 25};

#define DV_PSIZE 512
#define DV_DSIZE 480
#define DV_NCHUNK 64

#define DV_NPACKET_R 256
#define DV_NPACKET_T 255
#define DV_TNBUF 100	/* XXX too large value causes block noise */
#define DV_NEMPTY 10	/* depends on DV_TNBUF */
#define DV_RBUFSIZE (DV_PSIZE * DV_NPACKET_R)
#define DV_MAXBLOCKS (300)
#define DV_CYCLE_FRAC 0xc00

/* for MPEGTS format */
typedef uint8_t mpeg_ts_pld[188];

struct mpeg_pldt {
#if BYTE_ORDER == BIG_ENDIAN
	uint32_t	:7,
				c_count:13,
				c_offset:12;
#else /* BYTE_ORDER != BIG_ENDIAN */
	uint32_t	c_offset:12,
				c_count:13,
				:7;
#endif /* BYTE_ORDER == BIG_ENDIAN */
	mpeg_ts_pld payload;
};


#define	MPEG_NCHUNK 8
#define	MPEG_PSIZE 596
#define	MPEG_NPACKET_R 4096
#define	MPEG_RBUFSIZE (MPEG_PSIZE * MPEG_NPACKET_R)


FireWireCard::FireWireCard(const char* path)
	: fInitStatus(B_OK),
	fDev(-1),
	fBuf(NULL),
	fPad(NULL)
{
	printf("FireWireCard opening %s\n", path);

	fDev = open(path, O_RDWR);
	if (fDev < 0) {
		printf("FireWireCard opening %s failed\n", path);
		fInitStatus = B_ERROR;
		return;
	}
}


FireWireCard::~FireWireCard()
{
	if (fDev > 0)
		close(fDev);
}


status_t
FireWireCard::InitCheck()
{
	return fInitStatus;
}


ssize_t
FireWireCard::Read(void** data)
{
	if (fFormat == FMT_MPEGTS)
		return MpegtsRead(data);
	else
		return DvRead(data);
}


status_t
FireWireCard::Extract(void* dest, void** src, ssize_t* sizeUsed)
{
	if (fFormat == FMT_MPEGTS)
		return MpegtsExtract(dest, src, sizeUsed);
	else
		return DvExtract(dest, src, sizeUsed);
}


void
FireWireCard::GetBufInfo(size_t* rbufsize, int* rcount)
{
	*rbufsize = fRbufSize;
	*rcount = fRcount;
}


status_t
FireWireCard::DetectRecvFn()
{
	char* buf;
	char ich = TAG | CHANNEL;
	struct fw_isochreq isoreq;
	struct fw_isobufreq bufreq;
	int len;
	u_int32_t* ptr;
	struct ciphdr* ciph;

	bufreq.rx.nchunk = 8;
	bufreq.rx.npacket = 16;
	bufreq.rx.psize = 1024;
	bufreq.tx.nchunk = 0;
	bufreq.tx.npacket = 0;
	bufreq.tx.psize = 0;

	if (ioctl(fDev, FW_SSTBUF, &bufreq) < 0)
		return errno;

	isoreq.ch = ich & 0x3f;
	isoreq.tag = (ich >> 6) & 3;

	if (ioctl(fDev, FW_SRSTREAM, &isoreq) < 0)
		return errno;

	buf = (char*)malloc(1024*16);
	len = read(fDev, buf, 1024*16);
	if (len < 0) {
		free(buf);
		return errno;
	}
	ptr = (u_int32_t*) buf;
	ciph = (struct ciphdr*)(ptr + 1);

	switch(ciph->fmt) {
		case CIP_FMT_DVCR:
			fprintf(stderr, "Detected DV format on input.\n");
			fFormat = FMT_DV;
			fBuf = malloc(DV_RBUFSIZE);
			fRbufSize = DV_PSIZE;
			fRcount = DV_NPACKET_R;
			fPad = malloc(DV_DSIZE*DV_MAXBLOCKS);
			memset(fPad, 0xff, DV_DSIZE*DV_MAXBLOCKS);
			break;
		case CIP_FMT_MPEG:
			fprintf(stderr, "Detected MPEG TS format on input.\n");
			fFormat = FMT_MPEGTS;
			fBuf = malloc(MPEG_RBUFSIZE);
			fRbufSize = MPEG_PSIZE;
			fRcount = MPEG_NPACKET_R;
			break;
		default:
			fprintf(stderr, "Unsupported format for receiving: fmt=0x%x", ciph->fmt);
	}
	free(buf);
	return B_OK;
}


ssize_t
FireWireCard::DvRead(void** buffer)
{
	struct fw_isochreq isoreq;
	struct fw_isobufreq bufreq;
	ssize_t len;
	char ich = TAG|CHANNEL;

	bufreq.rx.nchunk = DV_NCHUNK;
	bufreq.rx.npacket = DV_NPACKET_R;
	bufreq.rx.psize = DV_PSIZE;
	bufreq.tx.nchunk = 0;
	bufreq.tx.npacket = 0;
	bufreq.tx.psize = 0;
	if (ioctl(fDev, FW_SSTBUF, &bufreq) < 0)
		return errno;

	isoreq.ch = ich & 0x3f;
	isoreq.tag = (ich >> 6) & 3;

	if (ioctl(fDev, FW_SRSTREAM, &isoreq) < 0)
		return errno;

	len = read(fDev, fBuf, DV_RBUFSIZE);
	if (len < 0) {
		if (errno == EAGAIN) {
			fprintf(stderr, "(EAGAIN) - push 'Play'?\n");
			fflush(stderr);
		} else
			fprintf(stderr, "read failed");
		return errno;
	}
	*buffer = fBuf;
	return len;
}


status_t
FireWireCard::DvExtract(void* dest, void** src, ssize_t* sizeUsed)
{
	struct dvdbc* dv;
	struct ciphdr* ciph;
	struct fw_pkt* pkt;
	u_int32_t* ptr;
	int nblocks[] = {250 /* NTSC */, 300 /* PAL */};
	int npad, k, m, system = -1, nb;

	k = m = 0;
	ptr = (u_int32_t*) (*src);

	pkt = (struct fw_pkt*) ptr;
	ciph = (struct ciphdr*)(ptr + 1);	/* skip iso header */
	if (ciph->fmt != CIP_FMT_DVCR) {
		fprintf(stderr, "unknown format 0x%x", ciph->fmt);
		return B_ERROR;
	}
	ptr = (u_int32_t*) (ciph + 1);		/* skip cip header */
	if (pkt->mode.stream.len <= sizeof(struct ciphdr))
		/* no payload */
		return B_ERROR;
	for (dv = (struct dvdbc*)ptr;
			(char*)dv < (char *)(ptr + ciph->len);
			dv+=6) {

		if  (dv->sct == DV_SCT_HEADER && dv->dseq == 0) {
			if (system < 0) {
				system = ciph->fdf.dv.fs;
				fprintf(stderr, "%s\n", system_name[system]);
			}

			/* Fix DSF bit */
			if (system == 1 &&
				(dv->payload[0] & DV_DSF_12) == 0)
				dv->payload[0] |= DV_DSF_12;
			nb = nblocks[system];
			fprintf(stderr, "%d", k%10);
#if FIX_FRAME
			if (m > 0 && m != nb) {
				/* padding bad frame */
				npad = ((nb - m) % nb);
				if (npad < 0)
					npad += nb;
				fprintf(stderr, "(%d blocks padded)",
							npad);
				npad *= DV_DSIZE;
				memcpy(dest, fPad, npad);
				dest = (char*)dest + npad;
			}
#endif
			k++;
			if (k % frame_rate[system] == 0) {
				/* every second */
				fprintf(stderr, "\n");
			}
			fflush(stderr);
			m = 0;
		}
		if (k == 0) 
			continue;
		m++;
		memcpy(dest, dv, DV_DSIZE);
		dest = (char*)dest + DV_DSIZE;
	}
	ptr = (u_int32_t*)dv;
	*src = ptr;
	return B_OK;
}


ssize_t
FireWireCard::MpegtsRead(void** buffer)
{
	struct fw_isochreq isoreq;
	struct fw_isobufreq bufreq;
	char ich = TAG|CHANNEL;
	ssize_t len;


	bufreq.rx.nchunk = MPEG_NCHUNK;
	bufreq.rx.npacket = MPEG_NPACKET_R;
	bufreq.rx.psize = MPEG_PSIZE;
	bufreq.tx.nchunk = 0;
	bufreq.tx.npacket = 0;
	bufreq.tx.psize = 0;
	if (ioctl(fDev, FW_SSTBUF, &bufreq) < 0)
		return errno;

	isoreq.ch = ich & 0x3f;
	isoreq.tag = (ich >> 6) & 3;

	if (ioctl(fDev, FW_SRSTREAM, &isoreq) < 0)
		return errno;

	len = read(fDev, fBuf, MPEG_RBUFSIZE);
	if (len < 0) {
		if (errno == EAGAIN) {
			fprintf(stderr, "(EAGAIN) - push 'Play'?\n");
			fflush(stderr);
		} else
			fprintf(stderr, "read failed");
		return errno;
	}
	*buffer = fBuf;
	return len;
}
	

status_t
FireWireCard::MpegtsExtract(void* dest, void** src, ssize_t* sizeUsed)
{
	uint32_t* ptr;
	struct fw_pkt* pkt;
	struct ciphdr* ciph;
	struct mpeg_pldt* pld;
	int pkt_size, startwr;

	ptr = (uint32_t *)(*src);
	startwr = 0;

	pkt = (struct fw_pkt*) ptr;	
	/* there is no CRC in the 1394 header */
	ciph = (struct ciphdr*)(ptr + 1);	/* skip iso header */
	if (ciph->fmt != CIP_FMT_MPEG) {
		fprintf(stderr, "unknown format 0x%x", ciph->fmt);
		return B_ERROR;
	}
	if (ciph->fn != 3) {
		fprintf(stderr,	"unsupported MPEG TS stream, fn=%d (only" 
			"fn=3 is supported)", ciph->fn);
		return B_ERROR;
	}
	ptr = (uint32_t*) (ciph + 1);		/* skip cip header */

	if (pkt->mode.stream.len <= sizeof(struct ciphdr)) {
		/* no payload */
		/* tlen needs to be decremented before end of the loop */
		goto next;
	}

	/* This is a condition that needs to be satisfied to start
	   writing the data */
	if (ciph->dbc % (1<<ciph->fn) == 0)
		startwr = 1;
	/* Read out all the MPEG TS data blocks from current packet */
	for (pld = (struct mpeg_pldt *)ptr;
	    (intptr_t)pld < (intptr_t)((char*)ptr +
	    pkt->mode.stream.len - sizeof(struct ciphdr));
	    pld++) {
		if (startwr == 1) {
			memcpy(dest, pld->payload, sizeof(pld->payload));
			dest = (char*)dest + sizeof(pld->payload);
		}
	}

next:
	/* CRCs are removed from both header and trailer
	so that only 4 bytes of 1394 header remains */
	pkt_size = pkt->mode.stream.len + 4; 
	ptr = (uint32_t*)((intptr_t)pkt + pkt_size);
	*src = ptr;
	return B_OK;
}


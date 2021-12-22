/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

/*
 * stream based deframer
 * has a state machine and handles each packet separately.
 * much more complex than the buffering one, and I thought it didn't work,
 * but since I fixed the rest it seems to be working even better without
 * taking the cpu over like the other one.
 */

#define CD_COL "31"
#include "CamStreamingDeframer.h"
#include "CamDevice.h"
#include "CamDebug.h"
#include <Autolock.h>
#define MAX_TAG_LEN CAMDEFRAMER_MAX_TAG_LEN
#define MAXFRAMEBUF CAMDEFRAMER_MAX_QUEUED_FRAMES


CamStreamingDeframer::CamStreamingDeframer(CamDevice *device)
	: CamDeframer(device)
{
}


CamStreamingDeframer::~CamStreamingDeframer()
{
}


ssize_t
CamStreamingDeframer::Write(const void *buffer, size_t size)
{
	int i = -1;
	int j;
	int end = size;
	int which;
	const uint8 *buf = (const uint8 *)buffer;
	int bufsize = size;
	bool detach = false;
	bool discard = false;
	//PRINT((CH "(%p, %d); state=%s framesz=%u queued=%u" CT, buffer, size, (fState==ST_SYNC)?"sync":"frame", (size_t)(fCurrentFrame?(fCurrentFrame->Position()):-1), (size_t)fInputBuff.Position()));
	if (!fCurrentFrame) {
		BAutolock l(fLocker);
		if (fFrames.CountItems() < MAXFRAMEBUF)
			fCurrentFrame = AllocFrame();
		else {
			PRINT((CH "DROPPED %" B_PRIuSIZE " bytes! "
				"(too many queued frames)" CT, size));
			return size; // drop XXX
		}
	}

	// update in case resolution changed
	fMinFrameSize = fDevice->MinRawFrameSize();
	fMaxFrameSize = fDevice->MaxRawFrameSize();

	if (fInputBuff.Position()) {
		// residual data ? append to it
		fInputBuff.Write(buffer, size);
		// and use it as input buf
		buf = (uint8 *)fInputBuff.Buffer();
		bufsize = fInputBuff.BufferLength();
		end = bufsize;
	}
	// whole buffer belongs to a frame, simple
	if (fState == ST_FRAME) {
		off_t position = fCurrentFrame->Position();
		if (position + bufsize < 0
			|| (size_t)(position + bufsize) < fMinFrameSize) {
			// no residual data, and
			fCurrentFrame->Write(buf, bufsize);
			fInputBuff.Seek(0LL, SEEK_SET);
			fInputBuff.SetSize(0);
			return size;
		}
	}

	// waiting for a frame...
	if (fState == ST_SYNC) {
		i = 0;
		while ((j = FindSOF(buf+i, bufsize-i, &which)) > -1) {
			i += j;
			if (fDevice->ValidateStartOfFrameTag(buf+i, fSkipSOFTags))
				break;
			i++;
		}
		// got one
		if (j >= 0) {
			PRINT((CH ": SOF[%d] at offset %d" CT, which, i));
			//PRINT((CH ": SOF: ... %02x %02x %02x %02x %02x %02x" CT, buf[i+6], buf[i+7], buf[i+8], buf[i+9], buf[i+10], buf[i+11]));
			int start = i + fSkipSOFTags;
			buf += start;
			bufsize -= start;
			end = bufsize;
			fState = ST_FRAME;
		}
	}

	// check for end of frame
	if (fState == ST_FRAME) {
#if 0
		int j, k;
		i = -1;
		k = 0;
		while ((j = FindEOF(buf + k, bufsize - k, &which)) > -1) {
			k += j;
			//PRINT((CH "| EOF[%d] at offset %d; pos %Ld" CT, which, k, fCurrentFrame->Position()));
			if (fCurrentFrame->Position()+k >= fMinFrameSize) {
				i = k;
				break;
			}
			k++;
			if (k >= bufsize)
				break;
		}
#endif
#if 1
		i = 0;
		off_t currentFramePosition = fCurrentFrame->Position();
		if (currentFramePosition < 0
			|| (size_t)currentFramePosition < fMinFrameSize) {
			if (currentFramePosition + bufsize > 0
				&& (size_t)(currentFramePosition + bufsize) >= fMinFrameSize) {
				i = (fMinFrameSize - (size_t)fCurrentFrame->Position());
			} else
				i = bufsize;
		}
		PRINT((CH ": checking for EOF; bufsize=%d i=%d" CT, bufsize, i));

		if (i + (int)fSkipEOFTags > bufsize) { // not enough room to check for EOF, leave it for next time
			end = i;
			i = -1; // don't detach yet
		} else {
			PRINT((CH ": EOF? %02x [%02x %02x %02x %02x] %02x" CT, buf[i-1], buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4]));
			while ((j = FindEOF(buf + i, bufsize - i, &which)) > -1) {
				i += j;
				PRINT((CH "| EOF[%d] at offset %d; pos %" B_PRIdOFF CT,
					which, i, fCurrentFrame->Position()));
				off_t position = fCurrentFrame->Position();
				if (position + i >= 0
					&& (size_t)(position + i) >= fMaxFrameSize) {
					// too big: discard
					//i = -1;
					discard = true;
					break;
				}
				if (fDevice->ValidateEndOfFrameTag(buf+i, fSkipEOFTags, fCurrentFrame->Position()+i))
					break;
				i++;
				if (i >= bufsize) {
					i = -1;
					break;
				}
			}
			if (j < 0)
				i = -1;
		}
#endif
		if (i >= 0) {
			PRINT((CH ": EOF[%d] at offset %d" CT, which, i));
			end = i;
			detach = true;
		}
		PRINT((CH ": writing %d bytes" CT, end));
		if (end <= bufsize)
			fCurrentFrame->Write(buf, end);
		off_t currentPosition = fCurrentFrame->Position();
		if (currentPosition > 0
			&& (size_t)currentPosition > fMaxFrameSize) {
			fCurrentFrame->SetSize(fMaxFrameSize);
			detach = true;
		}
		if (detach) {
			BAutolock f(fLocker);
			PRINT((CH ": Detaching a frame "
				"(%" B_PRIuSIZE " bytes, end = %d, )" CT,
				(size_t)fCurrentFrame->Position(), end));
			fCurrentFrame->Seek(0LL, SEEK_SET);
			if (discard) {
				delete fCurrentFrame;
			} else {
				fFrames.AddItem(fCurrentFrame);
				release_sem(fFrameSem);
			}
			fCurrentFrame = NULL;
			if (fFrames.CountItems() < MAXFRAMEBUF) {
				fCurrentFrame = AllocFrame();
			}
			fState = ST_SYNC;
		}
	}




	// put the remainder in input buff, discarding old data
#if 0
	fInputBuff.Seek(0LL, SEEK_SET);
	if (bufsize - end > 0)
		fInputBuff.Write(buf+end, bufsize - end);
#endif
	BMallocIO m;
	m.Write(buf+end, bufsize - end);
	fInputBuff.Seek(0LL, SEEK_SET);
	if (bufsize - end > 0)
		fInputBuff.Write(m.Buffer(), bufsize - end);
	fInputBuff.SetSize(bufsize - end);
	return size;
}

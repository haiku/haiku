/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

/*
 * buffer based deframer
 * buffers all packet until it finds a complete frame.
 * simpler than StreamingDeframer, but doesn't work any better
 * and hogs the cpu intermitently :^)
 */

#define CD_COL "31"
#include "CamBufferingDeframer.h"
#include "CamDevice.h"
#include "CamDebug.h"
#include <Autolock.h>
#define MAX_TAG_LEN CAMDEFRAMER_MAX_TAG_LEN
#define MAXFRAMEBUF CAMDEFRAMER_MAX_QUEUED_FRAMES

#define IB fInputBuffs[fInputBuffIndex]


CamBufferingDeframer::CamBufferingDeframer(CamDevice *device)
	: CamDeframer(device),
	fInputBuffIndex(0)
{
}


CamBufferingDeframer::~CamBufferingDeframer()
{
}


ssize_t
CamBufferingDeframer::Write(const void *buffer, size_t size)
{
	uint8 *b;
	int l;
	int i, s, e;
	int which;
	fMinFrameSize = fDevice->MinRawFrameSize();
	fMaxFrameSize = fDevice->MaxRawFrameSize();
	IB.Write(buffer, size);
	b = (uint8 *)IB.Buffer();
	l = IB.BufferLength();

	PRINT((CH "(%p, %" B_PRIuSIZE "), IB: %" B_PRIuSIZE CT, buffer, size,
		IB.BufferLength()));

	if (l < (int)(fMinFrameSize + fSkipSOFTags + fSkipEOFTags))
		return size; // not enough data anyway

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

	for (s = 0; (l - s > (int)fMinFrameSize) && ((i = FindSOF(b + s, l - fMinFrameSize - s, &which)) > -1); s++) {
		s += i;
		if ((int)(s + fSkipSOFTags + fMinFrameSize + fSkipEOFTags) > l)
			break;
		if (!fDevice->ValidateStartOfFrameTag(b + s, fSkipSOFTags))
			continue;

		PRINT((CH ": SOF[%d] at offset %d" CT, which, s));
		PRINT((CH ": SOF: ... %02x %02x %02x %02x %02x %02x" CT, b[s+6], b[s+7], b[s+8], b[s+9], b[s+10], b[s+11]));

		for (e = s + fSkipSOFTags + fMinFrameSize;
			 ((e <= (int)(s + fSkipSOFTags + fMaxFrameSize)) &&
			  (e < l) && ((i = 0*FindEOF(b + e, l - e, &which)) > -1));
			 e++) {
			e += i;

			//PRINT((CH ": EOF[%d] at offset %d" CT, which, s));
			if (!fDevice->ValidateEndOfFrameTag(b + e, fSkipEOFTags, e - s - fSkipSOFTags))
				continue;



		PRINT((CH ": SOF= ... %02x %02x %02x %02x %02x %02x" CT, b[s+6], b[s+7], b[s+8], b[s+9], b[s+10], b[s+11]));

			// we have one!
			s += fSkipSOFTags;

			// fill it
			fCurrentFrame->Write(b + s, e - s);

			// queue it
			BAutolock f(fLocker);
			PRINT((CH ": Detaching a frame (%" B_PRIuSIZE " bytes, "
				"%d to %d / %d)" CT, (size_t)fCurrentFrame->Position(),
				s, e, l));
			fCurrentFrame->Seek(0LL, SEEK_SET);
			fFrames.AddItem(fCurrentFrame);
			release_sem(fFrameSem);
			// next Write() will allocate a new one
			fCurrentFrame = NULL;
			// discard the frame and everything before it.
			DiscardFromInput(e + fSkipEOFTags);

			return size;
		}
	}
	return size;
}


size_t
CamBufferingDeframer::DiscardFromInput(size_t size)
{
	int next = (fInputBuffIndex+1)%2;
	PRINT((CH ": %" B_PRIuSIZE " bytes of %" B_PRIuSIZE " from buffs[%d] "
		"(%" B_PRIuSIZE " left)" CT,
		size, IB.BufferLength(), fInputBuffIndex, IB.BufferLength() - size));
	fInputBuffs[next].Seek(0LL, SEEK_SET);
	fInputBuffs[next].SetSize(0);
	uint8 *buff = (uint8 *)IB.Buffer();
	if (IB.BufferLength() > size) {
		buff += size;
		fInputBuffs[next].Write(buff, IB.BufferLength() - size);
	}
	IB.Seek(0LL, SEEK_SET);
	IB.SetSize(0);
	fInputBuffIndex = next;
	return size;
}

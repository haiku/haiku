/*
 * Copyright 2011, Gabriel Hartmann, gabriel.hartmann@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "UVCDeframer.h"

#include "CamDebug.h"
#include "CamDevice.h"

#include <Autolock.h>


#define MAX_TAG_LEN CAMDEFRAMER_MAX_TAG_LEN
#define MAXFRAMEBUF CAMDEFRAMER_MAX_QUEUED_FRAMES


UVCDeframer::UVCDeframer(CamDevice* device)
	: CamDeframer(device),
	fFrameCount(0),
	fID(0)
{
}


UVCDeframer::~UVCDeframer()
{
}


ssize_t
UVCDeframer::Write(const void* buffer, size_t size)
{
	const uint8* buf = (const uint8*)buffer;
	int payloadSize = size - buf[0]; // total length - header length
	
	// This packet is just a header
	if (size == buf[0])
		return 0;
	
	// Allocate frame
	if (!fCurrentFrame) {
		BAutolock l(fLocker);
		if (fFrames.CountItems() < MAXFRAMEBUF)
			fCurrentFrame = AllocFrame();
		else {
			printf("Dropped %ld bytes. Too many queued frames.)\n", size);
			return size;
		}
	}
	
	// Write payload to buffer
	fInputBuffer.Write(&buf[buf[0]], payloadSize);
	
	// If end of frame add frame to list of frames
	if ((buf[1] & 2) || (buf[1] & 1) != fID) {
		fID = buf[1] & 1;
		fFrameCount++;
		buf = (uint8*)fInputBuffer.Buffer();
		fCurrentFrame->Write(buf, fInputBuffer.BufferLength());
		fFrames.AddItem(fCurrentFrame);
		release_sem(fFrameSem);
		fCurrentFrame = NULL;
	}

	return size;
}


void
UVCDeframer::_PrintBuffer(const void* buffer, size_t size)
{
	uint8* b = (uint8*)buffer;
	for (size_t i = 0; i < size; i++)
		printf("0x%x\t", b[i]);
	printf("\n");
}


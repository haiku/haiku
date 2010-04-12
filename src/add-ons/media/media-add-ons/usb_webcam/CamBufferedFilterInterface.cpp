/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamBufferedFilterInterface.h"
#include "CamDevice.h"
#include "CamDebug.h"


CamBufferedFilterInterface::CamBufferedFilterInterface(CamDevice *device, bool allowWrite)
	: CamFilterInterface(device),
	fAllowWrite(allowWrite)
{
}


CamBufferedFilterInterface::~CamBufferedFilterInterface()
{
}


ssize_t
CamBufferedFilterInterface::Read(void *buffer, size_t size)
{
	return fInternalBuffer.Read(buffer, size);
}


ssize_t
CamBufferedFilterInterface::ReadAt(off_t pos, void *buffer, size_t size)
{
	return fInternalBuffer.ReadAt(pos, buffer, size);
}


ssize_t
CamBufferedFilterInterface::Write(const void *buffer, size_t size)
{
	if (!fAllowWrite)
		return B_READ_ONLY_DEVICE;
	return fInternalBuffer.Write(buffer, size);
}


ssize_t
CamBufferedFilterInterface::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (!fAllowWrite)
		return B_READ_ONLY_DEVICE;
	return fInternalBuffer.WriteAt(pos, buffer, size);
}


off_t
CamBufferedFilterInterface::Seek(off_t position, uint32 seek_mode)
{
	return fInternalBuffer.Seek(position, seek_mode);
}


off_t
CamBufferedFilterInterface::Position() const
{
	return fInternalBuffer.Position();
}


status_t
CamBufferedFilterInterface::SetSize(off_t size)
{
	if (!fAllowWrite)
		return B_READ_ONLY_DEVICE;
	return fInternalBuffer.SetSize(size);
}


size_t
CamBufferedFilterInterface::FrameSize()
{
	return fInternalBuffer.BufferLength(); // XXX: really ??
	return 0;
}


status_t
CamBufferedFilterInterface::DropFrame()
{
	fInternalBuffer.SetSize(0LL);
	if (fNextOfKin)
		return fNextOfKin->DropFrame();
	return B_OK;
}


status_t
CamBufferedFilterInterface::SetVideoFrame(BRect frame)
{
	fVideoFrame = frame;
	fInternalBuffer.SetSize(FrameSize()); // XXX: really ??
	if (fNextOfKin)
		return fNextOfKin->SetVideoFrame(frame);
	return B_OK;
}

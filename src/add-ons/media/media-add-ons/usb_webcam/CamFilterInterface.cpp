/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamFilterInterface.h"
#include "CamDevice.h"
#include "CamDebug.h"


CamFilterInterface::CamFilterInterface(CamDevice *device)
	: BPositionIO(),
	fDevice(device),
	fNextOfKin(NULL)
{
	fVideoFrame = BRect(0,0,-1,-1);

}


CamFilterInterface::~CamFilterInterface()
{
}


status_t
CamFilterInterface::ChainFilter(CamFilterInterface *to)
{
	if (fNextOfKin)
		return EALREADY;
	fNextOfKin = to;
	return B_OK;
}


status_t
CamFilterInterface::DetachFilter(CamFilterInterface *from)
{
	if (from && (fNextOfKin != from))
		return EINVAL;
	fNextOfKin = NULL;
	return B_OK;
}


CamFilterInterface*
CamFilterInterface::ChainFilter()
{
	return fNextOfKin;
}


ssize_t
CamFilterInterface::Read(void *buffer, size_t size)
{
	(void)buffer;
	(void)size;
	return EINVAL;
}


ssize_t
CamFilterInterface::ReadAt(off_t pos, void *buffer, size_t size)
{
	(void)pos;
	(void)buffer;
	(void)size;
	return EINVAL;
}


ssize_t
CamFilterInterface::Write(const void *buffer, size_t size)
{
	(void)buffer;
	(void)size;
	return EINVAL;
}


ssize_t
CamFilterInterface::WriteAt(off_t pos, const void *buffer, size_t size)
{
	(void)pos;
	(void)buffer;
	(void)size;
	return EINVAL;
}


off_t
CamFilterInterface::Seek(off_t position, uint32 seek_mode)
{
	(void)position;
	(void)seek_mode;
	return EINVAL;
}


off_t
CamFilterInterface::Position() const
{
	return 0LL;
}


status_t
CamFilterInterface::SetSize(off_t size)
{
	(void)size;
	return EINVAL;
}


size_t
CamFilterInterface::FrameSize()
{
	return 0;
}


status_t
CamFilterInterface::WaitFrame(bigtime_t timeout)
{
	if (fNextOfKin)
		return fNextOfKin->WaitFrame(timeout);
	return EINVAL;
}


status_t
CamFilterInterface::DropFrame()
{
	if (fNextOfKin)
		return fNextOfKin->DropFrame();
	return B_OK;
}


status_t
CamFilterInterface::SetVideoFrame(BRect frame)
{
	if (fNextOfKin)
		return fNextOfKin->SetVideoFrame(frame);
	fVideoFrame = frame;
	return B_OK;
}

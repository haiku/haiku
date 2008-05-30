/*
 * Copyright 2000-2008 Ingo Weinhold <ingo_weinhold@gmx.de> All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "VideoTarget.h"


VideoTarget::VideoTarget()
	: fBitmapLock(),
	  fBitmap(NULL)
{
}


VideoTarget::~VideoTarget()
{
}


bool
VideoTarget::LockBitmap()
{
	return fBitmapLock.Lock();
}


void
VideoTarget::UnlockBitmap()
{
	fBitmapLock.Unlock();
}


void
VideoTarget::SetBitmap(const BBitmap* bitmap)
{
	LockBitmap();
	fBitmap = bitmap;
	UnlockBitmap();
}


const BBitmap*
VideoTarget::GetBitmap() const
{
	return fBitmap;
}


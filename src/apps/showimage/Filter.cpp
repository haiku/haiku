/*****************************************************************************/
// Filter
// Written by Michael Pfeiffer
//
// Filter.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <scheduler.h>
#include "Filter.h"
#include "Scale.h"

Filter::Filter(BBitmap* image, BMessenger listener, uint32 what)
	: fListener(listener)
	, fWhat(what)
	, fIsRunning(false)
	, fWorkerThread(-1)
	, fSrcImage(image)
	, fDestImage(NULL)
{
}

Filter::~Filter()
{
	delete fDestImage;
}

BBitmap*
Filter::GetBitmap()
{
	if (fDestImage == NULL) {
		fDestImage = CreateDestImage(fSrcImage);
	}
	return fDestImage;
}

status_t 
Filter::worker_thread(void* data)
{
	Filter* filter = (Filter*)data;
	filter->Run();
	if (filter->fIsRunning) {
		filter->fListener.SendMessage(filter->fWhat);
		filter->fIsRunning = false;
	}
	return B_OK;
}

void
Filter::Start()
{
	GetBitmap();
	if (fSrcImage == NULL || fDestImage == NULL) return;
	fWorkerThread = spawn_thread(worker_thread, "filter", suggest_thread_priority(B_STATUS_RENDERING), this);
	if (fWorkerThread >= 0) {
		fIsRunning = true;
		resume_thread(fWorkerThread);
	}	
}

void
Filter::Stop()
{
	if (fIsRunning) {
		fIsRunning = false;
		status_t st;
		wait_for_thread(fWorkerThread, &st);
	}
}

bool
Filter::IsRunning() const
{
	return fIsRunning;
}

BBitmap*
Filter::GetSrcImage()
{
	return fSrcImage;
}

BBitmap*
Filter::GetDestImage()
{
	return fDestImage;
}

Scaler::Scaler(BBitmap* image, float scale, BMessenger listener, uint32 what)
	: Filter(image, listener, what)
	, fScale(scale)
{
}

Scaler::~Scaler()
{
}

BBitmap*
Scaler::CreateDestImage(BBitmap* srcImage)
{
	if (srcImage == NULL || srcImage->ColorSpace() != B_RGB32 && srcImage->ColorSpace() !=B_RGBA32) return NULL;
	BRect src(srcImage->Bounds());
	BRect dest(0, 0, (src.Width()+1)*fScale - 1, (src.Height()+1)*fScale - 1);
	BBitmap* destImage = new BBitmap(dest, srcImage->ColorSpace());
	return destImage;
}

void
Scaler::Run()
{
	scale(GetSrcImage(), GetDestImage(), IsRunningAddr(), fScale, fScale);
}

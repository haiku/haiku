/*****************************************************************************/
// Filter
// Written by Michael Pfeiffer
//
// Filter.h
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

#ifndef _Filter_h
#define _Filter_h

#include <OS.h>
#include <Bitmap.h>
#include <Messenger.h>
#include <StopWatch.h>

#define TIME_FILTER 0

class Filter;

// Used by class Filter 
class FilterThread {
public:
	FilterThread(Filter* filter, int i);
	~FilterThread();
	
private:
	status_t Run();
	static status_t worker_thread(void* data);
	thread_id fWorkerThread;
	Filter* fFilter;
	int fI;
};

class Filter {
public:
	// The filter uses the input "image" as source image
	// for an operation executed in Run() method which 
	// writes into the destination image, that can be 
	// retrieve using GetBitmap() method.
	// GetBitmap() can be called any time, but it
	// may contain "invalid" pixels.
	// To start the operation Start() method has to
	// be called. The operation is executed in as many
	// threads as CPUs are active.
	// IsRunning() is true as long as there are any
	// threads running.
	// The operation is complete when IsRunning() is false
	// and Stop() has not been called.
	// To abort an operation Stop() method has to
	// be called. Stop() has to be called after Start().
	// When the operation is done (and has not been aborted). 
	// Then listener receives a message with the specified "what" value.
	Filter(BBitmap* image, BMessenger listener, uint32 what);
	virtual ~Filter();
	
	// The bitmap the filter writes into
	BBitmap* GetBitmap();
	
	// Starts one or more FilterThreads
	void Start();
	// Has to be called after Start() (even if IsRunning() is false)
	void Stop();
	// Are there any running FilterThreads?
	bool IsRunning() const;

	// To be implemented by inherited class
	virtual BBitmap* CreateDestImage(BBitmap* srcImage) = 0;
	// Should calculate part i of n of the image. i starts with zero
	virtual void Run(int i, int n) = 0;

	// Used by FilterThread only!
	void Done();
	int32 CPUCount() const { return fCPUCount; }

protected:
	BBitmap* GetSrcImage();
	BBitmap* GetDestImage();

private:
	BMessenger fListener;
	uint32 fWhat;
	int32 fCPUCount;
	bool fStarted;
	sem_id fWaitForThreads;
	volatile int32 fNumberOfThreads;
	volatile bool fIsRunning;
	BBitmap* fSrcImage;
	BBitmap* fDestImage;		
#if TIME_FILTER
	BStopWatch* fStopWatch;
#endif
};

class Scaler : public Filter {
public:
	Scaler(BBitmap* image, BRect rect, BMessenger listener, uint32 what);
	~Scaler();

	BBitmap* CreateDestImage(BBitmap* srcImage);
	void Run(int i, int n);
	bool Matches(BRect rect) const;
	
private:
	void ScaleBilinear(int32 fromRow, int32 toRow);
	void ScaleBilinearFP(int32 fromRow, int32 toRow);

	BRect fRect;
};

#endif

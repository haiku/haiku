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

class Filter {
public:
	Filter(BBitmap* image, BMessenger listener, uint32 what);
	virtual ~Filter();
	
	BBitmap* GetBitmap();
	
	void Start();
	void Stop();
	bool IsRunning() const;
	volatile bool *IsRunningAddr() { return &fIsRunning; }

	virtual BBitmap* CreateDestImage(BBitmap* srcImage) = 0;
	virtual void Run() = 0;

protected:
	BBitmap* GetSrcImage();
	BBitmap* GetDestImage();

private:
	BMessenger fListener;
	uint32 fWhat;
	static status_t worker_thread(void* data);
	volatile bool fIsRunning;
	thread_id fWorkerThread;
	BBitmap* fSrcImage;
	BBitmap* fDestImage;		
};

class Scaler : public Filter {
public:
	Scaler(BBitmap* image, float scale, BMessenger listener, uint32 what);
	~Scaler();

	BBitmap* CreateDestImage(BBitmap* srcImage);
	void Run();
	float Scale() const { return fScale; }
private:
	float fScale;	
};

#endif

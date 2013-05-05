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

typedef int32 intType;
typedef int64 long_fixed_point;
typedef int32 fixed_point;

// Could use shift operator instead of multiplication and division, 
// but compiler will optimize it for use anyway.
#define to_fixed_point(number) static_cast<fixed_point>((number) * kFPPrecisionFactor)
#define from_fixed_point(number) ((number) / kFPPrecisionFactor)
#define to_float(number) from_fixed_point(static_cast<float>(number))

#define int_value(number) ((number) & kFPInverseMask)
#define tail_value(number) ((number) & kFPPrecisionMask)

// Has to be called after muliplication of two fixed point values
#define mult_correction(number) ((number) / kFPPrecisionFactor)

const int32 kFPPrecision = 8; // (32-kFPPrecision).kFPPrecision
const int32 kFPPrecisionFactor = (1 << kFPPrecision);
const int32 kFPPrecisionMask = ((kFPPrecisionFactor)-1);
const int32 kFPInverseMask = (~kFPPrecisionMask);
const int32 kFPOne = to_fixed_point(1);

// Used by class Filter 
class FilterThread {
public:
	FilterThread(Filter* filter, int32 i, int32 n,
		bool runInCurrentThread = false);
	~FilterThread();
	
private:
	status_t Run();
	static status_t worker_thread(void* data);
	
	Filter*	fFilter;
	int32	fI;
	int32	fN;
};

class Filter {
public:
	// The filter uses the input "image" as source image
	// for an operation executed in Run() method which 
	// writes into the destination image, that can be 
	// retrieve using GetBitmap() method.
	// GetBitmap() must be called either before Start(), 
	// or after Start() and IsRunning() returns false.
	// To start the operation Start() method has to
	// be called. The operation is executed in as many
	// threads as GetMaxNumberOfThreads() returns.
	// The implementation of GetMaxNumberOfThreads()
	// can use CPUCount() to retrieve the number of
	// active CPUs at the time the Filter was created.
	// IsRunning() is true as long as there are any
	// threads running.
	// The operation is complete when IsRunning() is false
	// and Stop() has not been called.
	// To abort an operation Stop() method has to
	// be called. Stop() has to be called after Start().
	// When the operation is done Completed() is called.
	// and only if it has not been aborted, then the listener
	// receives a message with the specified "what" value.
	Filter(BBitmap* image, BMessenger listener, uint32 what);
	virtual ~Filter();
	
	// The bitmap the filter writes into
	BBitmap* GetBitmap();
	// Removes the destination image from Filter (caller is new owner of image)
	BBitmap* DetachBitmap();
	
	// Starts one or more FilterThreads. Returns immediately if async is true.
	// Either Wait() or Stop() has to be called if async is true!
	void Start(bool async = true);
	// Wait for completion of operation
	void Wait();
	// Has to be called after Start() (even if IsRunning() is false)
	void Stop();
	// Are there any running FilterThreads?
	bool IsRunning() const;

	// To be implemented by inherited class (methods are called in this order):
	virtual BBitmap* CreateDestImage(BBitmap* srcImage) = 0;
	// The number of processing units
	virtual int32 GetNumberOfUnits() = 0;
	// Should calculate part i of n of the image. i starts with zero
	virtual void Run(int32 i, int32 n) = 0;
	// Completed() is called when the last FilterThread has completed its work.
	virtual void Completed();

	// Used by FilterThread only!
	void FilterThreadDone();
	void FilterThreadInitFailed();

	bool IsBitmapValid(BBitmap* bitmap) const;

protected:
	// Number of threads to be used to perform the operation
	int32 NumberOfThreads();
	BBitmap* GetSrcImage();
	BBitmap* GetDestImage();

private:
	int32 NumberOfActiveCPUs() const;
	// Returns the number of active CPUs
	int32 CPUCount() const { return fCPUCount; }

	BMessenger		fListener;
	uint32			fWhat;
	int32			fCPUCount; // the number of active CPUs
	bool			fStarted; // has Start() been called?
	sem_id			fWaitForThreads; // to exit
	int32			fN; // the number of used filter threads
	volatile int32	fNumberOfThreads; // the current number of FilterThreads
	volatile bool	fIsRunning; // FilterThreads should process data as long as it is true
	BBitmap*		fSrcImage;
	bool			fDestImageInitialized;
	BBitmap*		fDestImage;		
#if TIME_FILTER
	BStopWatch*		fStopWatch;
#endif
};

// Scales and optionally dithers an image
class Scaler : public Filter {
public:
	Scaler(BBitmap* image, BRect rect, BMessenger listener, uint32 what,
		bool dither);
	~Scaler();

	BBitmap* CreateDestImage(BBitmap* srcImage);
	int32 GetNumberOfUnits();
	void Run(int32 i, int32 n);
	void Completed();
	bool Matches(BRect rect, bool dither) const;
	
private:
	void ScaleBilinear(int32 fromRow, int32 toRow);
	void ScaleBilinearFP(int32 fromRow, int32 toRow);
	inline void RowValues(float* sum, const uchar* srcData, intType srcW,
					intType fromX, intType toX, const float a0X,
					const float a1X, const int32 kBPP);
	void DownScaleBilinear(int32 fromRow, int32 toRow);
	static inline uchar Limit(intType value);
	void Dither(int32 fromRow, int32 toRow);
	
	BBitmap* fScaledImage;
	BRect fRect;
	bool fDither;
};

// Rotates, mirrors or inverts an image
class ImageProcessor : public Filter {
public:
	enum operation {
		kRotateClockwise,
		kRotateCounterClockwise,
		kFlipLeftToRight,
		kFlipTopToBottom,
		kInvert,
		kNumberOfAffineTransformations = 4
	};

	ImageProcessor(enum operation op, BBitmap* image, BMessenger listener,
		uint32 what);
	BBitmap* CreateDestImage(BBitmap* srcImage);
	int32 GetNumberOfUnits();
	void Run(int32 i, int32 n);
	
private:	
	int32 BytesPerPixel(color_space cs) const;
	inline void CopyPixel(uchar* dest, int32 destX, int32 destY,
					const uchar* src, int32 x, int32 y);
	inline void InvertPixel(int32 x, int32 y, uchar* dest, const uchar* src);

	enum operation fOp;
	int32 fBPP;
	int32 fWidth;
	int32 fHeight;
	int32 fSrcBPR;
	int32 fDestBPR;
};

#endif

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

// Implementation of FilterThread
FilterThread::FilterThread(Filter* filter, int i)
	: fWorkerThread(-1)
	, fFilter(filter)
	, fI(i)
{
	fWorkerThread = spawn_thread(worker_thread, "filter", suggest_thread_priority(B_STATUS_RENDERING), this);
	if (fWorkerThread >= 0) {
		resume_thread(fWorkerThread);
	}	
}

FilterThread::~FilterThread()
{
	fFilter->Done();
}

status_t 
FilterThread::worker_thread(void* data)
{
	FilterThread* thread = (FilterThread*)data;
	return thread->Run();
}

status_t
FilterThread::Run()
{
	fFilter->Run(fI, fFilter->CPUCount());
	delete this;
	return B_OK;
}

// Implementation of Filter
Filter::Filter(BBitmap* image, BMessenger listener, uint32 what)
	: fListener(listener)
	, fWhat(what)
	, fStarted(false)
	, fNumberOfThreads(0)
	, fIsRunning(false)
	, fSrcImage(image)
	, fDestImage(NULL)
{
	system_info info;
	get_system_info(&info);
	fCPUCount = info.cpu_count;
	fWaitForThreads = create_sem(0, "wait_for_threads");
	#if TIME_FILTER
	fStopWatch = NULL;
	#endif	
}

Filter::~Filter()
{
	delete fDestImage;
	delete_sem(fWaitForThreads);
}

BBitmap*
Filter::GetBitmap()
{
	if (fDestImage == NULL) {
		fDestImage = CreateDestImage(fSrcImage);
	}
	return fDestImage;
}

void
Filter::Start()
{
	GetBitmap();
	if (fStarted || fSrcImage == NULL || fDestImage == NULL) return;
	
	#if TIME_FILTER
		fStopWatch = new BStopWatch("Filter Time");
	#endif
	
	fNumberOfThreads = fCPUCount;
	fIsRunning = true;	
	fStarted = true;
	
	// start filter threads
	for (int i = 0; i < fCPUCount; i ++) {
		new FilterThread(this, i);
	}
}

void
Filter::Stop()
{
	if (fStarted) {
		// tell FilterThreads to stop calculations
		fIsRunning = false;
		// wait for threads to exit
		acquire_sem_etc(fWaitForThreads, fCPUCount, 0, 0);
		// ready to start again
		fStarted = false;
	}
}

void
Filter::Done()
{
	if (atomic_add(&fNumberOfThreads, -1) == 1) {
		#if TIME_FILTER
			delete fStopWatch; fStopWatch = NULL;
		#endif
		if (fIsRunning) {
			fListener.SendMessage(fWhat);
		}
		fIsRunning = false;
	}
	release_sem(fWaitForThreads);
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

// Implementation of (bilinear) Scaler
Scaler::Scaler(BBitmap* image, BRect rect, BMessenger listener, uint32 what)
	: Filter(image, listener, what)
	, fRect(rect)
{
}

Scaler::~Scaler()
{
}

BBitmap*
Scaler::CreateDestImage(BBitmap* srcImage)
{
	if (srcImage == NULL || srcImage->ColorSpace() != B_RGB32 && srcImage->ColorSpace() !=B_RGBA32) return NULL;
	BRect dest(0, 0, fRect.IntegerWidth(), fRect.IntegerHeight());
	BBitmap* destImage = new BBitmap(dest, srcImage->ColorSpace());
	return destImage;
}

bool
Scaler::Matches(BRect rect) const {
	return fRect.IntegerWidth() == rect.IntegerWidth() &&
		fRect.IntegerHeight() == rect.IntegerHeight();
}


// Scale bilinear using floating point calculations
typedef int32 intType;

typedef struct {
	intType srcColumn;
	float alpha0;
	float alpha1;
} ColumnData;

void
Scaler::ScaleBilinear(intType fromRow, int32 toRow)
{
	BBitmap* src;
	BBitmap* dest;
	intType srcW, srcH;
	intType destW, destH;
	intType x, y, i;
	ColumnData* columnData;
	ColumnData* cd;
	const uchar* srcBits;
	uchar* destBits;
	intType srcBPR, destBPR;
	const uchar* srcData;
	uchar* destDataRow;
	uchar* destData;
	const int32 kBPP = 4;

	src = GetSrcImage();
	dest = GetDestImage();
		
	srcW = src->Bounds().IntegerWidth();
	srcH = src->Bounds().IntegerHeight();
	destW = dest->Bounds().IntegerWidth();
	destH = dest->Bounds().IntegerHeight();
	
	srcBits = (uchar*)src->Bits();
	destBits = (uchar*)dest->Bits();
	srcBPR = src->BytesPerRow();
	destBPR = dest->BytesPerRow();
	
	columnData = new ColumnData[destW];
	cd = columnData;
	for (i = 0; i < destW; i ++, cd++) {
		float column = (float)i * (float)srcW / (float)destW;
		cd->srcColumn = (intType)column;
		cd->alpha1 = column - cd->srcColumn;
		cd->alpha0 = 1.0 - cd->alpha1;
	}

	destDataRow = destBits + fromRow * destBPR;
		
	for (y = fromRow; IsRunning() && y <= toRow; y ++, destDataRow += destBPR) {
		float row;
		intType srcRow;
		float alpha0, alpha1;

		row = (float)y * (float)srcH / (float)destH;
		srcRow = (intType)row;
		alpha1 = row - srcRow;
		alpha0 = 1.0 - alpha1;

		srcData = srcBits + srcRow * srcBPR;
		destData = destDataRow;
	
		if (y < destH) {
			float a0, a1;
			const uchar *a, *b, *c, *d;

			for (x = 0; x < destW; x ++, destData += kBPP) {
				a = srcData + columnData[x].srcColumn * kBPP;
				b = a + kBPP;
				c = a + srcBPR;
				d = c + kBPP;
				
				a0 = columnData[x].alpha0;
				a1 = columnData[x].alpha1;
				
				destData[0] = static_cast<uchar>(
								(a[0] * a0 + b[0] * a1) * alpha0 +
								(c[0] * a0 + d[0] * a1) * alpha1);
				destData[1] = static_cast<uchar>(
								(a[1] * a0 + b[1] * a1) * alpha0 +
								(c[1] * a0 + d[1] * a1) * alpha1);
				destData[2] = static_cast<uchar>(
								(a[2] * a0 + b[2] * a1) * alpha0 +
								(c[2] * a0 + d[2] * a1) * alpha1);
			}
			
			// right column
			a = srcData + srcW * kBPP;
			c = a + srcBPR;
			
			destData[0] = static_cast<uchar>(a[0] * alpha0 + c[0] * alpha1);
			destData[1] = static_cast<uchar>(a[1] * alpha0 + c[1] * alpha1);
			destData[2] = static_cast<uchar>(a[2] * alpha0 + c[2] * alpha1);
		} else {
			float a0, a1;
			const uchar *a, *b;
			for (x = 0; x < destW; x ++, destData += kBPP) {
				a = srcData + columnData[x].srcColumn * kBPP;
				b = a + kBPP;
				
				a0 = columnData[x].alpha0;
				a1 = columnData[x].alpha1;
				
				destData[0] = static_cast<uchar>(a[0] * a0 + b[0] * a1);
				destData[1] = static_cast<uchar>(a[1] * a0 + b[1] * a1);
				destData[2] = static_cast<uchar>(a[2] * a0 + b[2] * a1);
			}
			
			// bottom, right pixel
			a = srcData + srcW * kBPP;

			destData[0] = a[0];
			destData[1] = a[1];
			destData[2] = a[2];
		}
	
	}
	
	delete[] columnData;
}

// Scale bilinear using fixed point calculations
// Is already more than two times faster than floating point version
// on AMD Athlon 1 GHz and Intel Pentium III 866 MHz.
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

typedef struct {
	int32 srcColumn;
	fixed_point alpha0;
	fixed_point alpha1;
} ColumnDataFP;

void
Scaler::ScaleBilinearFP(intType fromRow, int32 toRow)
{
	BBitmap* src;
	BBitmap* dest;
	intType srcW, srcH;
	intType destW, destH;
	intType x, y, i;
	ColumnDataFP* columnData;
	ColumnDataFP* cd;
	const uchar* srcBits;
	uchar* destBits;
	intType srcBPR, destBPR;
	const uchar* srcData;
	uchar* destDataRow;
	uchar* destData;
	const int32 kBPP = 4;

	src = GetSrcImage();
	dest = GetDestImage();
		
	srcW = src->Bounds().IntegerWidth();
	srcH = src->Bounds().IntegerHeight();
	destW = dest->Bounds().IntegerWidth();
	destH = dest->Bounds().IntegerHeight();
	
	srcBits = (uchar*)src->Bits();
	destBits = (uchar*)dest->Bits();
	srcBPR = src->BytesPerRow();
	destBPR = dest->BytesPerRow();
	
	fixed_point fpSrcW = to_fixed_point(srcW); 
	fixed_point fpDestW = to_fixed_point(destW);
	fixed_point fpSrcH = to_fixed_point(srcH);
	fixed_point fpDestH = to_fixed_point(destH);
	
	columnData = new ColumnDataFP[destW];
	cd = columnData;
	for (i = 0; i < destW; i ++, cd++) {
		fixed_point column = to_fixed_point(i) * (long_fixed_point)fpSrcW / fpDestW;
		cd->srcColumn = from_fixed_point(column);
		cd->alpha1 = tail_value(column); // weigth for left pixel value
		cd->alpha0 = kFPOne - cd->alpha1; // weigth for right pixel value
	}

	destDataRow = destBits + fromRow * destBPR;
		
	for (y = fromRow; IsRunning() && y <= toRow; y ++, destDataRow += destBPR) {
		fixed_point row;
		intType srcRow;
		fixed_point alpha0, alpha1;

		row = to_fixed_point(y) * (long_fixed_point)fpSrcH / fpDestH;
		srcRow = from_fixed_point(row);
		alpha1 = tail_value(row); // weight for row y+1
		alpha0 = kFPOne - alpha1; // weight for row y

		srcData = srcBits + srcRow * srcBPR;
		destData = destDataRow;

		// Need mult_correction for "outer" multiplication only
		#define I4(i) from_fixed_point(mult_correction(\
							(a[i] * a0 + b[i] * a1) * alpha0 + \
							(c[i] * a0 + d[i] * a1) * alpha1))
		#define V2(i) from_fixed_point(a[i] * alpha0 + c[i] * alpha1);
		#define H2(i) from_fixed_point(a[i] * a0 + b[i] * a1);
	
		if (y < destH) {
			fixed_point a0, a1;
			const uchar *a, *b, *c, *d;

			for (x = 0; x < destW; x ++, destData += kBPP) {
				a = srcData + columnData[x].srcColumn * kBPP;
				b = a + kBPP;
				c = a + srcBPR;
				d = c + kBPP;
				
				a0 = columnData[x].alpha0;
				a1 = columnData[x].alpha1;
				
				destData[0] = I4(0);
				destData[1] = I4(1);
				destData[2] = I4(2);
			}
			
			// right column
			a = srcData + srcW * kBPP;
			c = a + srcBPR;
			
			destData[0] = V2(0);
			destData[1] = V2(1);
			destData[2] = V2(2);
		} else {
			fixed_point a0, a1;
			const uchar *a, *b;
			for (x = 0; x < destW; x ++, destData += kBPP) {
				a = srcData + columnData[x].srcColumn * kBPP;
				b = a + kBPP;
				
				a0 = columnData[x].alpha0;
				a1 = columnData[x].alpha1;
				
				destData[0] = H2(0);
				destData[1] = H2(1);
				destData[2] = H2(2);
			}
			
			// bottom, right pixel
			a = srcData + srcW * kBPP;

			destData[0] = a[0];
			destData[1] = a[1];
			destData[2] = a[2];
		}
	
	}
	
	delete[] columnData;
}

void
Scaler::Run(int i, int n)
{	
	int32 from, to, height;
	height = (GetDestImage()->Bounds().IntegerHeight()+1)/n;
	from = i * height;
	if (i+1 == n) {
		to = (int32)GetDestImage()->Bounds().bottom;
	} else {
		to = from + height - 1;
	}
	ScaleBilinearFP(from, to);
}

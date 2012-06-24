/*
 * Copyright 2003-2006, Haiku, Inc. All rights reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserved.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 *		Ryan Leavengood, leavengood@gmail.com
 *		yellowTAB GmbH
 *		Bernd Korz
 */

#include <scheduler.h>
#include <Debug.h>
#include <Screen.h>

#include <syscalls.h>

#include "Filter.h"


// Implementation of FilterThread
FilterThread::FilterThread(Filter* filter, int32 i, int32 n,
	bool runInCurrentThread)
	: fFilter(filter),
	fI(i),
	fN(n)
{
	if (runInCurrentThread)
		Run();
	else {
		thread_id tid;
		tid = spawn_thread(worker_thread, "filter",
			suggest_thread_priority(B_STATUS_RENDERING), this);
		if (tid >= 0)
			resume_thread(tid);
		else
			delete this;
	}
}


FilterThread::~FilterThread()
{
	fFilter->FilterThreadDone();
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
	if (fI == 0) {
		BBitmap* bm;
		// create destination image in first thread
		bm = fFilter->GetBitmap();
		if (bm == NULL) {
			fFilter->FilterThreadInitFailed();
			return B_ERROR;
		}
		// and start other filter threads
		for (int32 i = fI + 1; i < fN; i ++) {
			new FilterThread(fFilter, i, fN);
		}
	}
	if (fFilter->GetBitmap())
		fFilter->Run(fI, fN);

	delete this;
	return B_OK;
}

// Implementation of Filter
Filter::Filter(BBitmap* image, BMessenger listener, uint32 what)
	:
	fListener(listener),
	fWhat(what),
	fStarted(false),
	fN(0),
	fNumberOfThreads(0),
	fIsRunning(false),
	fSrcImage(image),
	fDestImageInitialized(false),
	fDestImage(NULL)
{
	fCPUCount = NumberOfActiveCPUs();

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
	if (!fDestImageInitialized) {
		fDestImageInitialized = true;
		fDestImage = CreateDestImage(fSrcImage);
	}
	return fDestImage;
}


BBitmap*
Filter::DetachBitmap()
{
	BBitmap* image = fDestImage;
	fDestImage = NULL;
	return image;
}


void
Filter::Start(bool async)
{
	if (fStarted || fSrcImage == NULL) return;

	#if TIME_FILTER
		fStopWatch = new BStopWatch("Filter Time");
	#endif

	fN = NumberOfThreads();
	fNumberOfThreads = fN;
	fIsRunning = true;
	fStarted = true;

	// start first filter thread
	new FilterThread(this, 0, fN, !async);

	if (!async)
		Wait();
}


void
Filter::Wait()
{
	if (fStarted) {
		// wait for threads to exit
		while (acquire_sem_etc(fWaitForThreads, fN, 0, 0) == B_INTERRUPTED);
		// ready to start again
		fStarted = false;
	}
}


void
Filter::Stop()
{
	// tell FilterThreads to stop calculations
	fIsRunning = false;
	Wait();
}


bool
Filter::IsRunning() const
{
	return fIsRunning;
}


void
Filter::Completed()
{
}


void
Filter::FilterThreadDone()
{
	if (atomic_add(&fNumberOfThreads, -1) == 1) {
		#if TIME_FILTER
			delete fStopWatch; fStopWatch = NULL;
		#endif
		Completed();
		if (fIsRunning)
			fListener.SendMessage(fWhat);

		fIsRunning = false;
	}
	release_sem(fWaitForThreads);
}


void
Filter::FilterThreadInitFailed()
{
	ASSERT(fNumberOfThreads == fN);
	fNumberOfThreads = 0;
	Completed();
	fIsRunning = false;
	release_sem_etc(fWaitForThreads, fN, 0);
}


bool
Filter::IsBitmapValid(BBitmap* bitmap) const
{
	return bitmap != NULL && bitmap->InitCheck() == B_OK && bitmap->IsValid();
}


int32
Filter::NumberOfThreads()
{
	const int32 units = GetNumberOfUnits();
	int32 n;
	n = units / 32; // at least 32 units per CPU
	if (n > CPUCount())
		n = CPUCount();
	else if (n <= 0)
		n = 1; // at least one thread!

	return n;
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


int32
Filter::NumberOfActiveCPUs() const
{
	int count;
	system_info info;
	get_system_info(&info);
	count = info.cpu_count;
	int32 cpuCount = 0;
	for (int i = 0; i < count; i ++) {
		if (_kern_cpu_enabled(i))
			cpuCount++;
	}
	if (cpuCount == 0)
		cpuCount = 1;

	return cpuCount;
}


// Implementation of (bilinear) Scaler
Scaler::Scaler(BBitmap* image, BRect rect, BMessenger listener, uint32 what,
	bool dither)
	:
	Filter(image, listener, what),
	fScaledImage(NULL),
	fRect(rect),
	fDither(dither)
{
}


Scaler::~Scaler()
{
	if (GetDestImage() != fScaledImage) {
		delete fScaledImage;
		fScaledImage = NULL;
	}
}


BBitmap*
Scaler::CreateDestImage(BBitmap* srcImage)
{
	if (srcImage == NULL || (srcImage->ColorSpace() != B_RGB32
		&& srcImage->ColorSpace() != B_RGBA32))
			return NULL;

	BRect dest(0, 0, fRect.IntegerWidth(), fRect.IntegerHeight());
	BBitmap* destImage = new BBitmap(dest,
		fDither ? B_CMAP8 : srcImage->ColorSpace());

	if (!IsBitmapValid(destImage)) {
		delete destImage;
		return NULL;
	}

	if (fDither)
	{
		BRect dest_rect(0, 0, fRect.IntegerWidth(), fRect.IntegerHeight());
		fScaledImage = new BBitmap(dest_rect, srcImage->ColorSpace());
		if (!IsBitmapValid(fScaledImage)) {
			delete destImage;
			delete fScaledImage;
			fScaledImage = NULL;
			return NULL;
		}
	} else
		fScaledImage = destImage;

	return destImage;
}


bool
Scaler::Matches(BRect rect, bool dither) const
{
	return fRect.IntegerWidth() == rect.IntegerWidth()
		&& fRect.IntegerHeight() == rect.IntegerHeight()
		&& fDither == dither;
}


// Scale bilinear using floating point calculations
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
	dest = fScaledImage;

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
	for (i = 0; i < destW; i++, cd++) {
		float column = (float)i * (float)srcW / (float)destW;
		cd->srcColumn = (intType)column;
		cd->alpha1 = column - cd->srcColumn;
		cd->alpha0 = 1.0 - cd->alpha1;
	}

	destDataRow = destBits + fromRow * destBPR;

	for (y = fromRow; IsRunning() && y <= toRow; y++, destDataRow += destBPR) {
		float row;
		intType srcRow;
		float alpha0, alpha1;

		if (destH == 0)
			row = 0;
		else
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
				destData[3] = static_cast<uchar>(
								(a[3] * a0 + b[3] * a1) * alpha0 +
								(c[3] * a0 + d[3] * a1) * alpha1);
			}

			// right column
			a = srcData + srcW * kBPP;
			c = a + srcBPR;

			destData[0] = static_cast<uchar>(a[0] * alpha0 + c[0] * alpha1);
			destData[1] = static_cast<uchar>(a[1] * alpha0 + c[1] * alpha1);
			destData[2] = static_cast<uchar>(a[2] * alpha0 + c[2] * alpha1);
			destData[3] = static_cast<uchar>(a[3] * alpha0 + c[3] * alpha1);
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
				destData[3] = static_cast<uchar>(a[3] * a0 + b[3] * a1);
			}

			// bottom, right pixel
			a = srcData + srcW * kBPP;

			destData[0] = a[0];
			destData[1] = a[1];
			destData[2] = a[2];
			destData[3] = a[3];
		}

	}

	delete[] columnData;
}


// Scale bilinear using fixed point calculations
// Is already more than two times faster than floating point version
// on AMD Athlon 1 GHz and Dual Intel Pentium III 866 MHz.

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
	dest = fScaledImage;

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
	for (i = 0; i < destW; i++, cd++) {
		fixed_point column = to_fixed_point(i) * (long_fixed_point)fpSrcW
			/ fpDestW;
		cd->srcColumn = from_fixed_point(column);
		cd->alpha1 = tail_value(column); // weigth for left pixel value
		cd->alpha0 = kFPOne - cd->alpha1; // weigth for right pixel value
	}

	destDataRow = destBits + fromRow * destBPR;

	for (y = fromRow; IsRunning() && y <= toRow; y++, destDataRow += destBPR) {
		fixed_point row;
		intType srcRow;
		fixed_point alpha0, alpha1;

		if (fpDestH == 0)
			row = 0;
		else
			row = to_fixed_point(y) * (long_fixed_point)fpSrcH / fpDestH;

		srcRow = from_fixed_point(row);
		alpha1 = tail_value(row); // weight for row y + 1
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
				destData[3] = I4(3);
			}

			// right column
			a = srcData + srcW * kBPP;
			c = a + srcBPR;

			destData[0] = V2(0);
			destData[1] = V2(1);
			destData[2] = V2(2);
			destData[3] = V2(3);
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
				destData[3] = H2(3);
			}

			// bottom, right pixel
			a = srcData + srcW * kBPP;

			destData[0] = a[0];
			destData[1] = a[1];
			destData[2] = a[2];
			destData[3] = a[3];
		}
	}

	delete[] columnData;
}


void
Scaler::RowValues(float* sum, const uchar* src, intType srcW, intType fromX,
	intType toX, const float a0X, const float a1X, const int32 kBPP)
{
	sum[0] = a0X * src[0];
	sum[1] = a0X * src[1];
	sum[2] = a0X * src[2];

	src += kBPP;

	for (int32 x = fromX + 1; x < toX; x++, src += kBPP) {
		sum[0] += src[0];
		sum[1] += src[1];
		sum[2] += src[2];
	}

	if (toX <= srcW) {
		sum[0] += a1X * src[0];
		sum[1] += a1X * src[1];
		sum[2] += a1X * src[2];
	}
}


typedef struct {
	int32 from;
	int32 to;
	float alpha0;
	float alpha1;
} DownScaleColumnData;


void
Scaler::DownScaleBilinear(intType fromRow, int32 toRow)
{
	BBitmap* src;
	BBitmap* dest;
	intType srcW, srcH;
	intType destW, destH;
	intType x, y;
	const uchar* srcBits;
	uchar* destBits;
	intType srcBPR, destBPR;
	const uchar* srcData;
	uchar* destDataRow;
	uchar* destData;
	const int32 kBPP = 4;
	DownScaleColumnData* columnData;

	src = GetSrcImage();
	dest = fScaledImage;

	srcW = src->Bounds().IntegerWidth();
	srcH = src->Bounds().IntegerHeight();
	destW = dest->Bounds().IntegerWidth();
	destH = dest->Bounds().IntegerHeight();

	srcBits = (uchar*)src->Bits();
	destBits = (uchar*)dest->Bits();
	srcBPR = src->BytesPerRow();
	destBPR = dest->BytesPerRow();

	destDataRow = destBits + fromRow * destBPR;

	const float deltaX = (srcW + 1.0) / (destW + 1.0);
	const float deltaY = (srcH + 1.0) / (destH + 1.0);
	const float deltaXY = deltaX * deltaY;

	columnData = new DownScaleColumnData[destW + 1];
	DownScaleColumnData* cd = columnData;
	for (x = 0; x <= destW; x++, cd++) {
		const float fFromX = x * deltaX;
		const float fToX = fFromX + deltaX;

		cd->from = (intType)fFromX;
		cd->to = (intType)fToX;

		cd->alpha0 = 1.0 - (fFromX - cd->from);
		cd->alpha1 = fToX - cd->to;
	}

	for (y = fromRow; IsRunning() && y <= toRow; y ++, destDataRow += destBPR) {
		const float fFromY = y * deltaY;
		const float fToY = fFromY + deltaY;

		const intType fromY = (intType)fFromY;
		const intType toY = (intType)fToY;

		const float a0Y = 1.0 - (fFromY - fromY);
		const float a1Y = fToY - toY;

		const uchar* srcDataRow = srcBits + fromY * srcBPR;
		destData = destDataRow;

		cd = columnData;
		for (x = 0; x <= destW; x++, destData += kBPP, cd++) {
			const intType fromX = cd->from;
			const intType toX = cd->to;

			const float a0X = cd->alpha0;
			const float a1X = cd->alpha1;

			srcData = srcDataRow + fromX * kBPP;

			float totalSum[3];
			float sum[3];

			RowValues(sum, srcData, srcW, fromX, toX, a0X, a1X, kBPP);
			totalSum[0] = a0Y * sum[0];
			totalSum[1] = a0Y * sum[1];
			totalSum[2] = a0Y * sum[2];

			srcData += srcBPR;

			for (int32 r = fromY + 1; r < toY; r++, srcData += srcBPR) {
				RowValues(sum, srcData, srcW, fromX, toX, a0X, a1X, kBPP);
				totalSum[0] += sum[0];
				totalSum[1] += sum[1];
				totalSum[2] += sum[2];
			}

			if (toY <= srcH) {
				RowValues(sum, srcData, srcW, fromX, toX, a0X, a1X, kBPP);
				totalSum[0] += a1Y * sum[0];
				totalSum[1] += a1Y * sum[1];
				totalSum[2] += a1Y * sum[2];
			}

			destData[0] = static_cast<uchar>(totalSum[0] / deltaXY);
			destData[1] = static_cast<uchar>(totalSum[1] / deltaXY);
			destData[2] = static_cast<uchar>(totalSum[2] / deltaXY);
		}
	}

	delete[] columnData;
}


// Flyod-Steinberg Dithering
// Filter (distribution of error to adjacent pixels, X is current pixel):
// 0 X 7
// 3 5 1

typedef struct {
	intType error[3];
} DitheringColumnData;


uchar
Scaler::Limit(intType value)
{
	if (value < 0) {
		value = 0;
	} if (value > 255) {
		value = 255;
	}
	return value;
}


void
Scaler::Dither(int32 fromRow, int32 toRow)
{
	BBitmap* src;
	BBitmap* dest;
	intType destW;
	intType x, y;

	uchar* srcBits;
	intType srcBPR;
	uchar* srcDataRow;
	uchar* srcData;

	uchar* destBits;
	intType destBPR;
	uchar* destDataRow;
	uchar* destData;
	const int32 kBPP = 4;
	DitheringColumnData* columnData0;
	DitheringColumnData* columnData;
	DitheringColumnData* cd;
	BScreen screen;
	intType error[3], err[3];

	src = fScaledImage;
	dest = GetDestImage();

	ASSERT(src->ColorSpace() == B_RGB32 || src->ColorSpace() == B_RGBA32);
	ASSERT(dest->ColorSpace() == B_CMAP8);
	ASSERT(src->Bounds().IntegerWidth() == dest->Bounds().IntegerWidth());
	ASSERT(src->Bounds().IntegerHeight() == dest->Bounds().IntegerHeight());

	destW = dest->Bounds().IntegerWidth();

	srcBits = (uchar*)src->Bits();
	srcBPR = src->BytesPerRow();
	destBits = (uchar*)dest->Bits();
	destBPR = dest->BytesPerRow();

	// Allocate space for sentinel at left and right bounds,
	// so that columnData[-1] and columnData[destW + 1] can be safely accessed
	columnData0 = new DitheringColumnData[destW + 3];
	columnData = columnData0 + 1;

	// clear error
	cd = columnData;
	for (x = destW; x >= 0; x --, cd++) {
		cd->error[0] = cd->error[1] = cd->error[2] = 0;
	}

	srcDataRow = srcBits + fromRow * srcBPR;
	destDataRow = destBits + fromRow * destBPR;
	for (y = fromRow; IsRunning() && y <= toRow; y++, srcDataRow += srcBPR,
		destDataRow += destBPR) {
		// left to right
		error[0] = error[1] = error[2] = 0;
		srcData = srcDataRow;
		destData = destDataRow;
		for (x = 0; x <= destW; x ++, srcData += kBPP, destData += 1) {
			rgb_color color, actualColor;
			uint8 index;

			color.red = Limit(srcData[2] + error[0] / 16);
			color.green = Limit(srcData[1] + error[1] / 16);
			color.blue = Limit(srcData[0] + error[2] / 16);

			index = screen.IndexForColor(color);
			actualColor = screen.ColorForIndex(index);

			*destData = index;

			err[0] = color.red - actualColor.red;
			err[1] = color.green - actualColor.green;
			err[2] = color.blue - actualColor.blue;

			// distribute error
			// get error for next pixel
			cd = &columnData[x + 1];
			error[0] = cd->error[0] + 7 * err[0];
			error[1] = cd->error[1] + 7 * err[1];
			error[2] = cd->error[2] + 7 * err[2];

			// set error for right pixel below current pixel
			cd->error[0] = err[0];
			cd->error[1] = err[1];
			cd->error[2] = err[2];

			// add error for pixel below current pixel
			cd--;
			cd->error[0] += 5 * err[0];
			cd->error[1] += 5 * err[1];
			cd->error[2] += 5 * err[2];

			// add error for left pixel below current pixel
			cd--;
			cd->error[0] += 3 * err[0];
			cd->error[1] += 3 * err[1];
			cd->error[2] += 3 * err[2];
		}
		// Note: Alogrithm has good results with "left to right" already
		// Optionally remove code to end of block:
		y++;
		srcDataRow += srcBPR; destDataRow += destBPR;
		if (y > toRow) break;
		// right to left
		error[0] = error[1] = error[2] = 0;
		srcData = srcDataRow + destW * kBPP;
		destData = destDataRow + destW;
		for (x = 0; x <= destW; x++, srcData -= kBPP, destData -= 1) {
			rgb_color color, actualColor;
			uint8 index;

			color.red = Limit(srcData[2] + error[0] / 16);
			color.green = Limit(srcData[1] + error[1] / 16);
			color.blue = Limit(srcData[0] + error[2] / 16);

			index = screen.IndexForColor(color);
			actualColor = screen.ColorForIndex(index);

			*destData = index;

			err[0] = color.red - actualColor.red;
			err[1] = color.green - actualColor.green;
			err[2] = color.blue - actualColor.blue;

			// distribute error
			// get error for next pixel
			cd = &columnData[x - 1];
			error[0] = cd->error[0] + 7 * err[0];
			error[1] = cd->error[1] + 7 * err[1];
			error[2] = cd->error[2] + 7 * err[2];

			// set error for left pixel below current pixel
			cd->error[0] = err[0];
			cd->error[1] = err[1];
			cd->error[2] = err[2];

			// add error for pixel below current pixel
			cd++;
			cd->error[0] += 5 * err[0];
			cd->error[1] += 5 * err[1];
			cd->error[2] += 5 * err[2];

			// add error for right pixel below current pixel
			cd++;
			cd->error[0] += 3 * err[0];
			cd->error[1] += 3 * err[1];
			cd->error[2] += 3 * err[2];
		}
	}

	delete[] columnData0;
}


int32
Scaler::GetNumberOfUnits()
{
	return fRect.IntegerHeight() + 1;
}


void
Scaler::Run(int32 i, int32 n)
{
	int32 from, to, height, imageHeight;
	imageHeight = GetDestImage()->Bounds().IntegerHeight() + 1;
	height = imageHeight / n;
	from = i * height;
	if (i + 1 == n)
		to = imageHeight - 1;
	else
		to = from + height - 1;
	
	if (GetDestImage()->Bounds().Width() >= GetSrcImage()->Bounds().Width())
		ScaleBilinearFP(from, to);
	else
		DownScaleBilinear(from, to);

	if (fDither)
		Dither(from, to);

}


void
Scaler::Completed()
{
	if (GetDestImage() != fScaledImage)
		delete fScaledImage;

	fScaledImage = NULL;
}


// Implementation of ImageProcessor
ImageProcessor::ImageProcessor(enum operation op, BBitmap* image,
	BMessenger listener, uint32 what)
	:
	Filter(image, listener, what),
	fOp(op),
	fBPP(0),
	fWidth(0),
	fHeight(0),
	fSrcBPR(0),
	fDestBPR(0)
{
}


BBitmap*
ImageProcessor::CreateDestImage(BBitmap* /* srcImage */)
{
	color_space cs;
	BBitmap* bm;
	BRect rect;

	if (GetSrcImage() == NULL)
		return NULL;

	cs = GetSrcImage()->ColorSpace();
	fBPP = BytesPerPixel(cs);
	if (fBPP < 1)
		return NULL;

	fWidth = GetSrcImage()->Bounds().IntegerWidth();
	fHeight = GetSrcImage()->Bounds().IntegerHeight();

	if (fOp == kRotateClockwise || fOp == kRotateCounterClockwise)
		rect.Set(0, 0, fHeight, fWidth);
	else
		rect.Set(0, 0, fWidth, fHeight);

	bm = new BBitmap(rect, cs);
	if (!IsBitmapValid(bm)) {
		delete bm;
		return NULL;
	}

	fSrcBPR = GetSrcImage()->BytesPerRow();
	fDestBPR = bm->BytesPerRow();

	return bm;
}


int32
ImageProcessor::GetNumberOfUnits()
{
	return GetSrcImage()->Bounds().IntegerHeight() + 1;
}


int32
ImageProcessor::BytesPerPixel(color_space cs) const
{
	switch (cs) {
		case B_RGB32:		// fall through
		case B_RGB32_BIG:	// fall through
		case B_RGBA32:		// fall through
		case B_RGBA32_BIG:	return 4;

		case B_RGB24_BIG:	// fall through
		case B_RGB24:		return 3;

		case B_RGB16:		// fall through
		case B_RGB16_BIG:	// fall through
		case B_RGB15:		// fall through
		case B_RGB15_BIG:	// fall through
		case B_RGBA15:		// fall through
		case B_RGBA15_BIG:	return 2;

		case B_GRAY8:		// fall through
		case B_CMAP8:		return 1;
		case B_GRAY1:		return 0;
		default: return -1;
	}
}


void
ImageProcessor::CopyPixel(uchar* dest, int32 destX, int32 destY,
	const uchar* src, int32 x, int32 y)
{
	// Note: On my systems (Dual Intel P3 866MHz and AMD Athlon 1GHz),
	// replacing the multiplications below with pointer arithmethics showed
	// no speedup at all!
	dest += fDestBPR * destY + destX * fBPP;
	src += fSrcBPR * y + x * fBPP;
	// Replacing memcpy with this switch statement is slightly faster
	switch (fBPP) {
		case 4:
			dest[3] = src[3];
		case 3:
			dest[2] = src[2];
		case 2:
			dest[1] = src[1];
		case 1:
			dest[0] = src[0];
			break;
	}
}


// Note: For B_CMAP8 InvertPixel inverts the color index not the color value!
void
ImageProcessor::InvertPixel(int32 x, int32 y, uchar* dest, const uchar* src)
{
	dest += fDestBPR * y + x * fBPP;
	src += fSrcBPR * y + x * fBPP;
	switch (fBPP) {
		case 4:
			// dest[3] = ~src[3]; DON'T invert alpha channel
		case 3:
			dest[2] = ~src[2];
		case 2:
			dest[1] = ~src[1];
		case 1:
			dest[0] = ~src[0];
			break;
	}
}


// Note: On my systems, the operation kInvert shows a speedup on
// multiple CPUs only!
void
ImageProcessor::Run(int32 i, int32 n)
{
	int32 from, to;
	int32 height = (fHeight + 1) / n;
	from = i * height;
	if (i + 1 == n)
		to = fHeight;
	else
		to = from + height - 1;

	int32 x, y, destX, destY;
	const uchar* src = (uchar*)GetSrcImage()->Bits();
	uchar* dest = (uchar*)GetDestImage()->Bits();

	switch (fOp) {
		case kRotateClockwise:
			for (y = from; y <= to; y++) {
				for (x = 0; x <= fWidth; x++) {
					destX = fHeight - y;
					destY = x;
					CopyPixel(dest, destX, destY, src, x, y);
				}
			}
			break;
		case kRotateCounterClockwise:
			for (y = from; y <= to; y ++) {
				for (x = 0; x <= fWidth; x ++) {
					destX = y;
					destY = fWidth - x;
					CopyPixel(dest, destX, destY, src, x, y);
				}
			}
			break;
		case kFlipTopToBottom:
			for (y = from; y <= to; y ++) {
				for (x = 0; x <= fWidth; x ++) {
					destX = x;
					destY = fHeight - y;
					CopyPixel(dest, destX, destY, src, x, y);
				}
			}
			break;
		case kFlipLeftToRight:
			for (y = from; y <= to; y ++) {
				for (x = 0; x <= fWidth; x ++) {
					destX = fWidth - x;
					destY = y;
					CopyPixel(dest, destX, destY, src, x, y);
				}
			}
			break;
		case kInvert:
			for (y = from; y <= to; y ++) {
				for (x = 0; x <= fWidth; x ++) {
					InvertPixel(x, y, dest, src);
				}
			}
			break;
	}
}

/*
 * GraphicsDriver.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <algorithm>
#include <cstdio>
#include <cstdarg>

#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Message.h>
#include <PrintJob.h>
#include <Region.h>
#include <TextControl.h>
#include <TextControl.h>
#include <StopWatch.h>
#include <View.h>
#include <Directory.h>
#include <File.h>

#include "GraphicsDriver.h"
#include "PrintProcess.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PrinterCap.h"
#include "Preview.h"
#include "Transport.h"
#include "ValidRect.h"
#include "DbgMsg.h"


using namespace std;


// Measure printJob() time. Either true or false.
#define MEASURE_PRINT_JOB_TIME false


GraphicsDriver::GraphicsDriver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	fMessage(message),
	fView(NULL),
	fBitmap(NULL),
	fRotatedBitmap(NULL),
	fTransport(NULL),
	fOrgJobData(NULL),
	fRealJobData(NULL),
	fPrinterData(printerData),
	fPrinterCap(printerCap),
	fSpoolMetaData(NULL),
	fPageWidth(0),
	fPageHeight(0),
	fBandWidth(0),
	fBandHeight(0),
	fPixelDepth(0),
	fBandCount(0),
	fInternalCopies(0),
	fPageCount(0),
	fStatusWindow(NULL)
{
}


GraphicsDriver::~GraphicsDriver()
{
}


static BRect
RotateRect(BRect rect)
{
	BRect rotated(rect.top, rect.left, rect.bottom, rect.right);
	return rotated;
}


bool 
GraphicsDriver::_SetupData(BFile* spoolFile)
{
	if (fOrgJobData != NULL) {
		// already initialized
		return true;
	}

	print_file_header pfh;
	spoolFile->Seek(0, SEEK_SET);
	spoolFile->Read(&pfh, sizeof(pfh));

	DBGMSG(("print_file_header::version = 0x%x\n",  pfh.version));
	DBGMSG(("print_file_header::page_count = %d\n", pfh.page_count));
	DBGMSG(("print_file_header::first_page = 0x%x\n", (int)pfh.first_page));

	if (pfh.page_count <= 0) {
		// nothing to print
		return false;
	}

	fPageCount = (uint32) pfh.page_count;
	BMessage *msg = new BMessage();
	msg->Unflatten(spoolFile);
	fOrgJobData = new JobData(msg, fPrinterCap, JobData::kJobSettings);
	DUMP_BMESSAGE(msg);
	delete msg;

	fRealJobData = new JobData(*fOrgJobData);

	switch (fOrgJobData->GetNup()) {
	case 2:
	case 8:
	case 32:
	case 128:
		fRealJobData->SetPrintableRect(
			RotateRect(fOrgJobData->GetPrintableRect()));
		fRealJobData->SetScaledPrintableRect(
			RotateRect(fOrgJobData->GetScaledPrintableRect()));
		fRealJobData->SetPhysicalRect(
			RotateRect(fOrgJobData->GetPhysicalRect()));
		fRealJobData->SetScaledPhysicalRect(
			RotateRect(fOrgJobData->GetScaledPhysicalRect()));

		if (JobData::kPortrait == fOrgJobData->GetOrientation())
			fRealJobData->SetOrientation(JobData::kLandscape);
		else
			fRealJobData->SetOrientation(JobData::kPortrait);
		break;
	}

	if (fOrgJobData->GetCollate() && fPageCount > 1) {
		fRealJobData->SetCopies(1);
		fInternalCopies = fOrgJobData->GetCopies();
	} else {
		fInternalCopies = 1;
	}
	
	fSpoolMetaData = new SpoolMetaData(spoolFile);
	return true;
}


void 
GraphicsDriver::_CleanupData()
{
	delete fRealJobData;
	delete fOrgJobData;
	delete fSpoolMetaData;
	fRealJobData   = NULL;
	fOrgJobData    = NULL;
	fSpoolMetaData = NULL;
}


void 
GraphicsDriver::_SetupBitmap()
{
	fPixelDepth = color_space2pixel_depth(fOrgJobData->GetSurfaceType());

	fPageWidth  = (fRealJobData->GetPhysicalRect().IntegerWidth()
		* fOrgJobData->GetXres() + 71) / 72;
	fPageHeight = (fRealJobData->GetPhysicalRect().IntegerHeight()
		* fOrgJobData->GetYres() + 71) / 72;

	fBitmap = NULL;
	fRotatedBitmap = NULL;
	BRect rect;

	for (fBandCount = 1; fBandCount < 256; fBandCount++) {
		if (_NeedRotateBitmapBand()) {
			fBandWidth  = (fPageWidth + fBandCount - 1) / fBandCount;
			fBandHeight = fPageHeight;
		} else {
			fBandWidth  = fPageWidth;
			fBandHeight = (fPageHeight + fBandCount - 1) / fBandCount;
		}

		rect.Set(0, 0, fBandWidth - 1, fBandHeight - 1);
		fBitmap = new(std::nothrow) BBitmap(rect, fOrgJobData->GetSurfaceType(),
			true);
		if (fBitmap == NULL || fBitmap->InitCheck() != B_OK) {
			delete fBitmap;
			fBitmap = NULL;
			// Try with smaller bands
			continue;
		}

		if (_NeedRotateBitmapBand()) {
			BRect rotatedRect(0, 0, rect.bottom, rect.right);
			delete fRotatedBitmap;
			fRotatedBitmap = new(std::nothrow) BBitmap(rotatedRect,
				fOrgJobData->GetSurfaceType(), false);
			if (fRotatedBitmap == NULL || fRotatedBitmap->InitCheck() != B_OK) {
				delete fBitmap;
				fBitmap = NULL;
				delete fRotatedBitmap;
				fRotatedBitmap = NULL;

				// Try with smaller bands
				continue;
			}
		}

		// If we get here, all needed allocations have succeeded, we can safely
		// go ahead.
		break;
	};

	if (fBitmap == NULL) {
		debugger("Failed to allocate bitmaps for print rasterization");
		return;
	}

	fView = new BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW);
	fBitmap->AddChild(fView);

	DBGMSG(("****************\n"));
	DBGMSG(("page_width  = %d\n", fPageWidth));
	DBGMSG(("page_height = %d\n", fPageHeight));
	DBGMSG(("band_count  = %d\n", fBandCount));
	DBGMSG(("band_height = %d\n", fBandHeight));
	DBGMSG(("****************\n"));
}


void 
GraphicsDriver::_CleanupBitmap()
{
	delete fBitmap;
	fBitmap = NULL;
	fView   = NULL;

	delete fRotatedBitmap;
	fRotatedBitmap = NULL;
}


BPoint 
GraphicsDriver::GetScale(int32 nup, BRect physicalRect, float scaling)
{
	float width;
	float height;
	BPoint scale;

	scale.x = scale.y = 1.0f;

	switch (nup) {
	case 1:
		scale.x = scale.y = 1.0f;
		break;
	case 2:	/* 1x2 or 2x1 */
		width  = physicalRect.Width();
		height = physicalRect.Height();
		if (width < height) {	// portrait
			scale.x = height / 2.0f / width;
			scale.y = width / height;
		} else {	// landscape
			scale.x = height / width;
			scale.y = width / 2.0f / height;
		}
		break;
	case 8:	/* 2x4 or 4x2 */
		width  = physicalRect.Width();
		height = physicalRect.Height();
		if (width < height) {
			scale.x = height / 4.0f / width;
			scale.y = width / height / 2.0f;
		} else {
			scale.x = height / width / 2.0f;
			scale.y = width / 4.0f / height;
		}
		break;
	case 32:	/* 4x8 or 8x4 */
		width  = physicalRect.Width();
		height = physicalRect.Height();
		if (width < height) {
			scale.x = height / 8.0f / width;
			scale.y = width / height / 4.0f;
		} else {
			scale.x = height / width / 4.0f;
			scale.y = width / 8.0f / height;
		}
		break;
	case 4:		/* 2x2 */
		scale.x = scale.y = 1.0f / 2.0f;
		break;
	case 9:		/* 3x3 */
		scale.x = scale.y = 1.0f / 3.0f;
		break;
	case 16:	/* 4x4 */
		scale.x = scale.y = 1.0f / 4.0f;
		break;
	case 25:	/* 5x5 */
		scale.x = scale.y = 1.0f / 5.0f;
		break;
	case 36:	/* 6x6 */
		scale.x = scale.y = 1.0f / 6.0f;
		break;
	case 49:	/* 7x7 */
		scale.x = scale.y = 1.0f / 7.0f;
		break;
	case 64:	/* 8x8 */
		scale.x = scale.y = 1.0f / 8.0f;
		break;
	case 81:	/* 9x9 */
		scale.x = scale.y = 1.0f / 9.0f;
		break;
	case 100:	/* 10x10 */
		scale.x = scale.y = 1.0f / 10.0f;
		break;
	case 121:	/* 11x11 */
		scale.x = scale.y = 1.0f / 11.0f;
		break;
	}

	scale.x = scale.x * scaling / 100.0;
	scale.y = scale.y * scaling / 100.0;

	return scale;
}


BPoint 
GraphicsDriver::GetOffset(int32 nup, int index,
	JobData::Orientation orientation, const BPoint* scale,
	BRect scaledPhysicalRect, BRect scaledPrintableRect,
	BRect physicalRect)
{
	BPoint offset;
	offset.x = 0;
	offset.y = 0;

	float width  = scaledPhysicalRect.Width();
	float height = scaledPhysicalRect.Height();

	switch (nup) {
	case 1:
		break;
	case 2:
		if (index == 1) {
			if (JobData::kPortrait == orientation) {
				offset.x = width;
			} else {
				offset.y = height;
			}
		}
		break;
	case 8:
		if (JobData::kPortrait == orientation) {
			offset.x = width  * (index / 2);
			offset.y = height * (index % 2);
		} else {
			offset.x = width  * (index % 2);
			offset.y = height * (index / 2);
		}
		break;
	case 32:
		if (JobData::kPortrait == orientation) {
			offset.x = width  * (index / 4);
			offset.y = height * (index % 4);
		} else {
			offset.x = width  * (index % 4);
			offset.y = height * (index / 4);
		}
		break;
	case 4:
		offset.x = width  * (index / 2);
		offset.y = height * (index % 2);
		break;
	case 9:
		offset.x = width  * (index / 3);
		offset.y = height * (index % 3);
		break;
	case 16:
		offset.x = width  * (index / 4);
		offset.y = height * (index % 4);
		break;
	case 25:
		offset.x = width  * (index / 5);
		offset.y = height * (index % 5);
		break;
	case 36:
		offset.x = width  * (index / 6);
		offset.y = height * (index % 6);
		break;
	case 49:
		offset.x = width  * (index / 7);
		offset.y = height * (index % 7);
		break;
	case 64:
		offset.x = width  * (index / 8);
		offset.y = height * (index % 8);
		break;
	case 81:
		offset.x = width  * (index / 9);
		offset.y = height * (index % 9);
		break;
	case 100:
		offset.x = width  * (index / 10);
		offset.y = height * (index % 10);
		break;
	case 121:
		offset.x = width  * (index / 11);
		offset.y = height * (index % 11);
		break;
	}

	// adjust margin
	offset.x += scaledPrintableRect.left - physicalRect.left;
	offset.y += scaledPrintableRect.top - physicalRect.top;

	float real_scale = min(scale->x, scale->y);
	if (real_scale != scale->x)
		offset.x *= scale->x / real_scale;
	else
		offset.y *= scale->y / real_scale;

	return offset;
}


// print the specified pages on a physical page
bool 
GraphicsDriver::_PrintPage(PageDataList* pages)
{
	BPoint offset;
	offset.x = 0.0f;
	offset.y = 0.0f;

	if (pages == NULL) {
		return true;
	}

	do {
		// clear the physical page
		fView->SetScale(1.0);
		fView->SetHighColor(255, 255, 255);
		fView->ConstrainClippingRegion(NULL);
		fView->FillRect(fView->Bounds());

		BPoint scale = GetScale(fOrgJobData->GetNup(),
			fOrgJobData->GetPhysicalRect(), fOrgJobData->GetScaling());
		float real_scale = min(scale.x, scale.y) * fOrgJobData->GetXres()
			/ 72.0f;
		fView->SetScale(real_scale);
		float x = offset.x / real_scale;
		float y = offset.y / real_scale;
		int page_index = 0;

		for (PageDataList::iterator it = pages->begin(); it != pages->end();
			it++) {
			BPoint left_top(GetOffset(fOrgJobData->GetNup(), page_index++,
				fOrgJobData->GetOrientation(), &scale,
				fOrgJobData->GetScaledPhysicalRect(),
				fOrgJobData->GetScaledPrintableRect(),
				fOrgJobData->GetPhysicalRect()));

			left_top.x -= x;
			left_top.y -= y;

			BRect clip(fOrgJobData->GetScaledPrintableRect());
			clip.OffsetTo(left_top);

			BRegion *region = new BRegion();
			region->Set(clip);
			fView->ConstrainClippingRegion(region);
			delete region;

			if ((*it)->startEnum()) {
				bool more;
				do {
					PictureData	*picture_data;
					more = (*it)->enumObject(&picture_data);
					BPoint real_offset = left_top + picture_data->point;
					fView->DrawPicture(picture_data->picture, real_offset);
					fView->Sync();
					delete picture_data;
				} while (more);
			}
		}

		if (!_PrintBand(fBitmap, &offset))
			return false;

	} while (offset.x >= 0.0f && offset.y >= 0.0f);
	
	return true;
}


bool
GraphicsDriver::_PrintBand(BBitmap* band, BPoint* offset)
{
	if (!_NeedRotateBitmapBand())
		return NextBand(band, offset);

	_RotateInto(fRotatedBitmap, band);

	BPoint rotatedOffset(offset->y, offset->x);
	bool success = NextBand(fRotatedBitmap, &rotatedOffset);
	offset->x = rotatedOffset.y;
	offset->y = rotatedOffset.x;

	return success;
}


void
GraphicsDriver::_RotateInto(BBitmap* target, const BBitmap* source)
{
	ASSERT(target->ColorSpace() == B_RGB32);
	ASSERT(source->ColorSpace() == B_RGB32);
	ASSERT(target->Bounds().IntegerWidth() == source->Bounds().IntegerHeight());
	ASSERT(target->Bounds().IntegerHeight() == source->Bounds().IntegerWidth());

	const int32 width = source->Bounds().IntegerWidth() + 1;
	const int32 height = source->Bounds().IntegerHeight() + 1;

	const int32 sourceBPR = source->BytesPerRow();
	const int32 targetBPR = target->BytesPerRow();

	const uint8_t* sourceBits =
		reinterpret_cast<const uint8_t*>(source->Bits());
	uint8_t* targetBits = static_cast<uint8_t*>(target->Bits());

	for (int32 y = 0; y < height; y ++) {
		for (int32 x = 0; x < width; x ++) {
			const uint32_t* sourcePixel =
				reinterpret_cast<const uint32_t*>(sourceBits + sourceBPR * y
					+ sizeof(uint32_t) * x);

			int32 targetX = (height - y - 1);
			int32 targetY = x;
			uint32_t* targetPixel =
				reinterpret_cast<uint32_t*>(targetBits + targetBPR * targetY
					+ sizeof(uint32_t) * targetX);
			*targetPixel = *sourcePixel;
		}
	}
}

bool 
GraphicsDriver::_CollectPages(SpoolData* spoolData, PageDataList* pages)
{
	// collect the pages to be printed on the physical page
	PageData *page_data;
	int nup = fOrgJobData->GetNup();
	bool more;
	do {
		more = spoolData->enumObject(&page_data);
		if (pages != NULL)
			pages->push_back(page_data);
	} while (more && --nup);
	
	return more;
}


bool 
GraphicsDriver::_SkipPages(SpoolData* spoolData)
{
	return _CollectPages(spoolData, NULL);
}


bool 
GraphicsDriver::_PrintDocument(SpoolData* spoolData)
{
	bool more;
	bool success;
	int page_index;
	int copy;
	int copies;

	more = true;
	success = true;
	page_index = 0;
	
	if (fPrinterCap->Supports(PrinterCap::kCopyCommand))
		// let the printer perform the copy operation
		copies = 1;
	else
		// send the page multiple times to the printer
		copies = fRealJobData->GetCopies();

	fStatusWindow -> SetPageCopies(copies);
		// inform fStatusWindow about number of copies
	
	// printing of even/odd numbered pages only is valid in simplex mode
	bool simplex = fRealJobData->GetPrintStyle() == JobData::kSimplex;
	
	if (spoolData->startEnum()) {
		do {
			DBGMSG(("page index = %d\n", page_index));

			if (simplex
				&& fRealJobData->GetPageSelection()
					== JobData::kEvenNumberedPages)
				// skip odd numbered page
				more = _SkipPages(spoolData);

			if (!more)
				// end reached
				break;
			
			PageDataList pages;
			more = _CollectPages(spoolData, &pages);
			
			if (more && simplex
				&& fRealJobData->GetPageSelection()
					== JobData::kOddNumberedPages)
				// skip even numbered page
				more = _SkipPages(spoolData);

			// print each physical page "copies" of times
			for (copy = 0; success && copy < copies; copy ++) {

				// Update the status / cancel job
				if (fStatusWindow->UpdateStatusBar(page_index, copy))		
					return false;	

				success = StartPage(page_index);
				if (!success)
					break;
				
				// print the pages on the physical page
				fView->Window()->Lock();
				success = _PrintPage(&pages);
				fView->Window()->Unlock();

				if (success) {
					success = EndPage(page_index);
				}
			}
				
			page_index++;
		} while (success && more);
	}

#ifndef USE_PREVIEW_FOR_DEBUG
	if (success
		&& fPrinterCap->Supports(PrinterCap::kPrintStyle)
		&& (fOrgJobData->GetPrintStyle() != JobData::kSimplex)
		&& (((page_index + fOrgJobData->GetNup() - 1) / fOrgJobData->GetNup())
			% 2)) {
		// append an empty page on the back side of the page in duplex or
		// booklet mode
		success = 
			StartPage(page_index) &&
			_PrintPage(NULL) &&
			EndPage(page_index);
	}
#endif

	return success;
}


const JobData*
GraphicsDriver::GetJobData(BFile* spoolFile)
{
	_SetupData(spoolFile);
	return fOrgJobData;
}


bool 
GraphicsDriver::_PrintJob(BFile* spoolFile)
{
	bool success = true;
	if (!_SetupData(spoolFile)) {
		// silently exit if there is nothing to print
		return true;
	}

	fTransport = new Transport(fPrinterData);

	if (fTransport->CheckAbort()) {
		success = false;
	} else if (!fTransport->IsPrintToFileCanceled()) {
		BStopWatch stopWatch("printJob", !MEASURE_PRINT_JOB_TIME);
		_SetupBitmap();
		SpoolData spoolData(spoolFile, fPageCount, fOrgJobData->GetNup(),
			fOrgJobData->GetReverse());
		success = StartDocument();
		if (success) {
			fStatusWindow = new StatusWindow(
				fRealJobData->GetPageSelection() == JobData::kOddNumberedPages,
				fRealJobData->GetPageSelection() == JobData::kEvenNumberedPages,
				fRealJobData->GetFirstPage(),
				fPageCount, 
				fInternalCopies,fRealJobData->GetNup());
				
			while (fInternalCopies--) {
				success = _PrintDocument(&spoolData);
				if (success == false) {
					break;
				}
			}
			EndDocument(success);
		
			fStatusWindow->Lock();
			fStatusWindow->Quit();
		}
		_CleanupBitmap();
		_CleanupData();
	}

	if (success == false) {
		BAlert *alert;
		if (fTransport->CheckAbort())
			alert = new BAlert("", fTransport->LastError().c_str(), "OK");
		else
			alert = new BAlert("", "Printer not responding.", "OK");
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}

	delete fTransport;
	fTransport = NULL;

	return success;
}


BMessage*
GraphicsDriver::TakeJob(BFile* spoolFile)
{
	BMessage *msg;
	if (_PrintJob(spoolFile))
		msg = new BMessage('okok');
	else
		msg = new BMessage('baad');
	return msg;
}


bool 
GraphicsDriver::StartDocument()
{
	return true;
}


bool 
GraphicsDriver::StartPage(int)
{
	return true;
}


bool 
GraphicsDriver::NextBand(BBitmap*, BPoint*)
{
	return true;
}


bool 
GraphicsDriver::EndPage(int)
{
	return true;
}


bool 
GraphicsDriver::EndDocument(bool)
{
	return true;
}


void
GraphicsDriver::WriteSpoolData(const void* buffer, size_t size)
{
	if (fTransport == NULL)
		return;
	fTransport->Write(buffer, size);
}


void
GraphicsDriver::WriteSpoolString(const char* format, ...)
{
	if (fTransport == NULL)
		return;

	char buffer[256];
	va_list	ap;
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	fTransport->Write(buffer, strlen(buffer));
	va_end(ap);
}


void
GraphicsDriver::WriteSpoolChar(char c)
{
	if (fTransport == NULL)
		return;

	fTransport->Write(&c, 1);
}


void
GraphicsDriver::ReadSpoolData(void* buffer, size_t size)
{
	if (fTransport == NULL)
		return;
	fTransport->Read(buffer, size);
}


int
GraphicsDriver::ReadSpoolChar()
{
	if (fTransport == NULL)
		return -1;

	char c;
	fTransport->Read(&c, 1);
	return c;
}


bool
GraphicsDriver::_NeedRotateBitmapBand() const
{
	return JobData::kLandscape == fRealJobData->GetOrientation()
		&& !fPrinterCap->Supports(PrinterCap::kCanRotatePageInLandscape);
}


void 
GraphicsDriver::_ConvertRGB32ToRGB24(const void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	const rgb_color* s = static_cast<const rgb_color*>(src);
	for (int i = width; i > 0; i --) {
		*d ++ = s->red;
		*d ++ = s->green;
		*d ++ = s->blue;
		s++;
	}
}


void 
GraphicsDriver::_ConvertCMAP8ToRGB24(const void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	const uint8* s = static_cast<const uint8*>(src);
	const color_map* cmap = system_colors();
	for (int i = width; i > 0; i --) {
		const rgb_color* rgb = &cmap->color_list[*s];
		*d ++ = rgb->red;
		*d ++ = rgb->green;
		*d ++ = rgb->blue;
		s ++;		
	}
}


void 
GraphicsDriver::ConvertToRGB24(const void* src, void* dst, int width,
	color_space cs) {
	if (cs == B_RGB32)
		_ConvertRGB32ToRGB24(src, dst, width);
	else if (cs == B_CMAP8)
		_ConvertCMAP8ToRGB24(src, dst, width);
	else {
		DBGMSG(("color_space %d not supported", cs));
	}
}


uint8 
GraphicsDriver::_ConvertToGray(uint8 r, uint8 g, uint8 b) {
	if (r == g && g == b)
		return r;
	else
		return (r * 3 + g * 6 + b) / 10;
}


void 
GraphicsDriver::_ConvertRGB32ToGray(const void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	const rgb_color* s = static_cast<const rgb_color*>(src);
	for (int i = width; i > 0; i--, s++, d++)
		*d = _ConvertToGray(s->red, s->green, s->blue);
}


void 
GraphicsDriver::_ConvertCMAP8ToGray(const void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	const uint8* s = static_cast<const uint8*>(src);
	const color_map* cmap = system_colors();
	for (int i = width; i > 0; i--, s++, d++) {
		const rgb_color* rgb = &cmap->color_list[*s];
		*d = _ConvertToGray(rgb->red, rgb->green, rgb->blue);
	}
}


void 
GraphicsDriver::ConvertToGray(const void* src, void* dst, int width,
	color_space cs) {
	if (cs == B_RGB32)
		_ConvertRGB32ToGray(src, dst, width);
	else if (cs == B_CMAP8)
		_ConvertCMAP8ToGray(src, dst, width);
	else {
		DBGMSG(("color_space %d not supported", cs));
	}
}


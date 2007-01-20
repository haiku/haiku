/*
 * GraphicsDriver.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <algorithm>
#include <cstdio>
#include <cstdarg>

#include <Alert.h>
#include <Bitmap.h>
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

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

// Measure printJob() time. Either true or false.
#define MEASURE_PRINT_JOB_TIME false

enum {
	kMaxMemorySize = (4 *1024 *1024)
};

GraphicsDriver::GraphicsDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: fMsg(msg)
	, fPrinterData(printer_data)
	, fPrinterCap(printer_cap)
{
	fView          = NULL;
	fBitmap        = NULL;
	fTransport     = NULL;
	fOrgJobData    = NULL;
	fRealJobData   = NULL;
	fSpoolMetaData = NULL;
}

GraphicsDriver::~GraphicsDriver()
{
}


static BRect RotateRect(BRect rect)
{
	BRect rotated(rect.top, rect.left, rect.bottom, rect.right);
	return rotated;
}

bool 
GraphicsDriver::setupData(BFile *spool_file)
{
	if (fOrgJobData != NULL) {
		// already initialized
		return true;
	}
			
	print_file_header pfh;

	spool_file->Seek(0, SEEK_SET);
	spool_file->Read(&pfh, sizeof(pfh));

	DBGMSG(("print_file_header::version = 0x%x\n",  pfh.version));
	DBGMSG(("print_file_header::page_count = %d\n", pfh.page_count));
	DBGMSG(("print_file_header::first_page = 0x%x\n", (int)pfh.first_page));

	if (pfh.page_count <= 0) {
		// nothing to print
		return false;
	}

	fPageCount = (uint32) pfh.page_count;
	BMessage *msg = new BMessage();
	msg->Unflatten(spool_file);
	fOrgJobData = new JobData(msg, fPrinterCap, JobData::kJobSettings);
	DUMP_BMESSAGE(msg);
	delete msg;

	fRealJobData = new JobData(*fOrgJobData);

	switch (fOrgJobData->getNup()) {
	case 2:
	case 8:
	case 32:
	case 128:
		fRealJobData->setPrintableRect(RotateRect(fOrgJobData->getPrintableRect()));
		fRealJobData->setScaledPrintableRect(RotateRect(fOrgJobData->getScaledPrintableRect()));
		fRealJobData->setPhysicalRect(RotateRect(fOrgJobData->getPhysicalRect()));
		fRealJobData->setScaledPhysicalRect(RotateRect(fOrgJobData->getScaledPhysicalRect()));
		if (JobData::kPortrait == fOrgJobData->getOrientation())
			fRealJobData->setOrientation(JobData::kLandscape);
		else
			fRealJobData->setOrientation(JobData::kPortrait);
		break;
	}

	if (fOrgJobData->getCollate() && fPageCount > 1) {
		fRealJobData->setCopies(1);
		fInternalCopies = fOrgJobData->getCopies();
	} else {
		fInternalCopies = 1;
	}
	
	fSpoolMetaData = new SpoolMetaData(spool_file);
	return true;
}

void 
GraphicsDriver::cleanupData()
{
	delete fRealJobData;
	delete fOrgJobData;
	delete fSpoolMetaData;
	fRealJobData   = NULL;
	fOrgJobData    = NULL;
	fSpoolMetaData = NULL;
}

void 
GraphicsDriver::setupBitmap()
{
	fPixelDepth = color_space2pixel_depth(fOrgJobData->getSurfaceType());

	fPageWidth  = (fRealJobData->getPhysicalRect().IntegerWidth()  * fOrgJobData->getXres() + 71) / 72;
	fPageHeight = (fRealJobData->getPhysicalRect().IntegerHeight() * fOrgJobData->getYres() + 71) / 72;

	int widthByte = (fPageWidth * fPixelDepth + 7) / 8;
	int size = widthByte * fPageHeight;
#ifdef USE_PREVIEW_FOR_DEBUG
	size = 0;
#endif

	if (size < kMaxMemorySize) {
		fBandCount  = 0;
		fBandWidth  = fPageWidth;
		fBandHeight = fPageHeight;
	} else {
		fBandCount  = (size + kMaxMemorySize - 1) / kMaxMemorySize;
		if ((JobData::kLandscape == fRealJobData->getOrientation()) && (fFlags & kGDFRotateBandBitmap)) {
			fBandWidth  = (fPageWidth + fBandCount - 1) / fBandCount;
			fBandHeight = fPageHeight;
		} else {
			fBandWidth  = fPageWidth;
			fBandHeight = (fPageHeight + fBandCount - 1) / fBandCount;
		}
	}

	DBGMSG(("****************\n"));
	DBGMSG(("page_width  = %d\n", fPageWidth));
	DBGMSG(("page_height = %d\n", fPageHeight));
	DBGMSG(("band_count  = %d\n", fBandCount));
	DBGMSG(("band_height = %d\n", fBandHeight));
	DBGMSG(("****************\n"));

	BRect rect;
	rect.Set(0, 0, fBandWidth - 1, fBandHeight - 1);
	fBitmap = new BBitmap(rect, fOrgJobData->getSurfaceType(), true);
	fView   = new BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW);
	fBitmap->AddChild(fView);
}

void 
GraphicsDriver::cleanupBitmap()
{
	delete fBitmap;
	fBitmap = NULL;
	fView   = NULL;
}

BPoint 
GraphicsDriver::getScale(int32 nup, BRect physicalRect, float scaling)
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
GraphicsDriver::getOffset(int32 nup, int index, JobData::Orientation orientation, 
	const BPoint *scale, BRect scaledPhysicalRect, BRect scaledPrintableRect,
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
GraphicsDriver::printPage(PageDataList *pages)
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

		BPoint scale = getScale(fOrgJobData->getNup(), fOrgJobData->getPhysicalRect(), fOrgJobData->getScaling());
		float real_scale = min(scale.x, scale.y) * fOrgJobData->getXres() / 72.0f;
		fView->SetScale(real_scale);
		float x = offset.x / real_scale;
		float y = offset.y / real_scale;
		int page_index = 0;

		for (PageDataList::iterator it = pages->begin(); it != pages->end(); it++) {
			BPoint left_top(getOffset(fOrgJobData->getNup(), page_index++, fOrgJobData->getOrientation(), &scale, fOrgJobData->getScaledPhysicalRect(), fOrgJobData->getScaledPrintableRect(), fOrgJobData->getPhysicalRect()));
			left_top.x -= x;
			left_top.y -= y;
			BRect clip(fOrgJobData->getScaledPrintableRect());
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
		if (!nextBand(fBitmap, &offset)) {
			return false;
		}
	} while (offset.x >= 0.0f && offset.y >= 0.0f);
	
	return true;
}

bool 
GraphicsDriver::collectPages(SpoolData *spool_data, PageDataList *pages)
{
	// collect the pages to be printed on the physical page
	PageData *page_data;
	int nup = fOrgJobData->getNup();
	bool more;
	do {
		more = spool_data->enumObject(&page_data);
		if (pages != NULL) {
			pages->push_back(page_data);
		}
	} while (more && --nup);
	
	return more;
}

bool 
GraphicsDriver::skipPages(SpoolData *spool_data)
{
	return collectPages(spool_data, NULL);
}

bool 
GraphicsDriver::printDocument(SpoolData *spool_data)
{
	bool more;
	bool success;
	int page_index;
	int copy;
	int copies;

	more = true;
	success = true;
	page_index = 0;
	
	if (fPrinterCap->isSupport(PrinterCap::kCopyCommand)) {
		// let the printer perform the copy operation
		copies = 1;
	} else {
		// send the page multiple times to the printer
		copies = fRealJobData->getCopies();
	}
	fStatusWindow -> SetPageCopies(copies);						// inform fStatusWindow about number of copies
	
	// printing of even/odd numbered pages only is valid in simplex mode
	bool simplex = fRealJobData->getPrintStyle() == JobData::kSimplex;
	
	if (spool_data->startEnum()) {
		do {
			DBGMSG(("page index = %d\n", page_index));

			if (simplex && 
				fRealJobData->getPageSelection() == JobData::kEvenNumberedPages) {
				// skip odd numbered page
				more = skipPages(spool_data);
			}

			if (!more) {
				// end reached
				break;
			}
			
			PageDataList pages;
			more = collectPages(spool_data, &pages);
			
			if (more && simplex && 
				fRealJobData->getPageSelection() == JobData::kOddNumberedPages) {
				// skip even numbered page
				more = skipPages(spool_data);
			}

			// print each physical page "copies" of times
			for (copy = 0; success && copy < copies; copy ++) {

				// Update the status / cancel job
				if (fStatusWindow->UpdateStatusBar(page_index, copy))		
					return false;	

				success = startPage(page_index);
				if (!success)
					break;
				
				// print the pages on the physical page
				fView->Window()->Lock();
				success = printPage(&pages);
				fView->Window()->Unlock();

				if (success) {
					success = endPage(page_index);
				}
			}
				
			page_index++;
		} while (success && more);
	}

#ifndef USE_PREVIEW_FOR_DEBUG
	if (success
		&& fPrinterCap->isSupport(PrinterCap::kPrintStyle)
		&& (fOrgJobData->getPrintStyle() != JobData::kSimplex)
		&& (((page_index + fOrgJobData->getNup() - 1) / fOrgJobData->getNup()) % 2))
	{
		// append an empty page on the back side of the page in duplex or booklet mode
		success = 
			startPage(page_index) &&
			printPage(NULL) &&
			endPage(page_index);
	}
#endif

	return success;
}

const JobData*
GraphicsDriver::getJobData(BFile *spoolFile)
{
	setupData(spoolFile);
	return fOrgJobData;
}

bool 
GraphicsDriver::printJob(BFile *spool_file)
{
	bool success = true;
	if (!setupData(spool_file)) {
		// silently exit if there is nothing to print
		return true;
	}

	fTransport = new Transport(fPrinterData);

	if (fTransport->check_abort()) {
		success = false;
	} else if (!fTransport->is_print_to_file_canceled()) {
		BStopWatch stopWatch("printJob", !MEASURE_PRINT_JOB_TIME);
		setupBitmap();
		SpoolData spool_data(spool_file, fPageCount, fOrgJobData->getNup(), fOrgJobData->getReverse());
		success = startDoc();
		if (success) {
			fStatusWindow = new StatusWindow(
				fRealJobData->getPageSelection() == JobData::kOddNumberedPages,
				fRealJobData->getPageSelection() == JobData::kEvenNumberedPages, 												
				fRealJobData->getFirstPage(),
				fPageCount, 
				fInternalCopies,fRealJobData->getNup());
				
			while (fInternalCopies--) {
				success = printDocument(&spool_data);
				if (success == false) {
					break;
				}
			}
			endDoc(success);
		
			fStatusWindow -> Quit();
		}
		cleanupBitmap();
		cleanupData();
	}

	if (success == false) {
		BAlert *alert;
		if (fTransport->check_abort()) {
			alert = new BAlert("", fTransport->last_error().c_str(), "OK");
		} else {
			alert = new BAlert("", "Printer not responding.", "OK");
		}
		alert->Go();
	}

	delete fTransport;
	fTransport = NULL;

	return success;
}

BMessage *
GraphicsDriver::takeJob(BFile* spool_file, uint32 flags)
{
	fFlags = flags;
	BMessage *msg;
	if (printJob(spool_file)) {
		msg = new BMessage('okok');
	} else {
		msg = new BMessage('baad');
	}
	return msg;
}

bool 
GraphicsDriver::startDoc()
{
	return true;
}

bool 
GraphicsDriver::startPage(int)
{
	return true;
}

bool 
GraphicsDriver::nextBand(BBitmap *, BPoint *)
{
	return true;
}

bool 
GraphicsDriver::endPage(int)
{
	return true;
}

bool 
GraphicsDriver::endDoc(bool)
{
	return true;
}

void 
GraphicsDriver::writeSpoolData(const void *buffer, size_t size) throw(TransportException)
{
	if (fTransport) {
		fTransport->write(buffer, size);
	}
}

void 
GraphicsDriver::writeSpoolString(const char *format, ...) throw(TransportException)
{
	if (fTransport) {
		char buffer[256];
		va_list	ap;
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		fTransport->write(buffer, strlen(buffer));
		va_end(ap);
	}
}

void 
GraphicsDriver::writeSpoolChar(char c) throw(TransportException)
{
	if (fTransport) {
		fTransport->write(&c, 1);
	}
}


void 
GraphicsDriver::rgb32_to_rgb24(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	rgb_color* s = (rgb_color*)src;
	for (int i = width; i > 0; i --) {
		*d ++ = s->red;
		*d ++ = s->green;
		*d ++ = s->blue;
		s++;
	}
}

void 
GraphicsDriver::cmap8_to_rgb24(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	uint8* s = (uint8*)src;
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
GraphicsDriver::convert_to_rgb24(void* src, void* dst, int width, color_space cs) {
	if (cs == B_RGB32) rgb32_to_rgb24(src, dst, width);
	else if (cs == B_CMAP8) cmap8_to_rgb24(src, dst, width);
	else {
		DBGMSG(("color_space %d not supported", cs));
	}
}

uint8 
GraphicsDriver::gray(uint8 r, uint8 g, uint8 b) {
	if (r == g && g == b) {
		return r;
	} else {
		return (r * 3 + g * 6 + b) / 10;
	}
}


void 
GraphicsDriver::rgb32_to_gray(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	rgb_color* s = (rgb_color*)src;
	for (int i = width; i > 0; i --) {
		*d ++ = gray(s->red, s->green, s->blue);
		s++;
	}
}

void 
GraphicsDriver::cmap8_to_gray(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	uint8* s = (uint8*)src;
	const color_map* cmap = system_colors();
	for (int i = width; i > 0; i --) {
		const rgb_color* rgb = &cmap->color_list[*s];
		*d ++ = gray(rgb->red, rgb->green, rgb->blue);
		s ++;		
	}
}

void 
GraphicsDriver::convert_to_gray(void* src, void* dst, int width, color_space cs) {
	if (cs == B_RGB32) rgb32_to_gray(src, dst, width);
	else if (cs == B_CMAP8) cmap8_to_gray(src, dst, width);
	else {
		DBGMSG(("color_space %d not supported", cs));
	}
}


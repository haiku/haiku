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

enum {
	kMaxMemorySize = (4 *1024 *1024)
};

GraphicsDriver::GraphicsDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: fMsg(msg), fPrinterData(printer_data), fPrinterCap(printer_cap)
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

void GraphicsDriver::setupData(BFile *spool_file, long page_count)
{
	BMessage *msg = new BMessage();
	msg->Unflatten(spool_file);
	fOrgJobData = new JobData(msg, fPrinterCap);
	DUMP_BMESSAGE(msg);
	delete msg;

	fRealJobData = new JobData(*fOrgJobData);
	BRect rc;

	switch (fOrgJobData->getNup()) {
	case 2:
	case 8:
	case 32:
	case 128:
		rc.left   = fOrgJobData->getPrintableRect().top;
		rc.top    = fOrgJobData->getPrintableRect().left;
		rc.right  = fOrgJobData->getPrintableRect().bottom;
		rc.bottom = fOrgJobData->getPrintableRect().right;
		fRealJobData->setPrintableRect(rc);
		if (JobData::kPortrait == fOrgJobData->getOrientation())
			fRealJobData->setOrientation(JobData::kLandscape);
		else
			fRealJobData->setOrientation(JobData::kPortrait);
		break;
	}

	if (fOrgJobData->getCollate() && page_count > 1) {
		fRealJobData->setCopies(1);
		fInternalCopies = fOrgJobData->getCopies();
	} else {
		fInternalCopies = 1;
	}
	
	fSpoolMetaData = new SpoolMetaData(spool_file);
}

void GraphicsDriver::cleanupData()
{
	delete fRealJobData;
	delete fOrgJobData;
	delete fSpoolMetaData;
	fRealJobData   = NULL;
	fOrgJobData    = NULL;
	fSpoolMetaData = NULL;
}

void GraphicsDriver::setupBitmap()
{
	fPixelDepth = color_space2pixel_depth(fOrgJobData->getSurfaceType());

	fPageWidth  = (fRealJobData->getPrintableRect().IntegerWidth()  * fOrgJobData->getXres() + 71) / 72;
	fPageHeight = (fRealJobData->getPrintableRect().IntegerHeight() * fOrgJobData->getYres() + 71) / 72;

	int widthByte = (fPageWidth * fPixelDepth + 7) / 8;
	int size = widthByte * fPageHeight;

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

void GraphicsDriver::cleanupBitmap()
{
	delete fBitmap;
	fBitmap = NULL;
	fView   = NULL;
}

BPoint get_scale(JobData *org_job_data)
{
	float width;
	float height;
	BPoint scale;

	scale.x = scale.y = 1.0f;

	switch (org_job_data->getNup()) {
	case 1:
		scale.x = scale.y = 1.0f;
		break;
	case 2:	/* 1x2 or 2x1 */
		width  = org_job_data->getPrintableRect().Width();
		height = org_job_data->getPrintableRect().Height();
		if (width < height) {	// portrait
			scale.x = height / 2.0f / width;
			scale.y = width / height;
		} else {	// landscape
			scale.x = height / width;
			scale.y = width / 2.0f / height;
		}
		break;
	case 8:	/* 2x4 or 4x2 */
		width  = org_job_data->getPrintableRect().Width();
		height = org_job_data->getPrintableRect().Height();
		if (width < height) {
			scale.x = height / 4.0f / width;
			scale.y = width / height / 2.0f;
		} else {
			scale.x = height / width / 2.0f;
			scale.y = width / 4.0f / height;
		}
		break;
	case 32:	/* 4x8 or 8x4 */
		width  = org_job_data->getPrintableRect().Width();
		height = org_job_data->getPrintableRect().Height();
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

	DBGMSG(("real scale = %f, %f\n", scale.x, scale.y));
	return scale;
}

BPoint get_offset(
	int index,
	const BPoint *scale,
	JobData *org_job_data)
{
	BPoint offset;
	offset.x = 0;
	offset.y = 0;

	float width  = org_job_data->getPrintableRect().Width();
	float height = org_job_data->getPrintableRect().Height();

	switch (org_job_data->getNup()) {
	case 1:
		break;
	case 2:
		if (index == 1) {
			if (JobData::kPortrait == org_job_data->getOrientation()) {
				offset.x = width;
			} else {
				offset.y = height;
			}
		}
		break;
	case 8:
		if (JobData::kPortrait == org_job_data->getOrientation()) {
			offset.x = width  * (index / 2);
			offset.y = height * (index % 2);
		} else {
			offset.x = width  * (index % 2);
			offset.y = height * (index / 2);
		}
		break;
	case 32:
		if (JobData::kPortrait == org_job_data->getOrientation()) {
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

	float real_scale = min(scale->x, scale->y);
	if (real_scale != scale->x)
		offset.x *= scale->x / real_scale;
	else
		offset.y *= scale->y / real_scale;

	return offset;
}

bool GraphicsDriver::printPage(PageDataList *pages)
{
#ifndef USE_PREVIEW_FOR_DEBUG
	BPoint offset;
	offset.x = 0.0f;
	offset.y = 0.0f;

	if (pages == NULL) {
		return nextBand(NULL, &offset);
	}

	do {
		fView->SetScale(1.0);
		fView->SetHighColor(255, 255, 255);
		fView->ConstrainClippingRegion(NULL);
		fView->FillRect(fView->Bounds());

		BPoint scale = get_scale(fOrgJobData);
		float real_scale = min(scale.x, scale.y) * fOrgJobData->getXres() / 72.0f;
		fView->SetScale(real_scale);
		float x = offset.x / real_scale;
		float y = offset.y / real_scale;
		int page_index = 0;

		for (PageDataList::iterator it = pages->begin(); it != pages->end(); it++) {
			BPoint left_top(get_offset(page_index++, &scale, fOrgJobData));
			left_top.x -= x;
			left_top.y -= y;
			BRect clip(fOrgJobData->getPrintableRect());
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
#else	// !USE_PREVIEW_FOR_DEBUG
	fView->SetScale(1.0);
	fView->SetHighColor(255, 255, 255);
	fView->ConstrainClippingRegion(NULL);
	fView->FillRect(fView->Bounds());

	BPoint scale = get_scale(fOrgJobData);
	fView->SetScale(min(scale.x, scale.y));

	int page_index = 0;
	PageDataList::iterator it;

	for (it = pages->begin() ; it != pages->end() ; it++) {
		BPoint left_top(get_offset(page_index, &scale, fOrgJobData));
		BRect clip(fOrgJobData->getPrintableRect());
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
		page_index++;
	}

	BRect rc(fRealJobData->getPrintableRect());
	rc.OffsetTo(30.0, 30.0);
	PreviewWindow *preview = new PreviewWindow(rc, "Preview", fBitmap);
	preview->Go();
#endif	// USE_PREVIEW_FOR_DEBUG
}

bool GraphicsDriver::printDocument(SpoolData *spool_data)
{
	bool more;
	bool success;
	PageData *page_data;
	int page_index;
	int nup;

	more = false;
	success = true;
	page_index = 0;

	if (spool_data->startEnum()) {
		do {
			DBGMSG(("page index = %d\n", page_index));
			if (!(success = startPage(page_index)))
				break;
			nup = fOrgJobData->getNup();
			PageDataList pages;
			do {
				more = spool_data->enumObject(&page_data);
				pages.push_back(page_data);
			} while (more && --nup);
			fView->Window()->Lock();
			success = printPage(&pages);
			fView->Window()->Unlock();
			if (!success)
				break;
			if (!(success = endPage(page_index)))
				break;
			page_index++;
		} while (more);
	}

#ifndef USE_PREVIEW_FOR_DEBUG
	if (success
		&& fPrinterCap->isSupport(PrinterCap::kPrintStyle)
		&& (fOrgJobData->getPrintStyle() != JobData::kSimplex)
		&& (((page_index + fOrgJobData->getNup() - 1) / fOrgJobData->getNup()) % 2))
	{
		success = startPage(page_index);
		if (success) {
			success = printPage(NULL);
			if (success) {
				success = endPage(page_index);
			}
		}
	}
#endif

	return success;
}

bool GraphicsDriver::printJob(BFile *spool_file)
{
	bool success = true;
	print_file_header pfh;

	spool_file->Seek(0, SEEK_SET);
	spool_file->Read(&pfh, sizeof(pfh));

	DBGMSG(("print_file_header::version = 0x%x\n",  pfh.version));
	DBGMSG(("print_file_header::page_count = %d\n", pfh.page_count));
	DBGMSG(("print_file_header::first_page = 0x%x\n", (int)pfh.first_page));

	if (pfh.page_count <= 0) {
		return true;
	}

	fTransport = new Transport(fPrinterData);

	if (fTransport->check_abort()) {
		success = false;
	} else {
		setupData(spool_file, pfh.page_count);
		setupBitmap();
		SpoolData spool_data(spool_file, pfh.page_count, fOrgJobData->getNup(), fOrgJobData->getReverse());
		success = startDoc();
		if (success) {
			while (fInternalCopies--) {
				success = printDocument(&spool_data);
				if (success == false) {
					break;
				}
			}
			endDoc(success);
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

BMessage *GraphicsDriver::takeJob(BFile* spool_file, uint32 flags)
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

bool GraphicsDriver::startDoc()
{
	return true;
}

bool GraphicsDriver::startPage(int)
{
	return true;
}

bool GraphicsDriver::nextBand(BBitmap *, BPoint *)
{
	return true;
}

bool GraphicsDriver::endPage(int)
{
	return true;
}

bool GraphicsDriver::endDoc(bool)
{
	return true;
}

void GraphicsDriver::writeSpoolData(const void *buffer, size_t size) throw(TransportException)
{
	if (fTransport) {
		fTransport->write(buffer, size);
	}
}

void GraphicsDriver::writeSpoolString(const char *format, ...) throw(TransportException)
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

void GraphicsDriver::writeSpoolChar(char c) throw(TransportException)
{
	if (fTransport) {
		fTransport->write(&c, 1);
	}
}


void GraphicsDriver::rgb32_to_rgb24(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	rgb_color* s = (rgb_color*)src;
	for (int i = width; i > 0; i --) {
		*d ++ = s->red;
		*d ++ = s->green;
		*d ++ = s->blue;
		s++;
	}
}

void GraphicsDriver::cmap8_to_rgb24(void* src, void* dst, int width) {
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

void GraphicsDriver::convert_to_rgb24(void* src, void* dst, int width, color_space cs) {
	if (cs == B_RGB32) rgb32_to_rgb24(src, dst, width);
	else if (cs == B_CMAP8) cmap8_to_rgb24(src, dst, width);
	else {
		DBGMSG(("color_space %d not supported", cs));
	}
}

uint8 GraphicsDriver::gray(uint8 r, uint8 g, uint8 b) {
	if (r == g && g == b) {
		return r;
	} else {
		return (r * 3 + g * 6 + b) / 10;
	}
}


void GraphicsDriver::rgb32_to_gray(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	rgb_color* s = (rgb_color*)src;
	for (int i = width; i > 0; i --) {
		*d ++ = gray(s->red, s->green, s->blue);
		s++;
	}
}

void GraphicsDriver::cmap8_to_gray(void* src, void* dst, int width) {
	uint8* d = (uint8*)dst;
	uint8* s = (uint8*)src;
	const color_map* cmap = system_colors();
	for (int i = width; i > 0; i --) {
		const rgb_color* rgb = &cmap->color_list[*s];
		*d ++ = gray(rgb->red, rgb->green, rgb->blue);
		s ++;		
	}
}

void GraphicsDriver::convert_to_gray(void* src, void* dst, int width, color_space cs) {
	if (cs == B_RGB32) rgb32_to_gray(src, dst, width);
	else if (cs == B_CMAP8) cmap8_to_gray(src, dst, width);
	else {
		DBGMSG(("color_space %d not supported", cs));
	}
}


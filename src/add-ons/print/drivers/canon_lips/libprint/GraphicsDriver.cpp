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

GraphicsDriver::GraphicsDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: __msg(msg), __printer_data(printer_data), __printer_cap(printer_cap)
{
	__view          = NULL;
	__bitmap        = NULL;
	__transport     = NULL;
	__org_job_data  = NULL;
	__real_job_data = NULL;
}

GraphicsDriver::~GraphicsDriver()
{
}

void GraphicsDriver::setupData(BFile *spool_file, long page_count)
{
	BMessage *msg = new BMessage();
	msg->Unflatten(spool_file);
	__org_job_data = new JobData(msg, __printer_cap);
	DUMP_BMESSAGE(msg);
	delete msg;

	__real_job_data = new JobData(*__org_job_data);
	BRect rc;

	switch (__org_job_data->getNup()) {
	case 2:
	case 8:
	case 32:
	case 128:
		rc.left   = __org_job_data->getPrintableRect().top;
		rc.top    = __org_job_data->getPrintableRect().left;
		rc.right  = __org_job_data->getPrintableRect().bottom;
		rc.bottom = __org_job_data->getPrintableRect().right;
		__real_job_data->setPrintableRect(rc);
		if (JobData::PORTRAIT == __org_job_data->getOrientation())
			__real_job_data->setOrientation(JobData::LANDSCAPE);
		else
			__real_job_data->setOrientation(JobData::PORTRAIT);
		break;
	}

	if (__org_job_data->getCollate() && page_count > 1) {
		__real_job_data->setCopies(1);
		__internal_copies = __org_job_data->getCopies();
	} else {
		__internal_copies = 1;
	}
}

void GraphicsDriver::cleanupData()
{
	delete __real_job_data;
	delete __org_job_data;
	__real_job_data = NULL;
	__org_job_data  = NULL;
}

#define MAX_MEMORY_SIZE		(4 *1024 *1024)

void GraphicsDriver::setupBitmap()
{
	__pixel_depth = color_space2pixel_depth(__org_job_data->getSurfaceType());

	__page_width  = (__real_job_data->getPrintableRect().IntegerWidth()  * __org_job_data->getXres() + 71) / 72;
	__page_height = (__real_job_data->getPrintableRect().IntegerHeight() * __org_job_data->getYres() + 71) / 72;

	int widthByte = (__page_width * __pixel_depth + 7) / 8;
	int size = widthByte * __page_height;

	if (size < MAX_MEMORY_SIZE) {
		__band_count  = 0;
		__band_width  = __page_width;
		__band_height = __page_height;
	} else {
		__band_count  = (size + MAX_MEMORY_SIZE - 1) / MAX_MEMORY_SIZE;
		if ((JobData::LANDSCAPE == __real_job_data->getOrientation()) && (__flags & GDF_ROTATE_BAND_BITMAP)) {
			__band_width  = (__page_width + __band_count - 1) / __band_count;
			__band_height = __page_height;
		} else {
			__band_width  = __page_width;
			__band_height = (__page_height + __band_count - 1) / __band_count;
		}
	}

	DBGMSG(("****************\n"));
	DBGMSG(("page_width  = %d\n", __page_width));
	DBGMSG(("page_height = %d\n", __page_height));
	DBGMSG(("band_count  = %d\n", __band_count));
	DBGMSG(("band_height = %d\n", __band_height));
	DBGMSG(("****************\n"));

	BRect rect;
	rect.Set(0, 0, __band_width - 1, __band_height - 1);
	__bitmap = new BBitmap(rect, __org_job_data->getSurfaceType(), true);
	__view   = new BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW);
	__bitmap->AddChild(__view);
}

void GraphicsDriver::cleanupBitmap()
{
	delete __bitmap;
	__bitmap = NULL;
	__view   = NULL;
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
			if (JobData::PORTRAIT == org_job_data->getOrientation()) {
				offset.x = width;
			} else {
				offset.y = height;
			}
		}
		break;
	case 8:
		if (JobData::PORTRAIT == org_job_data->getOrientation()) {
			offset.x = width  * (index / 2);
			offset.y = height * (index % 2);
		} else {
			offset.x = width  * (index % 2);
			offset.y = height * (index / 2);
		}
		break;
	case 32:
		if (JobData::PORTRAIT == org_job_data->getOrientation()) {
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
		__view->SetScale(1.0);
		__view->SetHighColor(255, 255, 255);
		__view->ConstrainClippingRegion(NULL);
		__view->FillRect(__view->Bounds());

		BPoint scale = get_scale(__org_job_data);
		float real_scale = min(scale.x, scale.y) * __org_job_data->getXres() / 72.0f;
		__view->SetScale(real_scale);
		float x = offset.x / real_scale;
		float y = offset.y / real_scale;
		int page_index = 0;

		for (PageDataList::iterator it = pages->begin(); it != pages->end(); it++) {
			BPoint left_top(get_offset(page_index++, &scale, __org_job_data));
			left_top.x -= x;
			left_top.y -= y;
			BRect clip(__org_job_data->getPrintableRect());
			clip.OffsetTo(left_top);
			BRegion *region = new BRegion();
			region->Set(clip);
			__view->ConstrainClippingRegion(region);
			delete region;
			if ((*it)->startEnum()) {
				bool more;
				do {
					PictureData	*picture_data;
					more = (*it)->enumObject(&picture_data);
					BPoint real_offset = left_top + picture_data->point;
					__view->DrawPicture(picture_data->picture, real_offset);
					__view->Sync();
					delete picture_data;
				} while (more);
			}
		}
		if (!nextBand(__bitmap, &offset)) {
			return false;
		}
	} while (offset.x >= 0.0f && offset.y >= 0.0f);

	return true;
#else	// !USE_PREVIEW_FOR_DEBUG
	__view->SetScale(1.0);
	__view->SetHighColor(255, 255, 255);
	__view->ConstrainClippingRegion(NULL);
	__view->FillRect(__view->Bounds());

	BPoint scale = get_scale(__org_job_data);
	__view->SetScale(min(scale.x, scale.y));

	int page_index = 0;
	PageDataList::iterator it;

	for (it = pages->begin() ; it != pages->end() ; it++) {
		BPoint left_top(get_offset(page_index, &scale, __org_job_data));
		BRect clip(__org_job_data->getPrintableRect());
		clip.OffsetTo(left_top);
		BRegion *region = new BRegion();
		region->Set(clip);
		__view->ConstrainClippingRegion(region);
		delete region;
		if ((*it)->startEnum()) {
			bool more;
			do {
				PictureData	*picture_data;
				more = (*it)->enumObject(&picture_data);
				BPoint real_offset = left_top + picture_data->point;
				__view->DrawPicture(picture_data->picture, real_offset);
				__view->Sync();
				delete picture_data;
			} while (more);
		}
		page_index++;
	}

	BRect rc(__real_job_data->getPrintableRect());
	rc.OffsetTo(30.0, 30.0);
	PreviewWindow *preview = new PreviewWindow(rc, "Preview", __bitmap);
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
			nup = __org_job_data->getNup();
			PageDataList pages;
			do {
				more = spool_data->enumObject(&page_data);
				pages.push_back(page_data);
			} while (more && --nup);
			__view->Window()->Lock();
			success = printPage(&pages);
			__view->Window()->Unlock();
			if (!success)
				break;
			if (!(success = endPage(page_index)))
				break;
			page_index++;
		} while (more);
	}

#ifndef USE_PREVIEW_FOR_DEBUG
	if (success
		&& __printer_cap->isSupport(PrinterCap::PRINTSTYLE)
		&& (__org_job_data->getPrintStyle() != JobData::SIMPLEX)
		&& (((page_index + __org_job_data->getNup() - 1) / __org_job_data->getNup()) % 2))
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

	__transport = new Transport(__printer_data);

	if (__transport->check_abort()) {
		success = false;
	} else {
		setupData(spool_file, pfh.page_count);
		setupBitmap();
		SpoolData spool_data(spool_file, pfh.page_count, __org_job_data->getNup(), __org_job_data->getReverse());
		success = startDoc();
		if (success) {
			while (__internal_copies--) {
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
		if (__transport->check_abort()) {
			alert = new BAlert("", __transport->last_error().c_str(), "OK");
		} else {
			alert = new BAlert("", "Printer not responding.", "OK");
		}
		alert->Go();
	}

	delete __transport;
	__transport = NULL;

	return success;
}

BMessage *GraphicsDriver::takeJob(BFile* spool_file, uint32 flags)
{
	__flags = flags;
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
	if (__transport) {
		__transport->write(buffer, size);
	}
}

void GraphicsDriver::writeSpoolString(const char *format, ...) throw(TransportException)
{
	if (__transport) {
		char buffer[256];
		va_list	ap;
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		__transport->write(buffer, strlen(buffer));
		va_end(ap);
	}
}

void GraphicsDriver::writeSpoolChar(char c) throw(TransportException)
{
	if (__transport) {
		__transport->write(&c, 1);
	}
}

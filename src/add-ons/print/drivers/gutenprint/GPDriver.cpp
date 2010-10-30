/*
 * GP.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2010 Michael Pfeiffer.
 */


#include "GPDriver.h"

#include <memory>

#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <File.h>

#include "DbgMsg.h"
#include "Halftone.h"
#include "JobData.h"
#include "PackBits.h"
#include "GPCapabilities.h"
#include "GPData.h"
#include "PrinterData.h"
#include "UIDriver.h"
#include "ValidRect.h"


using namespace std;


GPDriver::GPDriver(BMessage* msg, PrinterData* printer_data,
	const PrinterCap* printer_cap)
	:
	GraphicsDriver(msg, printer_data, printer_cap)
{
}

void
GPDriver::Write(const void *buffer, size_t size)
	throw(TransportException)
{
	writeSpoolData(buffer, size);
}

bool
GPDriver::startDoc()
{
	try {
		const GPData* data = dynamic_cast<const GPData*>(getPrinterData());
		ASSERT(data != NULL);
		fConfiguration.fDriver = data->fGutenprintDriverName;

		SetParameter(fConfiguration.fPageSize, PrinterCap::kPaper,
			getJobData()->getPaper());

		SetParameter(fConfiguration.fResolution, PrinterCap::kResolution,
			getJobData()->getResolutionID());

		fConfiguration.fXDPI = getJobData()->getXres();
		fConfiguration.fYDPI = getJobData()->getYres();

		SetParameter(fConfiguration.fInputSlot, PrinterCap::kPaperSource,
			getJobData()->getPaperSource());

		SetParameter(fConfiguration.fPrintingMode, PrinterCap::kColor,
			getJobData()->getColor());

		if (getPrinterCap()->isSupport(PrinterCap::kDriverSpecificCapabilities))
			SetDriverSpecificSettings();

		fprintf(stderr, "Driver: %s\n", fConfiguration.fDriver.String());
		fprintf(stderr, "PageSize %s\n", fConfiguration.fPageSize.String());
		fprintf(stderr, "Resolution %s\n", fConfiguration.fResolution.String());
		fprintf(stderr, "InputSlot %s\n", fConfiguration.fInputSlot.String());
		fprintf(stderr, "PrintingMode %s\n", fConfiguration.fPrintingMode.String());

		return fBinding.BeginJob(&fConfiguration, this) == B_OK;
	}
	catch (TransportException& err) {
		return false;
	} 
}


void
GPDriver::SetParameter(BString& parameter, PrinterCap::CapID category,
	int value)
{
	const BaseCap* capability;
	capability = getPrinterCap()->findCap(category, value);
	if (capability != NULL && capability->fKey != "")
		parameter = capability->Key();
}


void
GPDriver::SetDriverSpecificSettings()
{
	PrinterCap::CapID category = PrinterCap::kDriverSpecificCapabilities;
	int count = getPrinterCap()->countCap(category);
	const BaseCap** capabilities = getPrinterCap()->enumCap(category);
	for (int i = 0; i < count; i++) {
		const BaseCap* capability = capabilities[i];
		PrinterCap::CapID id = static_cast<PrinterCap::CapID>(capability->ID());
		AddDriverSpecificSetting(id, capability->fKey.c_str());
	}
}


void
GPDriver::AddDriverSpecificSetting(PrinterCap::CapID category, const char* key) {
	const BaseCap* capability = NULL;
	if (getJobData()->HasDriverSpecificSetting(key))
	{
		const string& value = getJobData()->DriverSpecificSetting(key);
		capability = getPrinterCap()->findCapWithKey(category, value.c_str());
	}

	if (capability == NULL) {
		// job data should contain a value;
		// try to use the default value anyway
		capability = getPrinterCap()->getDefaultCap(category);
	}

	if (capability == NULL) {
		// should not reach here!
		return;
	}

	fConfiguration.fDriverSpecificSettings[key] = capability->fKey;
}


bool
GPDriver::startPage(int)
{
	fBinding.BeginPage();
	return true;
}


bool
GPDriver::endPage(int)
{
	try {
		fBinding.EndPage();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
GPDriver::endDoc(bool)
{
	try {
		fBinding.EndJob();
		return true;
	}
	catch (TransportException& err) {
		return false;
	} 
}


bool
GPDriver::nextBand(BBitmap* bitmap, BPoint* offset)
{
	DBGMSG(("> nextBand\n"));
	try {
		BRect bounds = bitmap->Bounds();

		RECT rc;
		rc.left = (int)bounds.left;
		rc.top = (int)bounds.top;
		rc.right = (int)bounds.right;
		rc.bottom = (int)bounds.bottom;

		int height = rc.bottom - rc.top + 1;

		int x = (int)offset->x;
		int y = (int)offset->y;

		int page_height = getPageHeight();

		if (y + height > page_height)
			height = page_height - y;

		rc.bottom = height - 1;

		DBGMSG(("height = %d\n", height));
		DBGMSG(("x = %d\n", x));
		DBGMSG(("y = %d\n", y));

		if (get_valid_rect(bitmap, &rc)) {

			DBGMSG(("validate rect = %d, %d, %d, %d\n",
				rc.left, rc.top, rc.right, rc.bottom));

			x = rc.left;
			y += rc.top;
/*
			int width = rc.right - rc.left + 1;
			int height = rc.bottom - rc.top + 1;
			int delta = bitmap->BytesPerRow();

			DBGMSG(("width = %d\n", width));
			DBGMSG(("height = %d\n", height));
			DBGMSG(("delta = %d\n", delta));
			DBGMSG(("renderobj->get_pixel_depth() = %d\n", fHalftone->getPixelDepth()));
*/
			int width = rc.right - rc.left + 1;
			int height = rc.bottom - rc.top + 1;
			fprintf(stderr, "GPDriver nextBand x %d, y %d, width %d,"
				" height %d\n",
				x, y, width, height);
			BRect imageRect(rc.left, rc.top, rc.right, rc.bottom);
			fBinding.AddBitmapToPage(bitmap, imageRect, BPoint(x, y));
		} else {
			DBGMSG(("band bitmap is clean.\n"));
		}

		if (y >= page_height) {
			offset->x = -1.0;
			offset->y = -1.0;
		} else
			offset->y += height;

		DBGMSG(("< nextBand\n"));
		return true;
	}
	catch (TransportException& err) {
		BAlert* alert = new BAlert("", err.what(), "OK");
		alert->Go();
		return false;
	} 
}

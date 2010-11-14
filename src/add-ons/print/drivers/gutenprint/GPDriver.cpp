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
	const EnumCap* capability;
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
		const DriverSpecificCap* capability =
			dynamic_cast<const DriverSpecificCap*>(capabilities[i]);
		if (capability == NULL) {
			fprintf(stderr, "Internal error: DriverSpecificCap name='%s' "
				"has wrong type!\n", capabilities[i]->Label());
			continue;
		}

		PrinterCap::CapID id = static_cast<PrinterCap::CapID>(capability->ID());
		const char* key = capability->fKey.c_str();
		switch (capability->fType) {
			case DriverSpecificCap::kList:
				AddDriverSpecificSetting(id, key);
				break;
			case DriverSpecificCap::kBoolean:
				AddDriverSpecificBooleanSetting(id, key);
				break;
			case DriverSpecificCap::kIntRange:
				AddDriverSpecificIntSetting(id, key);
				break;
			case DriverSpecificCap::kIntDimension:
				AddDriverSpecificDimensionSetting(id, key);
				break;
			case DriverSpecificCap::kDoubleRange:
				AddDriverSpecificDoubleSetting(id, key);
				break;
		}
	}
}


void
GPDriver::AddDriverSpecificSetting(PrinterCap::CapID category, const char* key) {
	const EnumCap* capability = NULL;
	if (getJobData()->Settings().HasString(key))
	{
		const string& value = getJobData()->Settings().GetString(key);
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

	fConfiguration.fStringSettings[key] = capability->fKey;
}


void
GPDriver::AddDriverSpecificBooleanSetting(PrinterCap::CapID category,
	const char* key) {
	if (getJobData()->Settings().HasBoolean(key))
		fConfiguration.fBooleanSettings[key] =
			getJobData()->Settings().GetBoolean(key);
}


void
GPDriver::AddDriverSpecificIntSetting(PrinterCap::CapID category,
	const char* key) {
	if (getJobData()->Settings().HasInt(key))
		fConfiguration.fIntSettings[key] =
			getJobData()->Settings().GetInt(key);
}


void
GPDriver::AddDriverSpecificDimensionSetting(PrinterCap::CapID category,
	const char* key) {
	if (getJobData()->Settings().HasInt(key))
		fConfiguration.fDimensionSettings[key] =
			getJobData()->Settings().GetInt(key);
}


void
GPDriver::AddDriverSpecificDoubleSetting(PrinterCap::CapID category,
	const char* key) {
	if (getJobData()->Settings().HasDouble(key))
		fConfiguration.fDoubleSettings[key] =
			getJobData()->Settings().GetDouble(key);
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

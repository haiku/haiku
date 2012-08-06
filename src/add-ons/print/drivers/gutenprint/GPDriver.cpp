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


GPDriver::GPDriver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	GraphicsDriver(message, printerData, printerCap)
{
}

void
GPDriver::Write(const void* buffer, size_t size)
	throw (TransportException)
{
	WriteSpoolData(buffer, size);
}

bool
GPDriver::StartDocument()
{
	try {
		const GPData* data = dynamic_cast<const GPData*>(GetPrinterData());
		ASSERT(data != NULL);
		fConfiguration.fDriver = data->fGutenprintDriverName;

		SetParameter(fConfiguration.fPageSize, PrinterCap::kPaper,
			GetJobData()->GetPaper());

		SetParameter(fConfiguration.fResolution, PrinterCap::kResolution,
			GetJobData()->GetResolutionID());

		fConfiguration.fXDPI = GetJobData()->GetXres();
		fConfiguration.fYDPI = GetJobData()->GetYres();

		SetParameter(fConfiguration.fInputSlot, PrinterCap::kPaperSource,
			GetJobData()->GetPaperSource());

		SetParameter(fConfiguration.fPrintingMode, PrinterCap::kColor,
			GetJobData()->GetColor());

		if (GetPrinterCap()->Supports(PrinterCap::kDriverSpecificCapabilities))
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
	capability = GetPrinterCap()->FindCap(category, value);
	if (capability != NULL && capability->fKey != "")
		parameter = capability->Key();
}


void
GPDriver::SetDriverSpecificSettings()
{
	PrinterCap::CapID category = PrinterCap::kDriverSpecificCapabilities;
	int count = GetPrinterCap()->CountCap(category);
	const BaseCap** capabilities = GetPrinterCap()->GetCaps(category);
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
	if (GetJobData()->Settings().HasString(key))
	{
		const string& value = GetJobData()->Settings().GetString(key);
		capability = GetPrinterCap()->FindCapWithKey(category, value.c_str());
	}

	if (capability == NULL) {
		// job data should contain a value;
		// try to use the default value anyway
		capability = GetPrinterCap()->GetDefaultCap(category);
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
	if (GetJobData()->Settings().HasBoolean(key))
		fConfiguration.fBooleanSettings[key] =
			GetJobData()->Settings().GetBoolean(key);
}


void
GPDriver::AddDriverSpecificIntSetting(PrinterCap::CapID category,
	const char* key) {
	if (GetJobData()->Settings().HasInt(key))
		fConfiguration.fIntSettings[key] =
			GetJobData()->Settings().GetInt(key);
}


void
GPDriver::AddDriverSpecificDimensionSetting(PrinterCap::CapID category,
	const char* key) {
	if (GetJobData()->Settings().HasInt(key))
		fConfiguration.fDimensionSettings[key] =
			GetJobData()->Settings().GetInt(key);
}


void
GPDriver::AddDriverSpecificDoubleSetting(PrinterCap::CapID category,
	const char* key) {
	if (GetJobData()->Settings().HasDouble(key))
		fConfiguration.fDoubleSettings[key] =
			GetJobData()->Settings().GetDouble(key);
}


bool
GPDriver::StartPage(int)
{
	fBinding.BeginPage();
	return true;
}


bool
GPDriver::EndPage(int)
{
	try {
		fBinding.EndPage();
		return true;
	}
	catch (TransportException& err) {
		ShowError(err.What());
		return false;
	} 
}


bool
GPDriver::EndDocument(bool)
{
	try {
		fBinding.EndJob();
		return true;
	}
	catch (TransportException& err) {
		ShowError(err.What());
		return false;
	} 
}


bool
GPDriver::NextBand(BBitmap* bitmap, BPoint* offset)
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

		int pageHeight = GetPageHeight();

		if (y + height > pageHeight)
			height = pageHeight - y;

		rc.bottom = height - 1;

		DBGMSG(("height = %d\n", height));
		DBGMSG(("x = %d\n", x));
		DBGMSG(("y = %d\n", y));

		if (get_valid_rect(bitmap, &rc)) {

			DBGMSG(("validate rect = %d, %d, %d, %d\n",
				rc.left, rc.top, rc.right, rc.bottom));

			x = rc.left;
			y += rc.top;

			int width = rc.right - rc.left + 1;
			int height = rc.bottom - rc.top + 1;
			fprintf(stderr, "GPDriver nextBand x %d, y %d, width %d,"
				" height %d\n",
				x, y, width, height);
			BRect imageRect(rc.left, rc.top, rc.right, rc.bottom);
			status_t status;
			status = fBinding.AddBitmapToPage(bitmap, imageRect, BPoint(x, y));
			if (status == B_NO_MEMORY) {
				ShowError("Out of memory");
				return false;
			} else if (status != B_OK) {
				ShowError("Unknown error");
				return false;
			}

		} else {
			DBGMSG(("band bitmap is empty.\n"));
		}

		if (y >= pageHeight) {
			offset->x = -1.0;
			offset->y = -1.0;
		} else
			offset->y += height;

		DBGMSG(("< nextBand\n"));
		return true;
	}
	catch (TransportException& err) {
		ShowError(err.What());
		return false;
	} 
}


void
GPDriver::ShowError(const char* message)
{
	BString text;
	text << "An error occurred attempting to print with Gutenprint:";
	text << "\n";
	text << message;
	BAlert* alert = new BAlert("", text.String(), "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}

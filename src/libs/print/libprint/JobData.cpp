/*
 * JobData.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "JobData.h"

#include <Debug.h>
#include <InterfaceDefs.h>
#include <Message.h>

#include <sstream>

#include "PrinterCap.h"
#include "DbgMsg.h"

static const char *kJDXRes                  = "xres";
static const char *kJDYRes                  = "yres";
static const char *kJDCopies                = "copies";
static const char *kJDOrientation           = "orientation";
static const char *kJDScaling               = "scale";
static const char *kJDScaledPaperRect       = "paper_rect";
static const char *kJDScaledPrintableRect   = "printable_rect";
static const char *kJDFirstPage             = "first_page";
static const char *kJDLastPage              = "last_page";

static const char *kJDShowPreview           = "JJJJ_showPreview";
static const char *kJDPaper                 = "JJJJ_paper";
static const char *kJDNup                   = "JJJJ_nup";
static const char *kJDGamma                 = "JJJJ_gamma";
static const char *kJDInkDensity            = "JJJJ_ink_density";
static const char *kJDPaperSource           = "JJJJ_paper_source";
static const char *kJDCollate               = "JJJJ_collate";
static const char *kJDReverse               = "JJJJ_reverse";
static const char *kJDPrintStyle            = "JJJJ_print_style";
static const char *kJDBindingLocation       = "JJJJ_binding_location";
static const char *kJDPageOrder             = "JJJJ_page_order";
static const char *kJDColor                 = "JJJJ_color";
static const char *kJDDitherType            = "JJJJ_dither_type";
static const char *kJDPaperRect             = "JJJJ_paper_rect";
static const char* kJDPrintableRect         = "JJJJ_printable_rect";
static const char* kJDPageSelection         = "JJJJ_page_selection";
static const char* kJDMarginUnit            = "JJJJ_margin_unit";
static const char* kJDPhysicalRect          = "JJJJ_physical_rect";
static const char* kJDScaledPhysicalRect    = "JJJJ_scaled_physical_rect";
static const char* kJDResolution            = "JJJJ_resolution";
static const char* kJDDriverSpecificSettings = "JJJJ_driverSpecificSettings";


DriverSpecificSettings::DriverSpecificSettings()
{
}


DriverSpecificSettings::DriverSpecificSettings(
	const DriverSpecificSettings& settings)
	:
	fSettings(settings.fSettings)
{
}


DriverSpecificSettings &
DriverSpecificSettings::operator=(const DriverSpecificSettings &settings)
{
	fSettings = settings.fSettings;
	return *this;
}


void
DriverSpecificSettings::MakeEmpty()
{
	fSettings.MakeEmpty();
}


bool
DriverSpecificSettings::HasString(const char* key) const
{
	const char* value;
	return fSettings.FindString(key, &value) == B_OK;
}


const char*
DriverSpecificSettings::GetString(const char* key) const
{
	ASSERT(HasString(key));
	const char* value = NULL;
	fSettings.FindString(key, &value);
	return value;
}


void
DriverSpecificSettings::SetString(const char* key, const char* value)
{
	if (HasString(key))
		fSettings.ReplaceString(key, value);
	else
		fSettings.AddString(key, value);
}


bool
DriverSpecificSettings::HasBoolean(const char* key) const
{
	bool value;
	return fSettings.FindBool(key, &value) == B_OK;
}


bool
DriverSpecificSettings::GetBoolean(const char* key) const
{
	ASSERT(HasBoolean(key));
	bool value;
	fSettings.FindBool(key, &value);
	return value;
}


void
DriverSpecificSettings::SetBoolean(const char* key, bool value)
{
	if (HasBoolean(key))
		fSettings.ReplaceBool(key, value);
	else
		fSettings.AddBool(key, value);
}


bool
DriverSpecificSettings::HasInt(const char* key) const
{
	int32 value;
	return fSettings.FindInt32(key, &value) == B_OK;
}


int32
DriverSpecificSettings::GetInt(const char* key) const
{
	ASSERT(HasInt(key));
	int32 value;
	fSettings.FindInt32(key, &value);
	return value;
}


void
DriverSpecificSettings::SetInt(const char* key, int32 value)
{
	if (HasInt(key))
		fSettings.ReplaceInt32(key, value);
	else
		fSettings.AddInt32(key, value);
}


bool
DriverSpecificSettings::HasDouble(const char* key) const
{
	double value;
	return fSettings.FindDouble(key, &value) == B_OK;
}


double
DriverSpecificSettings::GetDouble(const char* key) const
{
	ASSERT(HasDouble(key));
	double value;
	fSettings.FindDouble(key, &value);
	return value;
}


void
DriverSpecificSettings::SetDouble(const char* key, double value)
{
	if (HasDouble(key))
		fSettings.ReplaceDouble(key, value);
	else
		fSettings.AddDouble(key, value);
}


BMessage&
DriverSpecificSettings::Message()
{
	return fSettings;
}


JobData::JobData(BMessage *message, const PrinterCap *printerCap, SettingType settingType)
{
	Load(message, printerCap, settingType);
}


JobData::~JobData()
{
}


JobData::JobData(const JobData &jobData)
{
	*this = jobData;
}


JobData&
JobData::operator=(const JobData &jobData)
{
	fShowPreview           = jobData.fShowPreview;
	fPaper                 = jobData.fPaper;
	fResolutionID          = jobData.fResolutionID;
	fXRes                  = jobData.fXRes;
	fYRes                  = jobData.fYRes;
	fOrientation           = jobData.fOrientation;
	fScaling               = jobData.fScaling;
	fPaperRect             = jobData.fPaperRect;
	fScaledPaperRect       = jobData.fScaledPaperRect;
	fPrintableRect         = jobData.fPrintableRect;
	fScaledPrintableRect   = jobData.fScaledPrintableRect;
	fNup                   = jobData.fNup;
	fFirstPage             = jobData.fFirstPage;
	fLastPage              = jobData.fLastPage;
	fGamma                 = jobData.fGamma;
	fInkDensity            = jobData.fInkDensity;
	fPaperSource           = jobData.fPaperSource;
	fCopies                = jobData.fCopies;
	fCollate               = jobData.fCollate;
	fReverse               = jobData.fReverse;
	fPrintStyle            = jobData.fPrintStyle;
	fBindingLocation       = jobData.fBindingLocation;
	fPageOrder             = jobData.fPageOrder;
	fSettingType           = jobData.fSettingType;
	fMsg                   = jobData.fMsg;
	fColor                 = jobData.fColor;
	fDitherType            = jobData.fDitherType;
	fPageSelection         = jobData.fPageSelection;
	fMarginUnit            = jobData.fMarginUnit;
	fPhysicalRect          = jobData.fPhysicalRect;
	fScaledPhysicalRect    = jobData.fScaledPhysicalRect;
	fDriverSpecificSettings = jobData.fDriverSpecificSettings;
	return *this;
}


void
JobData::Load(BMessage* message, const PrinterCap* printerCap,
	SettingType settingType)
{
	fMsg = message;
	fSettingType = settingType;

	const PaperCap *paperCap = NULL;
 
 	if (message->HasBool(kJDShowPreview))
 		fShowPreview = message->FindBool(kJDShowPreview);
 	else
 		fShowPreview = false;
 
	if (message->HasInt32(kJDPaper))
		fPaper = (Paper)message->FindInt32(kJDPaper);
	else if (printerCap->Supports(PrinterCap::kPaper)) {
		paperCap = (const PaperCap *)printerCap->GetDefaultCap(
			PrinterCap::kPaper);
		fPaper = paperCap->fPaper;
	} else
		fPaper = kA4;

	if (message->HasInt32(kJDResolution)) {
		message->FindInt32(kJDResolution, &fResolutionID);
	} else if (printerCap->Supports(PrinterCap::kResolution)) {
		fResolutionID = printerCap->GetDefaultCap(PrinterCap::kResolution)
			->ID();
	} else {
		// should not reach here!
		fResolutionID = 0;
	}

	if (message->HasInt64(kJDXRes)) {
		int64 xres64; 
		message->FindInt64(kJDXRes, &xres64);
		fXRes = xres64; 
	} else if (printerCap->Supports(PrinterCap::kResolution)) {
		fXRes = ((const ResolutionCap *)printerCap->GetDefaultCap(
			PrinterCap::kResolution))->fXResolution;
	} else {
		fXRes = 300; 
	}

	if (message->HasInt64(kJDYRes)) {
		int64 yres64;
		message->FindInt64(kJDYRes, &yres64);
		fYRes = yres64;
	} else if (printerCap->Supports(PrinterCap::kResolution)) {
		fYRes = ((const ResolutionCap *)printerCap->GetDefaultCap(
			PrinterCap::kResolution))->fYResolution;
	} else {
		fYRes = 300;
	}

	if (message->HasInt32(kJDOrientation))
		fOrientation = (Orientation)message->FindInt32(kJDOrientation);
	else if (printerCap->Supports(PrinterCap::kOrientation))
		fOrientation = ((const OrientationCap *)printerCap->GetDefaultCap(
			PrinterCap::kOrientation))->fOrientation;
	else
		fOrientation = kPortrait;

	if (message->HasFloat(kJDScaling))
		fScaling = message->FindFloat(kJDScaling);
	else
		fScaling = 100.0f;

	if (message->HasRect(kJDPaperRect)) {
		fPaperRect = message->FindRect(kJDPaperRect);
	} else if (paperCap != NULL) {
		fPaperRect = paperCap->fPaperRect;
	}

	if (message->HasRect(kJDScaledPaperRect)) {
		fScaledPaperRect = message->FindRect(kJDScaledPaperRect);
	} else {
		fScaledPaperRect = fPaperRect;
	}

	if (message->HasRect(kJDPrintableRect)) {
		fPrintableRect = message->FindRect(kJDPrintableRect);
	} else if (paperCap != NULL) {
		fPrintableRect = paperCap->fPhysicalRect;
	}

	if (message->HasRect(kJDScaledPrintableRect)) {
		fScaledPrintableRect = message->FindRect(kJDScaledPrintableRect);
	} else {
		fScaledPrintableRect = fPrintableRect;
	}

	if (message->HasRect(kJDPhysicalRect)) {
		fPhysicalRect = message->FindRect(kJDPhysicalRect);
	} else if (paperCap != NULL) {
		fPhysicalRect = paperCap->fPhysicalRect;
	}

	if (message->HasRect(kJDScaledPhysicalRect)) {
		fScaledPhysicalRect = message->FindRect(kJDScaledPhysicalRect);
	} else {
		fScaledPhysicalRect = fPhysicalRect;
	}

	if (message->HasInt32(kJDFirstPage))
		fFirstPage = message->FindInt32(kJDFirstPage);
	else
		fFirstPage = 1;

	if (message->HasInt32(kJDLastPage))
		fLastPage = message->FindInt32(kJDLastPage);
	else
		fLastPage = -1;

	if (message->HasInt32(kJDNup))
		fNup = message->FindInt32(kJDNup);
	else
		fNup = 1;

	if (message->HasFloat(kJDGamma))
		fGamma = fMsg->FindFloat(kJDGamma);
	else
		fGamma = 0.25f;

	if (message->HasFloat(kJDInkDensity))
		fInkDensity = fMsg->FindFloat(kJDInkDensity);
	else
		fInkDensity = 0.0f;

	if (message->HasInt32(kJDPaperSource))
		fPaperSource = (PaperSource)fMsg->FindInt32(kJDPaperSource);
	else if (printerCap->Supports(PrinterCap::kPaperSource))
		fPaperSource = ((const PaperSourceCap *)printerCap->GetDefaultCap(
			PrinterCap::kPaperSource))->fPaperSource;
	else
		fPaperSource = kAuto;

	if (message->HasInt32(kJDCopies))
		fCopies = message->FindInt32(kJDCopies);
	else
		fCopies = 1;

	if (message->HasBool(kJDCollate))
		fCollate = message->FindBool(kJDCollate);
	else
		fCollate = false;

	if (message->HasBool(kJDReverse))
		fReverse = message->FindBool(kJDReverse);
	else
		fReverse = false;

	if (message->HasInt32(kJDPrintStyle))
		fPrintStyle = (PrintStyle)message->FindInt32(kJDPrintStyle);
	else if (printerCap->Supports(PrinterCap::kPrintStyle))
		fPrintStyle = ((const PrintStyleCap *)printerCap->GetDefaultCap(
			PrinterCap::kPrintStyle))->fPrintStyle;
	else
		fPrintStyle = kSimplex;

	if (message->HasInt32(kJDBindingLocation))
		fBindingLocation = (BindingLocation)message->FindInt32(
			kJDBindingLocation);
	else if (printerCap->Supports(PrinterCap::kBindingLocation))
		fBindingLocation = ((const BindingLocationCap *)printerCap->
			GetDefaultCap(PrinterCap::kBindingLocation))->fBindingLocation;
	else
		fBindingLocation = kLongEdgeLeft;

	if (message->HasInt32(kJDPageOrder))
		fPageOrder = (PageOrder)message->FindInt32(kJDPageOrder);
	else
		fPageOrder = kAcrossFromLeft;

	if (message->HasInt32(kJDColor))
		fColor = (Color)message->FindInt32(kJDColor);
	else if (printerCap->Supports(PrinterCap::kColor))
		fColor = ((const ColorCap *)printerCap->GetDefaultCap(
			PrinterCap::kColor))->fColor;
	else
		fColor = kMonochrome;
	
	if (message->HasInt32(kJDDitherType))
		fDitherType = (Halftone::DitherType)message->FindInt32(kJDDitherType);
	else
		fDitherType = Halftone::kTypeFloydSteinberg;
	
	if (message->HasInt32(kJDPageSelection))
		fPageSelection = (PageSelection)message->FindInt32(kJDPageSelection);
	else
		fPageSelection = kAllPages;

	if (message->HasInt32(kJDMarginUnit))
		fMarginUnit = (MarginUnit)message->FindInt32(kJDMarginUnit);
	else
		fMarginUnit = kUnitInch;

	if (message->HasMessage(kJDDriverSpecificSettings))
		message->FindMessage(kJDDriverSpecificSettings,
			&fDriverSpecificSettings.Message());
}


void
JobData::Save(BMessage* message)
{
	if (message == NULL) {
		message = fMsg;
	}

	// page settings
	message->RemoveName(kJDPaper);
	message->AddInt32(kJDPaper, fPaper);

	message->RemoveName(kJDResolution);
	message->AddInt32(kJDResolution, fResolutionID);

	message->RemoveName(kJDXRes);
	message->AddInt64(kJDXRes, fXRes);
	
	message->RemoveName(kJDYRes);
	message->AddInt64(kJDYRes, fYRes);

	message->RemoveName(kJDOrientation);
	message->AddInt32(kJDOrientation, fOrientation);

	message->RemoveName(kJDScaling);
	message->AddFloat(kJDScaling, fScaling);

	message->RemoveName(kJDPaperRect);
	message->AddRect(kJDPaperRect, fPaperRect);

	message->RemoveName(kJDScaledPaperRect);
	message->AddRect(kJDScaledPaperRect, fScaledPaperRect);

	message->RemoveName(kJDPrintableRect);
	message->AddRect(kJDPrintableRect, fPrintableRect);

	message->RemoveName(kJDScaledPrintableRect);
	message->AddRect(kJDScaledPrintableRect, fScaledPrintableRect);

	message->RemoveName(kJDPhysicalRect);
	message->AddRect(kJDPhysicalRect, fPhysicalRect);

	message->RemoveName(kJDScaledPhysicalRect);
	message->AddRect(kJDScaledPhysicalRect, fScaledPhysicalRect);

	message->RemoveName(kJDMarginUnit);
	message->AddInt32(kJDMarginUnit, fMarginUnit);

	// page settings end here
	
	// job settings

	// make sure job settings are not present in page settings
	message->RemoveName(kJDShowPreview);
	if (fSettingType == kJobSettings)
		message->AddBool(kJDShowPreview, fShowPreview);
	
	message->RemoveName(kJDNup);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDNup, fNup);

	message->RemoveName(kJDFirstPage);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDFirstPage, fFirstPage);

	message->RemoveName(kJDLastPage);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDLastPage, fLastPage);

	message->RemoveName(kJDGamma);
	if (fSettingType == kJobSettings)
		message->AddFloat(kJDGamma, fGamma);

	message->RemoveName(kJDInkDensity);
	if (fSettingType == kJobSettings)
		message->AddFloat(kJDInkDensity, fInkDensity);

	message->RemoveName(kJDPaperSource);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDPaperSource, fPaperSource);

	message->RemoveName(kJDCopies);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDCopies, fCopies);

	message->RemoveName(kJDCollate);
	if (fSettingType == kJobSettings)
		message->AddBool(kJDCollate, fCollate);

	message->RemoveName(kJDReverse);
	if (fSettingType == kJobSettings)
		message->AddBool(kJDReverse, fReverse);

	message->RemoveName(kJDPrintStyle);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDPrintStyle, fPrintStyle);

	message->RemoveName(kJDBindingLocation);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDBindingLocation, fBindingLocation);

	message->RemoveName(kJDPageOrder);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDPageOrder, fPageOrder);

	message->RemoveName(kJDColor);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDColor, fColor);

	message->RemoveName(kJDDitherType);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDDitherType, fDitherType);
	
	message->RemoveName(kJDPageSelection);
	if (fSettingType == kJobSettings)
		message->AddInt32(kJDPageSelection, fPageSelection);

	message->RemoveName(kJDDriverSpecificSettings);
	if (fSettingType == kJobSettings)
	{
		message->AddMessage(kJDDriverSpecificSettings,
			&fDriverSpecificSettings.Message());
	}
}


DriverSpecificSettings&
JobData::Settings()
{
	return fDriverSpecificSettings;
}


const DriverSpecificSettings&
JobData::Settings() const
{
	return fDriverSpecificSettings;
}

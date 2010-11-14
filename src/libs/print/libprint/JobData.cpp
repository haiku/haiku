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


JobData::JobData(BMessage *msg, const PrinterCap *cap, SettingType type)
{
	load(msg, cap, type);
}


JobData::~JobData()
{
}


JobData::JobData(const JobData &job_data)
{
	*this = job_data;
}


JobData&
JobData::operator=(const JobData &job_data)
{
	fShowPreview           = job_data.fShowPreview;
	fPaper                 = job_data.fPaper;
	fResolutionID          = job_data.fResolutionID;
	fXRes                  = job_data.fXRes;
	fYRes                  = job_data.fYRes;
	fOrientation           = job_data.fOrientation;
	fScaling               = job_data.fScaling;
	fPaperRect             = job_data.fPaperRect;
	fScaledPaperRect       = job_data.fScaledPaperRect;
	fPrintableRect         = job_data.fPrintableRect;
	fScaledPrintableRect   = job_data.fScaledPrintableRect;
	fNup                   = job_data.fNup;
	fFirstPage             = job_data.fFirstPage;
	fLastPage              = job_data.fLastPage;
	fGamma                 = job_data.fGamma;
	fInkDensity            = job_data.fInkDensity;
	fPaperSource           = job_data.fPaperSource;
	fCopies                = job_data.fCopies;
	fCollate               = job_data.fCollate;
	fReverse               = job_data.fReverse;
	fPrintStyle            = job_data.fPrintStyle;
	fBindingLocation       = job_data.fBindingLocation;
	fPageOrder             = job_data.fPageOrder;
	fSettingType           = job_data.fSettingType;
	fMsg                   = job_data.fMsg;
	fColor                 = job_data.fColor;
	fDitherType            = job_data.fDitherType;
	fPageSelection         = job_data.fPageSelection;
	fMarginUnit            = job_data.fMarginUnit;
	fPhysicalRect          = job_data.fPhysicalRect;
	fScaledPhysicalRect    = job_data.fScaledPhysicalRect;
	fDriverSpecificSettings = job_data.fDriverSpecificSettings;
	return *this;
}


void
JobData::load(BMessage *msg, const PrinterCap *cap, SettingType type)
{
	fMsg = msg;
	fSettingType = type;

	const PaperCap *paperCap = NULL;
 
 	if (msg->HasBool(kJDShowPreview))
 		fShowPreview = msg->FindBool(kJDShowPreview);
 	else
 		fShowPreview = false;
 
	if (msg->HasInt32(kJDPaper))
		fPaper = (Paper)msg->FindInt32(kJDPaper);
	else if (cap->isSupport(PrinterCap::kPaper)) {
		paperCap = (const PaperCap *)cap->getDefaultCap(PrinterCap::kPaper);
		fPaper = paperCap->fPaper;
	} else
		fPaper = kA4;

	if (msg->HasInt32(kJDResolution)) {
		msg->FindInt32(kJDResolution, &fResolutionID);
	} else if (cap->isSupport(PrinterCap::kResolution)) {
		fResolutionID = cap->getDefaultCap(PrinterCap::kResolution)->ID();
	} else {
		// should not reach here!
		fResolutionID = 0;
	}

	if (msg->HasInt64(kJDXRes)) {
		int64 xres64; 
		msg->FindInt64(kJDXRes, &xres64);
		fXRes = xres64; 
	} else if (cap->isSupport(PrinterCap::kResolution)) {
		fXRes = ((const ResolutionCap *)cap->getDefaultCap(
			PrinterCap::kResolution))->fXResolution;
	} else {
		fXRes = 300; 
	}

	if (msg->HasInt64(kJDYRes)) {
		int64 yres64;
		msg->FindInt64(kJDYRes, &yres64);
		fYRes = yres64;
	} else if (cap->isSupport(PrinterCap::kResolution)) {
		fYRes = ((const ResolutionCap *)cap->getDefaultCap(
			PrinterCap::kResolution))->fYResolution;
	} else {
		fYRes = 300;
	}

	if (msg->HasInt32(kJDOrientation))
		fOrientation = (Orientation)msg->FindInt32(kJDOrientation);
	else if (cap->isSupport(PrinterCap::kOrientation))
		fOrientation = ((const OrientationCap *)cap->getDefaultCap(
			PrinterCap::kOrientation))->fOrientation;
	else
		fOrientation = kPortrait;

	if (msg->HasFloat(kJDScaling))
		fScaling = msg->FindFloat(kJDScaling);
	else
		fScaling = 100.0f;

	if (msg->HasRect(kJDPaperRect)) {
		fPaperRect = msg->FindRect(kJDPaperRect);
	} else if (paperCap != NULL) {
		fPaperRect = paperCap->fPaperRect;
	}

	if (msg->HasRect(kJDScaledPaperRect)) {
		fScaledPaperRect = msg->FindRect(kJDScaledPaperRect);
	} else {
		fScaledPaperRect = fPaperRect;
	}

	if (msg->HasRect(kJDPrintableRect)) {
		fPrintableRect = msg->FindRect(kJDPrintableRect);
	} else if (paperCap != NULL) {
		fPrintableRect = paperCap->fPhysicalRect;
	}

	if (msg->HasRect(kJDScaledPrintableRect)) {
		fScaledPrintableRect = msg->FindRect(kJDScaledPrintableRect);
	} else {
		fScaledPrintableRect = fPrintableRect;
	}

	if (msg->HasRect(kJDPhysicalRect)) {
		fPhysicalRect = msg->FindRect(kJDPhysicalRect);
	} else if (paperCap != NULL) {
		fPhysicalRect = paperCap->fPhysicalRect;
	}

	if (msg->HasRect(kJDScaledPhysicalRect)) {
		fScaledPhysicalRect = msg->FindRect(kJDScaledPhysicalRect);
	} else {
		fScaledPhysicalRect = fPhysicalRect;
	}

	if (msg->HasInt32(kJDFirstPage))
		fFirstPage = msg->FindInt32(kJDFirstPage);
	else
		fFirstPage = 1;

	if (msg->HasInt32(kJDLastPage))
		fLastPage = msg->FindInt32(kJDLastPage);
	else
		fLastPage = -1;

	if (msg->HasInt32(kJDNup))
		fNup = msg->FindInt32(kJDNup);
	else
		fNup = 1;

	if (msg->HasFloat(kJDGamma))
		fGamma = fMsg->FindFloat(kJDGamma);
	else
		fGamma = 0.25f;

	if (msg->HasFloat(kJDInkDensity))
		fInkDensity = fMsg->FindFloat(kJDInkDensity);
	else
		fInkDensity = 0.0f;

	if (msg->HasInt32(kJDPaperSource))
		fPaperSource = (PaperSource)fMsg->FindInt32(kJDPaperSource);
	else if (cap->isSupport(PrinterCap::kPaperSource))
		fPaperSource = ((const PaperSourceCap *)cap->getDefaultCap(
			PrinterCap::kPaperSource))->fPaperSource;
	else
		fPaperSource = kAuto;

	if (msg->HasInt32(kJDCopies))
		fCopies = msg->FindInt32(kJDCopies);
	else
		fCopies = 1;

	if (msg->HasBool(kJDCollate))
		fCollate = msg->FindBool(kJDCollate);
	else
		fCollate = false;

	if (msg->HasBool(kJDReverse))
		fReverse = msg->FindBool(kJDReverse);
	else
		fReverse = false;

	if (msg->HasInt32(kJDPrintStyle))
		fPrintStyle = (PrintStyle)msg->FindInt32(kJDPrintStyle);
	else if (cap->isSupport(PrinterCap::kPrintStyle))
		fPrintStyle = ((const PrintStyleCap *)cap->getDefaultCap(
			PrinterCap::kPrintStyle))->fPrintStyle;
	else
		fPrintStyle = kSimplex;

	if (msg->HasInt32(kJDBindingLocation))
		fBindingLocation = (BindingLocation)msg->FindInt32(kJDBindingLocation);
	else if (cap->isSupport(PrinterCap::kBindingLocation))
		fBindingLocation = ((const BindingLocationCap *)cap->getDefaultCap(
			PrinterCap::kBindingLocation))->fBindingLocation;
	else
		fBindingLocation = kLongEdgeLeft;

	if (msg->HasInt32(kJDPageOrder))
		fPageOrder = (PageOrder)msg->FindInt32(kJDPageOrder);
	else
		fPageOrder = kAcrossFromLeft;

	if (msg->HasInt32(kJDColor))
		fColor = (Color)msg->FindInt32(kJDColor);
	else if (cap->isSupport(PrinterCap::kColor))
		fColor = ((const ColorCap *)cap->getDefaultCap(PrinterCap::kColor))
			->fColor;
	else
		fColor = kMonochrome;
	
	if (msg->HasInt32(kJDDitherType))
		fDitherType = (Halftone::DitherType)msg->FindInt32(kJDDitherType);
	else
		fDitherType = Halftone::kTypeFloydSteinberg;
	
	if (msg->HasInt32(kJDPageSelection))
		fPageSelection = (PageSelection)msg->FindInt32(kJDPageSelection);
	else
		fPageSelection = kAllPages;

	if (msg->HasInt32(kJDMarginUnit))
		fMarginUnit = (MarginUnit)msg->FindInt32(kJDMarginUnit);
	else
		fMarginUnit = kUnitInch;

	if (msg->HasMessage(kJDDriverSpecificSettings))
		msg->FindMessage(kJDDriverSpecificSettings,
			&fDriverSpecificSettings.Message());
}


void
JobData::save(BMessage *msg)
{
	if (msg == NULL) {
		msg = fMsg;
	}

	// page settings
	msg->RemoveName(kJDPaper);
	msg->AddInt32(kJDPaper, fPaper);

	msg->RemoveName(kJDResolution);
	msg->AddInt32(kJDResolution, fResolutionID);

	msg->RemoveName(kJDXRes);
	msg->AddInt64(kJDXRes, fXRes);
	
	msg->RemoveName(kJDYRes);
	msg->AddInt64(kJDYRes, fYRes);

	msg->RemoveName(kJDOrientation);
	msg->AddInt32(kJDOrientation, fOrientation);

	msg->RemoveName(kJDScaling);
	msg->AddFloat(kJDScaling, fScaling);

	msg->RemoveName(kJDPaperRect);
	msg->AddRect(kJDPaperRect, fPaperRect);

	msg->RemoveName(kJDScaledPaperRect);
	msg->AddRect(kJDScaledPaperRect, fScaledPaperRect);

	msg->RemoveName(kJDPrintableRect);
	msg->AddRect(kJDPrintableRect, fPrintableRect);

	msg->RemoveName(kJDScaledPrintableRect);
	msg->AddRect(kJDScaledPrintableRect, fScaledPrintableRect);

	msg->RemoveName(kJDPhysicalRect);
	msg->AddRect(kJDPhysicalRect, fPhysicalRect);

	msg->RemoveName(kJDScaledPhysicalRect);
	msg->AddRect(kJDScaledPhysicalRect, fScaledPhysicalRect);

	msg->RemoveName(kJDMarginUnit);
	msg->AddInt32(kJDMarginUnit, fMarginUnit);

	// page settings end here
	
	// job settings

	// make sure job settings are not present in page settings
	msg->RemoveName(kJDShowPreview);
	if (fSettingType == kJobSettings)
		msg->AddBool(kJDShowPreview, fShowPreview);
	
	msg->RemoveName(kJDNup);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDNup, fNup);

	msg->RemoveName(kJDFirstPage);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDFirstPage, fFirstPage);

	msg->RemoveName(kJDLastPage);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDLastPage, fLastPage);

	msg->RemoveName(kJDGamma);
	if (fSettingType == kJobSettings)
		msg->AddFloat(kJDGamma, fGamma);

	msg->RemoveName(kJDInkDensity);
	if (fSettingType == kJobSettings)
		msg->AddFloat(kJDInkDensity, fInkDensity);

	msg->RemoveName(kJDPaperSource);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDPaperSource, fPaperSource);

	msg->RemoveName(kJDCopies);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDCopies, fCopies);

	msg->RemoveName(kJDCollate);
	if (fSettingType == kJobSettings)
		msg->AddBool(kJDCollate, fCollate);

	msg->RemoveName(kJDReverse);
	if (fSettingType == kJobSettings)
		msg->AddBool(kJDReverse, fReverse);

	msg->RemoveName(kJDPrintStyle);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDPrintStyle, fPrintStyle);

	msg->RemoveName(kJDBindingLocation);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDBindingLocation, fBindingLocation);

	msg->RemoveName(kJDPageOrder);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDPageOrder, fPageOrder);

	msg->RemoveName(kJDColor);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDColor, fColor);

	msg->RemoveName(kJDDitherType);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDDitherType, fDitherType);
	
	msg->RemoveName(kJDPageSelection);
	if (fSettingType == kJobSettings)
		msg->AddInt32(kJDPageSelection, fPageSelection);

	msg->RemoveName(kJDDriverSpecificSettings);
	if (fSettingType == kJobSettings)
	{
		msg->AddMessage(kJDDriverSpecificSettings,
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

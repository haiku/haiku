/*
 * JobData.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <InterfaceDefs.h>
#include <Message.h>
#include "JobData.h"
#include "PrinterCap.h"
#include "DbgMsg.h"

const char *kJDXRes             = "xres";
const char *kJDYRes             = "yres";
const char *kJDCopies           = "copies";
const char *kJDOrientation      = "orientation";
const char *kJDScaling          = "scaling";
const char *kJDPaperRect        = "paper_rect";
const char *kJDPrintableRect    = "printable_rect";
const char *kJDFirstPage        = "first_page";
const char *kJDLastPage         = "last_page";

const char *kJDPaper            = "JJJJ_paper";
const char *kJDNup              = "JJJJ_nup";
const char *kJDGamma            = "JJJJ_gamma";
const char *kJDInkDensity       = "JJJJ_ink_density";
const char *kJDPaperSource      = "JJJJ_paper_source";
const char *kJDCollate          = "JJJJ_collate";
const char *kJDReverse          = "JJJJ_reverse";
const char *kJDPrintStyle       = "JJJJ_print_style";
const char *kJDBindingLocation  = "JJJJ_binding_location";
const char *kJDPageOrder        = "JJJJ_page_order";
const char *kJDColor            = "JJJJ_color";
const char *kJDDitherType       = "JJJJ_dither_type";

JobData::JobData(BMessage *msg, const PrinterCap *cap, Settings settings)
{
	load(msg, cap, settings);
}

JobData::~JobData()
{
}

JobData::JobData(const JobData &job_data)
{
	fPaper            = job_data.fPaper;
	fXRes             = job_data.fXRes;
	fYRes             = job_data.fYRes;
	fOrientation      = job_data.fOrientation;
	fScaling          = job_data.fScaling;
	fPaperRect        = job_data.fPaperRect;
	fPrintableRect    = job_data.fPrintableRect;
	fNup              = job_data.fNup;
	fFirstPage        = job_data.fFirstPage;
	fLastPage         = job_data.fLastPage;
	fGamma            = job_data.fGamma;
	fInkDensity       = job_data.fInkDensity;
	fPaperSource      = job_data.fPaperSource;
	fCopies           = job_data.fCopies;
	fCollate          = job_data.fCollate;
	fReverse          = job_data.fReverse;
	fPrintStyle       = job_data.fPrintStyle;
	fBindingLocation  = job_data.fBindingLocation;
	fPageOrder        = job_data.fPageOrder;
	fSettings         = job_data.fSettings;
	fMsg              = job_data.fMsg;
	fColor            = job_data.fColor;
	fDitherType       = job_data.fDitherType;
}

JobData &JobData::operator = (const JobData &job_data)
{
	fPaper            = job_data.fPaper;
	fXRes             = job_data.fXRes;
	fYRes             = job_data.fYRes;
	fOrientation      = job_data.fOrientation;
	fScaling          = job_data.fScaling;
	fPaperRect        = job_data.fPaperRect;
	fPrintableRect    = job_data.fPrintableRect;
	fNup              = job_data.fNup;
	fFirstPage        = job_data.fFirstPage;
	fLastPage         = job_data.fLastPage;
	fGamma            = job_data.fGamma;
	fInkDensity       = job_data.fInkDensity;
	fPaperSource      = job_data.fPaperSource;
	fCopies           = job_data.fCopies;
	fCollate          = job_data.fCollate;
	fReverse          = job_data.fReverse;
	fPrintStyle       = job_data.fPrintStyle;
	fBindingLocation  = job_data.fBindingLocation;
	fPageOrder        = job_data.fPageOrder;
	fSettings         = job_data.fSettings;
	fMsg              = job_data.fMsg;
	fColor            = job_data.fColor;
	fDitherType       = job_data.fDitherType;
	return *this;
}

void JobData::load(BMessage *msg, const PrinterCap *cap, Settings settings)
{
	fMsg = msg;
	fSettings = settings;

	if (msg->HasInt32(kJDPaper))
		fPaper = (Paper)msg->FindInt32(kJDPaper);
	else if (cap->isSupport(PrinterCap::kPaper))
		fPaper = ((const PaperCap *)cap->getDefaultCap(PrinterCap::kPaper))->paper;
	else
		fPaper = kA4;

	if (msg->HasInt64(kJDXRes)) {
		int64 xres64; 
		msg->FindInt64(kJDXRes, &xres64);
		fXRes = xres64; 
	} else if (cap->isSupport(PrinterCap::kResolution)) {
		fXRes = ((const ResolutionCap *)cap->getDefaultCap(PrinterCap::kResolution))->xres;
	} else {
		fXRes = 300; 
	}

	if (msg->HasInt64(kJDYRes)) {
		int64 yres64;
		msg->FindInt64(kJDYRes, &yres64);
		fYRes = yres64;
	} else if (cap->isSupport(PrinterCap::kResolution)) {
		fYRes = ((const ResolutionCap *)cap->getDefaultCap(PrinterCap::kResolution))->yres;
	} else {
		fYRes = 300;
	}

	if (msg->HasInt32(kJDOrientation))
		fOrientation = (Orientation)msg->FindInt32(kJDOrientation);
	else if (cap->isSupport(PrinterCap::kOrientation))
		fOrientation = ((const OrientationCap *)cap->getDefaultCap(PrinterCap::kOrientation))->orientation;
	else
		fOrientation = kPortrait;

	if (msg->HasFloat(kJDScaling))
		fScaling = msg->FindFloat(kJDScaling);
	else
		fScaling = 100.0f;

	if (msg->HasRect(kJDPaperRect)) {
		fPaperRect = msg->FindRect(kJDPaperRect);
	}

	if (msg->HasRect(kJDPrintableRect)) {
		fPrintableRect = msg->FindRect(kJDPrintableRect);
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
		fPaperSource = ((const PaperSourceCap *)cap->getDefaultCap(PrinterCap::kPaperSource))->paper_source;
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
		fPrintStyle = ((const PrintStyleCap *)cap->getDefaultCap(PrinterCap::kPrintStyle))->print_style;
	else
		fPrintStyle = kSimplex;

	if (msg->HasInt32(kJDBindingLocation))
		fBindingLocation = (BindingLocation)msg->FindInt32(kJDBindingLocation);
	else if (cap->isSupport(PrinterCap::kBindingLocation))
		fBindingLocation = ((const BindingLocationCap *)cap->getDefaultCap(PrinterCap::kBindingLocation))->binding_location;
	else
		fBindingLocation = kLongEdgeLeft;

	if (msg->HasInt32(kJDPageOrder))
		fPageOrder = (PageOrder)msg->FindInt32(kJDPageOrder);
	else
		fPageOrder = kAcrossFromLeft;

	if (msg->HasBool(kJDColor))
		fColor = msg->FindBool(kJDColor);
	else
		fColor = false;
	
	if (msg->HasInt32(kJDDitherType))
		fDitherType = (Halftone::DitherType)msg->FindInt32(kJDDitherType);
	else
		fDitherType = Halftone::kTypeFloydSteinberg;
}

void JobData::save(BMessage *msg)
{
	if (msg == NULL) {
		msg = fMsg;
	}

	// page settings
	if (msg->HasInt32(kJDPaper))
		msg->ReplaceInt32(kJDPaper, fPaper);
	else
		msg->AddInt32(kJDPaper, fPaper);

	if (msg->HasInt64(kJDXRes))
		msg->ReplaceInt64(kJDXRes, fXRes);
	else
		msg->AddInt64(kJDXRes, fXRes);
	
	if (msg->HasInt64(kJDYRes))
		msg->ReplaceInt64(kJDYRes, fYRes);
	else
		msg->AddInt64(kJDYRes, fYRes);

	if (msg->HasInt32(kJDOrientation))
		msg->ReplaceInt32(kJDOrientation, fOrientation);
	else
		msg->AddInt32(kJDOrientation, fOrientation);

	if (msg->HasFloat(kJDScaling))
		msg->ReplaceFloat(kJDScaling, fScaling);
	else
		msg->AddFloat(kJDScaling, fScaling);

	if (msg->HasRect(kJDPaperRect))
		msg->ReplaceRect(kJDPaperRect, fPaperRect);
	else
		msg->AddRect(kJDPaperRect, fPaperRect);

	if (msg->HasRect(kJDPrintableRect))
		msg->ReplaceRect(kJDPrintableRect, fPrintableRect);
	else
		msg->AddRect(kJDPrintableRect, fPrintableRect);

	// page settings end here; don't store job settings in message
	if (fSettings == kPageSettings) return;
	
	// job settings
	if (msg->HasInt32(kJDNup))
		msg->ReplaceInt32(kJDNup, fNup);
	else
		msg->AddInt32(kJDNup, fNup);

	if (msg->HasInt32(kJDFirstPage))
		msg->ReplaceInt32(kJDFirstPage, fFirstPage);
	else
		msg->AddInt32(kJDFirstPage, fFirstPage);

	if (msg->HasInt32(kJDLastPage))
		msg->ReplaceInt32(kJDLastPage, fLastPage);
	else
		msg->AddInt32(kJDLastPage, fLastPage);

	if (msg->HasFloat(kJDGamma))
		msg->ReplaceFloat(kJDGamma, fGamma);
	else
		msg->AddFloat(kJDGamma, fGamma);

	if (msg->HasFloat(kJDInkDensity))
		msg->ReplaceFloat(kJDInkDensity, fInkDensity);
	else
		msg->AddFloat(kJDInkDensity, fInkDensity);

	if (msg->HasInt32(kJDPaperSource))
		msg->ReplaceInt32(kJDPaperSource, fPaperSource);
	else
		msg->AddInt32(kJDPaperSource, fPaperSource);

	if (msg->HasInt32(kJDCopies))
		msg->ReplaceInt32(kJDCopies, fCopies);
	else
		msg->AddInt32(kJDCopies, fCopies);

	if (msg->HasBool(kJDCollate))
		msg->ReplaceBool(kJDCollate, fCollate);
	else
		msg->AddBool(kJDCollate, fCollate);

	if (msg->HasBool(kJDReverse))
		msg->ReplaceBool(kJDReverse, fReverse);
	else
		msg->AddBool(kJDReverse, fReverse);

	if (msg->HasInt32(kJDPrintStyle))
		msg->ReplaceInt32(kJDPrintStyle, fPrintStyle);
	else
		msg->AddInt32(kJDPrintStyle, fPrintStyle);

	if (msg->HasInt32(kJDBindingLocation))
		msg->ReplaceInt32(kJDBindingLocation, fBindingLocation);
	else
		msg->AddInt32(kJDBindingLocation, fBindingLocation);

	if (msg->HasInt32(kJDPageOrder))
		msg->ReplaceInt32(kJDPageOrder, fPageOrder);
	else
		msg->AddInt32(kJDPageOrder, fPageOrder);

	if (msg->HasBool(kJDColor))
		msg->ReplaceBool(kJDColor, fColor);
	else
		msg->AddBool(kJDColor, fColor);

	if (msg->HasInt32(kJDDitherType))
		msg->ReplaceInt32(kJDDitherType, fDitherType);
	else
		msg->AddInt32(kJDDitherType, fDitherType);
}

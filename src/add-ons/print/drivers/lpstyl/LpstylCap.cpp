/*
* Copyright 2017, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/
#include "LpstylCap.h"


#define TO72DPI(a)	(a * 72.0f / 360.0f)


static const PaperCap a4(
	"A4",
	true,
	JobData::kA4,
	BRect(0.0f,           0.0f,           TO72DPI(2975.0f), TO72DPI(4210.0f)),
	BRect(TO72DPI(72.0f), TO72DPI(72.0f), TO72DPI(2903.0f), TO72DPI(4138.0f)));

static const PaperCap letter(
	"Letter",
	true,
	JobData::kLetter,
	BRect(0.0f,           0.0f,           TO72DPI(3060.0f), TO72DPI(3960.0f)),
	BRect(TO72DPI(72.0f), TO72DPI(72.0f), TO72DPI(2988.0f), TO72DPI(3888.0f)));


static const ResolutionCap dpi360("360dpi",   true,  1, 360,  360);


static const PaperCap* papers[] = {
	&a4,
	&letter,
};

static const ResolutionCap* resolutions[] = {
	&dpi360,
};

static const ColorCap color("Color", false, JobData::kColor);
static const ColorCap monochrome("Shades of Gray", true, JobData::kMonochrome);

static const ColorCap* colors[] = {
	&color,
	&monochrome
};

// This is required so libprint saves PrinterData in the printer spool dir.
static const ProtocolClassCap proto("Serial", true, 1, "Serial port");

static const ProtocolClassCap* protocols[] = {
	&proto
};


int
LpstylCap::CountCap(CapID capid) const
{
	switch (capid) {
		case kPaper:
			return sizeof(papers) / sizeof(papers[0]);
		case kResolution:
			return sizeof(resolutions) / sizeof(resolutions[0]);
#if 0
		case kPaperSource:
			return sizeof(papersources) / sizeof(papersources[0]);
		case kPrintStyle:
			return sizeof(printstyles) / sizeof(printstyles[0]);
		case kBindingLocation:
			return sizeof(bindinglocations) / sizeof(bindinglocations[0]);
#endif
		case kColor:
			return sizeof(colors) / sizeof(colors[0]);
		case kProtocolClass:
			return sizeof(protocols) / sizeof(protocols[0]);
		default:
			return 0;
		}
}


const BaseCap**
LpstylCap::GetCaps(CapID capid) const
{
	switch (capid) {
		case kPaper:
			return (const BaseCap **)papers;
		case kResolution:
			return (const BaseCap **)resolutions;
#if 0
		case kPaperSource:
			return (const BaseCap **)papersources;
		case kPrintStyle:
			return (const BaseCap **)printstyles;
		case kBindingLocation:
			return (const BaseCap **)bindinglocations;
#endif
		case kProtocolClass:
			return (const BaseCap **)protocols;
		case kColor:
			return (const BaseCap **)colors;
		default:
			return NULL;
	}
}


bool
LpstylCap::Supports(CapID capid) const
{
	switch (capid) {
		case kPaper:
		case kResolution:
		case kColor:
		case kProtocolClass:
			return true;
		case kPaperSource:
		case kPrintStyle:
		case kBindingLocation:
		case kCopyCommand:
		case kHalftone:
		default:
			return false;
	}
}

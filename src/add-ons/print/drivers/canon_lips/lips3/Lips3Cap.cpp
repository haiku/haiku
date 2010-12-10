/*
 * Lips3Cap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


#include "Lips3Cap.h"

#include "PrinterData.h"

#define TO72DPI(a)	(a * 72.0f / 600.0f)

const PaperCap a3(
	"A3",
	false,
	JobData::kA3,
	BRect(0.0f,            0.0f,            TO72DPI(7014.0f), TO72DPI(9920.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(6894.0f), TO72DPI(9800.0f)));

const PaperCap a4(
	"A4",
	true,
	JobData::kA4,
	BRect(0.0f,            0.0f,            TO72DPI(4960.0f), TO72DPI(7014.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4840.0f), TO72DPI(6894.0f)));

const PaperCap a5(
	"A5",
	false,
	JobData::kA5,
	BRect(0.0f,            0.0f,            TO72DPI(3506.0f), TO72DPI(4960.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(3386.0f), TO72DPI(4840.0f)));

const PaperCap japanese_postcard(
	"Japanese Postcard",
	false,
	JobData::kJapanesePostcard,
	BRect(0.0f,           0.0f,             TO72DPI(2362.0f), TO72DPI(3506.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(2242.0f), TO72DPI(3386.0f)));

const PaperCap b4(
	"B4",
	false,
	JobData::kB4,
	BRect(0.0f,            0.0f,            TO72DPI(6070.0f), TO72DPI(8598.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(5950.0f), TO72DPI(8478.0f)));

const PaperCap b5(
	"B5",
	false,
	JobData::kB5,
	BRect(0.0f,            0.0f,            TO72DPI(4298.0f), TO72DPI(6070.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4178.0f), TO72DPI(5950.0f)));

const PaperCap letter(
	"Letter",
	false,
	JobData::kLetter,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(6600.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(6480.0f)));

const PaperCap legal(
	"Legal",
	false,
	JobData::kLegal,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(8400.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(8280.0f)));

const PaperSourceCap autobin("Auto",  true,  JobData::kAuto);
const PaperSourceCap manual("Manual", false, JobData::kManual);
const PaperSourceCap upper("Upper",   false, JobData::kUpper);
const PaperSourceCap lower("Lower",   false, JobData::kLower);

const ResolutionCap dpi300("300dpi",   true, 0, 300,  300);

const PaperCap* papers[] = {
	&a4,
	&a3,
	&a5,
	&b4,
	&b5,
	&letter,
	&legal
};

const PaperSourceCap* papersources[] = {
	&autobin,
	&manual,
	&upper,
	&lower
};

const ResolutionCap* resolutions[] = {
	&dpi300
};

const ColorCap color("Color", false, JobData::kColor);
const ColorCap monochrome("Shades of Gray", true, JobData::kMonochrome);

const ColorCap* colors[] = {
	&color,
	&monochrome
};


Lips3Cap::Lips3Cap(const PrinterData* printer_data)
	:
	PrinterCap(printer_data)
{
}


int
Lips3Cap::CountCap(CapID capid) const
{
	switch (capid) {
		case kPaper:
			return sizeof(papers) / sizeof(papers[0]);
		case kPaperSource:
			return sizeof(papersources) / sizeof(papersources[0]);
		case kResolution:
			return sizeof(resolutions) / sizeof(resolutions[0]);
		case kColor:
			return sizeof(colors) / sizeof(colors[0]);
		default:
			return 0;
	}
}


const
BaseCap **Lips3Cap::GetCaps(CapID capid) const
{
	switch (capid) {
		case kPaper:
			return (const BaseCap **)papers;
		case kPaperSource:
			return (const BaseCap **)papersources;
		case kResolution:
			return (const BaseCap **)resolutions;
		case kColor:
			return (const BaseCap **)colors;
		default:
			return NULL;
	}
}


bool
Lips3Cap::Supports(CapID capid) const
{
	switch (capid) {
		case kPaper:
		case kPaperSource:
		case kResolution:
		case kColor:
		case kCopyCommand:
		case kHalftone:
			return true;
		default:
			return false;
	}
}

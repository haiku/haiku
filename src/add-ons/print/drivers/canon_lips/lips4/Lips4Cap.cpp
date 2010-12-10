/*
 * Lips4Cap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */


#include "Lips4Cap.h"

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

const PaperCap tabloid(
	"Tabloid",
	false,
	JobData::kTabloid,
	BRect(0.0f,            0.0f,            TO72DPI(6600.0), TO72DPI(10200.0)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(6480.0), TO72DPI(10080.0)));

const PaperCap executive(
	"Executive",
	false,
	JobData::kExecutive,
	BRect(0.0f,            0.0f,            TO72DPI(4350.0f), TO72DPI(6300.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4230.0f), TO72DPI(6180.0f)));

const PaperCap japanese_envelope_you4(
	"Japanese Envelope You#4",
	false,
	JobData::kJEnvYou4,
	BRect(0.0f,            0.0f,            TO72DPI(2480.0f), TO72DPI(5550.0f)),
	BRect(TO72DPI(236.0f), TO72DPI(236.0f), TO72DPI(2244.0f), TO72DPI(5314.0f)));
/*
const PaperCap japanese_envelope_kaku2(
	"Japanese Envelope Kaku#2",
	false,
	JobData::kJEnvKaku2,
	BRect(0.0f,            0.0f,            TO72DPI(5568.0f), TO72DPI(7842.0f)),
	BRect(TO72DPI(236.0f), TO72DPI(236.0f), TO72DPI(5432.0f), TO72DPI(7606.0f)));
*/
const PaperSourceCap autobin("Auto",  true,  JobData::kAuto);
const PaperSourceCap manual("Manual", false, JobData::kManual);
const PaperSourceCap upper("Upper",   false, JobData::kUpper);
const PaperSourceCap middle("Middle", false, JobData::kMiddle);
const PaperSourceCap lower("Lower",   false, JobData::kLower);

const ResolutionCap dpi1200("1200dpi", false, 0, 1200, 1200);
const ResolutionCap dpi600("600dpi",   true,  1, 600,  600);
const ResolutionCap dpi300("300dpi",   false, 2, 300,  300);

const PrintStyleCap simplex("Simplex", true,  JobData::kSimplex);
const PrintStyleCap duplex("Duplex",   false, JobData::kDuplex);
const PrintStyleCap booklet("Booklet", false, JobData::kBooklet);

const BindingLocationCap longedge1("Long Edge (left)",     true,
	JobData::kLongEdgeLeft);
const BindingLocationCap longedge2("Long Edge (right)",    false,
	JobData::kLongEdgeRight);
const BindingLocationCap shortedge1("Short Edge (top)",    false,
	JobData::kShortEdgeTop);
const BindingLocationCap shortedge2("Short Edge (bottom)", false,
	JobData::kShortEdgeBottom);

const PaperCap* papers[] = {
	&a4,
	&a3,
	&a5,
	&b4,
	&b5,
	&letter,
	&legal,
	&tabloid,
	&executive,
	&japanese_postcard,
	&japanese_envelope_you4
};

const PaperSourceCap* papersources[] = {
	&autobin,
	&manual,
	&upper,
	&middle,
	&lower
};

const ResolutionCap* resolutions[] = {
	&dpi1200,
	&dpi600,
	&dpi300
};

const PrintStyleCap* printstyles[] = {
	&simplex,
	&duplex,
	&booklet
};

const BindingLocationCap *bindinglocations[] = {
	&longedge1,
	&longedge2,
	&shortedge1,
	&shortedge2
};

const ColorCap color("Color", false, JobData::kColor);
const ColorCap monochrome("Shades of Gray", true, JobData::kMonochrome);

const ColorCap* colors[] = {
	&color,
	&monochrome
};


int
Lips4Cap::CountCap(CapID capid) const
{
	switch (capid) {
		case kPaper:
			return sizeof(papers) / sizeof(papers[0]);
		case kPaperSource:
			return sizeof(papersources) / sizeof(papersources[0]);
		case kResolution:
			return sizeof(resolutions) / sizeof(resolutions[0]);
		case kPrintStyle:
			return sizeof(printstyles) / sizeof(printstyles[0]);
		case kBindingLocation:
			return sizeof(bindinglocations) / sizeof(bindinglocations[0]);
		case kColor:
			return sizeof(colors) / sizeof(colors[0]);
		default:
			return 0;
		}
}


const BaseCap**
Lips4Cap::GetCaps(CapID capid) const
{
	switch (capid) {
		case kPaper:
			return (const BaseCap **)papers;
		case kPaperSource:
			return (const BaseCap **)papersources;
		case kResolution:
			return (const BaseCap **)resolutions;
		case kPrintStyle:
			return (const BaseCap **)printstyles;
		case kBindingLocation:
			return (const BaseCap **)bindinglocations;
		case kColor:
			return (const BaseCap **)colors;
		default:
			return NULL;
	}
}


bool
Lips4Cap::Supports(CapID capid) const
{
	switch (capid) {
		case kPaper:
		case kPaperSource:
		case kResolution:
		case kPrintStyle:
		case kBindingLocation:
		case kColor:
		case kCopyCommand:
		case kHalftone:
			return true;
		default:
			return false;
		}
}

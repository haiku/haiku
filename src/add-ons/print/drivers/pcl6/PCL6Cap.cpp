/*
 * PCL6Cap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003-2007 Michael Pfeiffer.
 */
#include "PCL6Cap.h"

#include "PCL6Config.h"
#include "PCL6Writer.h"
#include "PrinterData.h"

#define TO72DPI(a)	(a * 72.0f / 600.0f)

// since 1.1
const PaperCap letter(
	"Letter",
	false,
	JobData::kLetter,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(6600.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(6480.0f)));

// since 1.1
const PaperCap legal(
	"Legal",
	false,
	JobData::kLegal,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(8400.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(8280.0f)));

// since 1.1
const PaperCap a4(
	"A4",
	true,
	JobData::kA4,
	BRect(0.0f,            0.0f,            TO72DPI(4960.0f), TO72DPI(7014.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4840.0f), TO72DPI(6894.0f)));

// TODO
// since 1.1
const PaperCap exec(
	"Executive",
	false,
	JobData::kA4,
	BRect(0.0f,            0.0f,            TO72DPI(4960.0f), TO72DPI(7014.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4840.0f), TO72DPI(6894.0f)));

// TODO
// since 1.1
const PaperCap ledger(
	"Ledger",
	false,
	JobData::kLegal,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(8400.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(8280.0f)));

// since 1.1
const PaperCap a3(
	"A3",
	false,
	JobData::kA3,
	BRect(0.0f,            0.0f,            TO72DPI(7014.0f), TO72DPI(9920.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(6894.0f), TO72DPI(9800.0f)));

// TODO 
// COM10Envelope
// since 1.1

// MonarchEnvelope
// since 1.1

// C5Envelope
// since 1.1

// DLEnvelope
// since 1.1

// JB4Paper
// since 1.1

// JB5Paper
// since 1.1

// since 2.1
const PaperCap b5(
	"B5",
	false,
	JobData::kB5,
	BRect(0.0f,            0.0f,            TO72DPI(4298.0f), TO72DPI(6070.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4178.0f), TO72DPI(5950.0f)));

// since 1.1
const PaperCap japanese_postcard(
	"Japanese Postcard",
	false,
	JobData::kJapanesePostcard,
	BRect(0.0f,           0.0f,             TO72DPI(2362.0f), TO72DPI(3506.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(2242.0f), TO72DPI(3386.0f)));

// TODO
// JDoublePostcard
// since 1.1

// since 1.1
const PaperCap a5(
	"A5",
	false,
	JobData::kA5,
	BRect(0.0f,            0.0f,            TO72DPI(3506.0f), TO72DPI(4960.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(3386.0f), TO72DPI(4840.0f)));

// TODO 
// A6
// since 2.0

// JB6
// since 2.0
// TBD: Validate. Is this JB4?
const PaperCap b4(
	"B4",
	false,
	JobData::kB4,
	BRect(0.0f,            0.0f,            TO72DPI(6070.0f), TO72DPI(8598.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(5950.0f), TO72DPI(8478.0f)));


// JIS8K
// since 2.1

// JIS16K
// since 2.1

// JISExec
// since 2.1

// since 1.1
const PaperSourceCap defaultSource("Default", false, JobData::kCassette1);
const PaperSourceCap autobin("Auto", true, JobData::kAuto);
const PaperSourceCap manualFeed("Manual Feed", false, JobData::kManual);
const PaperSourceCap multiPurposeTray("Multi Purpose Tray", false,
	JobData::kCassette3);
const PaperSourceCap upperCassette("Upper Cassette", false, JobData::kUpper);
const PaperSourceCap lowerCassette("Lower Cassette", false, JobData::kLower);
const PaperSourceCap envelopeTray("Envelope Tray", false,
	JobData::kCassette2);
// since 2.0:
const PaperSourceCap thridCassette("Thrid Cassette",  false,  JobData::kMiddle);

const ResolutionCap dpi150("150dpi", false, 0, 150, 150);
const ResolutionCap dpi300("300dpi", true, 1, 300, 300);
const ResolutionCap dpi600("600dpi", false, 2, 600, 600);
const ResolutionCap dpi1200("1200dpi", false, 3, 1200, 1200);

const PrintStyleCap simplex("Simplex", true, JobData::kSimplex);
const PrintStyleCap duplex("Duplex", false, JobData::kDuplex);

const ProtocolClassCap pc1_1("PCL 6 Protocol Class 1.1", true,
	PCL6Writer::kProtocolClass1_1,
	"The printer driver supports the following features of protocol class 1.1:"
	"\n"
	"* Monochrome and Color Printing.\n"
	"* Paper Formats: Letter, Legal, A4, A3, A5 and Japanese Postcard.\n"
	"* Paper Sources: Auto, Default, Manual Feed, Multi-Purpose Tray, Upper "
	"and Lower Cassette and Envelope Tray.\n"
	"* Resolutions: 150, 300, 600 and 1200 DPI."
#if ENABLE_RLE_COMPRESSION
	"\n* Compression Method: RLE."
#else
	"\n* Compression Method: None."
#endif
);

const ProtocolClassCap pc2_0("PCL 6 Protocol Class 2.0", false,
	PCL6Writer::kProtocolClass2_0,
	"In addition to features of protocol class 1.1, the printer driver "
	"supports the following features of protocol class 2.0:\n"
	"* Additonal Paper Source: Third Cassette."
//	"\n* JPEG compression (not implemented yet)"
);

const ProtocolClassCap pc2_1("PCL 6 Protocol Class 2.1", false,
	PCL6Writer::kProtocolClass2_1, 
	"In addition to features of previous protocol classes, the printer driver "
	"supports the following features of protocol class 2.1:\n"
	"* Additional Paper Format: B5."
#if ENABLE_DELTA_ROW_COMPRESSION
	"\n* Additional Compression Method: Delta Row Compression."
#endif
);

// Disable until driver supports new features of protocol class 3.0
//	const ProtocolClassCap pc3_0("PCL 6 Protocol Class 3.0", false,
//		PCL6Writer::kProtocolClass3_0, "Protocol Class 3.0");

const PaperCap* papers1_1[] = {
	&letter,
	&legal,
	&a4,
	&a3,
	&a5,
	&b4,
	&japanese_postcard
};

const PaperCap* papers2_1[] = {
	&letter,
	&legal,
	&a4,
	&a3,
	&a5,
	&b4,
	&b5,
	&japanese_postcard
};

const PaperSourceCap* paperSources1_1[] = {
	&autobin,
	&defaultSource,
	&envelopeTray,
	&lowerCassette,
	&upperCassette,
	&manualFeed,
	&multiPurposeTray
};

const PaperSourceCap* paperSources2_0[] = {
	&autobin,
	&defaultSource,
	&envelopeTray,
	&lowerCassette,
	&upperCassette,
	&thridCassette,
	&manualFeed,
	&multiPurposeTray
};

const ResolutionCap* resolutions[] = {
	&dpi150,
	&dpi300,
	&dpi600,
	&dpi1200,
};

const PrintStyleCap* printStyles[] = {
	&simplex,
	&duplex
};

const ProtocolClassCap* protocolClasses[] = {
	&pc1_1,
	&pc2_0,
	&pc2_1
// Disabled until driver supports features of protocol class 3.0
//	&pc3_0
};

const ColorCap color("Color", false, JobData::kColor);
const ColorCap colorCompressionDisabled("Color (No Compression)", false,
	JobData::kColorCompressionDisabled);
const ColorCap monochrome("Shades of Gray", true, JobData::kMonochrome);

const ColorCap* colors[] = {
	&color,
	&colorCompressionDisabled,
	&monochrome
};


PCL6Cap::PCL6Cap(const PrinterData* printer_data)
	:
	PrinterCap(printer_data)
{
}


int
PCL6Cap::CountCap(CapID capid) const
{
	switch (capid) {
		case kPaper:
			if (GetProtocolClass() >= PCL6Writer::kProtocolClass2_1)
				return sizeof(papers2_1) / sizeof(papers2_1[0]);
			return sizeof(papers1_1) / sizeof(papers1_1[0]);
		case kPaperSource:
			if (GetProtocolClass() >= PCL6Writer::kProtocolClass2_0)
				return sizeof(paperSources2_0) / sizeof(paperSources2_0[0]);
			return sizeof(paperSources1_1) / sizeof(paperSources1_1[0]);
		case kResolution:
			return sizeof(resolutions) / sizeof(resolutions[0]);
		case kColor:
			return sizeof(colors) / sizeof(colors[0]);
		case kPrintStyle:
			return sizeof(printStyles) / sizeof(printStyles[0]);
		case kProtocolClass:
			return sizeof(protocolClasses) / sizeof(protocolClasses[0]);
		default:
			return 0;
	}
}


const BaseCap**
PCL6Cap::GetCaps(CapID capid) const
{
	switch (capid) {
		case kPaper:
			if (GetProtocolClass() >= PCL6Writer::kProtocolClass2_1)
				return (const BaseCap **)papers2_1;
			return (const BaseCap**)papers1_1;
		case kPaperSource:
			if (GetProtocolClass() >= PCL6Writer::kProtocolClass2_0)
				return (const BaseCap **)paperSources2_0;
			return (const BaseCap**)paperSources1_1;
		case kResolution:
			return (const BaseCap**)resolutions;
		case kColor:
			return (const BaseCap**)colors;
		case kPrintStyle:
			return (const BaseCap**)printStyles;
		case kProtocolClass:
			return (const BaseCap**)protocolClasses;
		default:
			return NULL;
	}
}


bool
PCL6Cap::Supports(CapID capid) const
{
	switch (capid) {
		case kPaper:
		case kPaperSource:
		case kResolution:
		case kColor:
		case kCopyCommand:
		case kPrintStyle:
		case kProtocolClass:
		case kHalftone:
			return true;
		default:
			return false;
	}
}

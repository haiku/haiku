/*
 * Lips4Cap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "Lips4Cap.h"

#define TO72DPI(a)	(a * 72.0f / 600.0f)

const PaperCap a3(
	"A3",
	false,
	JobData::A3,
	BRect(0.0f,            0.0f,            TO72DPI(7014.0f), TO72DPI(9920.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(6894.0f), TO72DPI(9800.0f)));

const PaperCap a4(
	"A4",
	true,
	JobData::A4,
	BRect(0.0f,            0.0f,            TO72DPI(4960.0f), TO72DPI(7014.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4840.0f), TO72DPI(6894.0f)));

const PaperCap a5(
	"A5",
	false,
	JobData::A5,
	BRect(0.0f,            0.0f,            TO72DPI(3506.0f), TO72DPI(4960.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(3386.0f), TO72DPI(4840.0f)));

const PaperCap japanese_postcard(
	"Japanese Postcard",
	false,
	JobData::JAPANESE_POSTCARD,
	BRect(0.0f,           0.0f,             TO72DPI(2362.0f), TO72DPI(3506.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(2242.0f), TO72DPI(3386.0f)));

const PaperCap b4(
	"B4",
	false,
	JobData::B4,
	BRect(0.0f,            0.0f,            TO72DPI(6070.0f), TO72DPI(8598.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(5950.0f), TO72DPI(8478.0f)));

const PaperCap b5(
	"B5",
	false,
	JobData::B5,
	BRect(0.0f,            0.0f,            TO72DPI(4298.0f), TO72DPI(6070.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4178.0f), TO72DPI(5950.0f)));

const PaperCap letter(
	"Letter",
	false,
	JobData::LETTER,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(6600.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(6480.0f)));

const PaperCap legal(
	"Legal",
	false,
	JobData::LEGAL,
	BRect(0.0f,            0.0f,            TO72DPI(5100.0f), TO72DPI(8400.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4980.0f), TO72DPI(8280.0f)));

const PaperCap tabloid(
	"Tabloid",
	false,
	JobData::TABLOID,
	BRect(0.0f,            0.0f,            TO72DPI(6600.0), TO72DPI(10200.0)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(6480.0), TO72DPI(10080.0)));

const PaperCap executive(
	"Executive",
	false,
	JobData::EXECUTIVE,
	BRect(0.0f,            0.0f,            TO72DPI(4350.0f), TO72DPI(6300.0f)),
	BRect(TO72DPI(120.0f), TO72DPI(120.0f), TO72DPI(4230.0f), TO72DPI(6180.0f)));

const PaperCap japanese_envelope_you4(
	"Japanese Envelope You#4",
	false,
	JobData::JENV_YOU4,
	BRect(0.0f,            0.0f,            TO72DPI(2480.0f), TO72DPI(5550.0f)),
	BRect(TO72DPI(236.0f), TO72DPI(236.0f), TO72DPI(2244.0f), TO72DPI(5314.0f)));
/*
const PaperCap japanese_envelope_kaku2(
	"Japanese Envelope Kaku#2",
	false,
	JobData::JENV_KAKU2,
	BRect(0.0f,            0.0f,            TO72DPI(5568.0f), TO72DPI(7842.0f)),
	BRect(TO72DPI(236.0f), TO72DPI(236.0f), TO72DPI(5432.0f), TO72DPI(7606.0f)));
*/
const PaperSourceCap autobin("Auto",  true,  JobData::AUTO);
const PaperSourceCap manual("Manual", false, JobData::MANUAL);
const PaperSourceCap upper("Upper",   false, JobData::UPPER);
const PaperSourceCap middle("Middle", false, JobData::MIDDLE);
const PaperSourceCap lower("Lower",   false, JobData::LOWER);

const ResolutionCap dpi1200("1200dpi", false, 1200, 1200);
const ResolutionCap dpi600("600dpi",   true,  600,  600);
const ResolutionCap dpi300("300dpi",   false, 300,  300);

const PrintStyleCap simplex("Simplex", true,  JobData::SIMPLEX);
const PrintStyleCap duplex("Duplex",   false, JobData::DUPLEX);
const PrintStyleCap booklet("Booklet", false, JobData::BOOKLET);

const BindingLocationCap longedge1("Long Edge (left)",     true,  JobData::LONG_EDGE_LEFT);
const BindingLocationCap longedge2("Long Edge (right)",    false, JobData::LONG_EDGE_RIGHT);
const BindingLocationCap shortedge1("Short Edge (top)",    false, JobData::SHORT_EDGE_TOP);
const BindingLocationCap shortedge2("Short Edge (bottom)", false, JobData::SHORT_EDGE_BOTTOM);

const PaperCap *papers[] = {
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

const PaperSourceCap *papersources[] = {
	&autobin,
	&manual,
	&upper,
	&middle,
	&lower
};

const ResolutionCap *resolutions[] = {
	&dpi1200,
	&dpi600,
	&dpi300
};

const PrintStyleCap *printstyles[] = {
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


int Lips4Cap::countCap(CAPID capid) const
{
	switch (capid) {
	case PAPER:
		return sizeof(papers) / sizeof(papers[0]);
	case PAPERSOURCE:
		return sizeof(papersources) / sizeof(papersources[0]);
	case RESOLUTION:
		return sizeof(resolutions) / sizeof(resolutions[0]);
	case PRINTSTYLE:
		return sizeof(printstyles) / sizeof(printstyles[0]);
	case BINDINGLOCATION:
		return sizeof(bindinglocations) / sizeof(bindinglocations[0]);
	default:
		return 0;
	}
}

const BaseCap **Lips4Cap::enumCap(CAPID capid) const
{
	switch (capid) {
	case PAPER:
		return (const BaseCap **)papers;
	case PAPERSOURCE:
		return (const BaseCap **)papersources;
	case RESOLUTION:
		return (const BaseCap **)resolutions;
	case PRINTSTYLE:
		return (const BaseCap **)printstyles;
	case BINDINGLOCATION:
		return (const BaseCap **)bindinglocations;
	default:
		return NULL;
	}
}

bool Lips4Cap::isSupport(CAPID capid) const
{
	switch (capid) {
	case PAPER:
	case PAPERSOURCE:
	case RESOLUTION:
	case PRINTSTYLE:
	case BINDINGLOCATION:
		return true;
	default:
		return false;
	}
}

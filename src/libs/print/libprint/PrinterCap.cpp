/*
 * PrinterCap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "PrinterCap.h"
#include "PrinterData.h"

BaseCap::BaseCap(const string &label, bool isDefault)
	:
	fLabel(label),
	fIsDefault(isDefault)
{
}


BaseCap::~BaseCap()
{
}


PaperCap::PaperCap(const string &label, bool isDefault, JobData::Paper paper,
	const BRect &paperRect, const BRect &physicalRect)
	:
	BaseCap(label, isDefault),
	fPaper(paper),
	fPaperRect(paperRect),
	fPhysicalRect(physicalRect)
{
}


int
PaperCap::ID() const
{
	return fPaper;
}


PaperSourceCap::PaperSourceCap(const string &label, bool isDefault,
	JobData::PaperSource paperSource)
	:
	BaseCap(label, isDefault),
	fPaperSource(paperSource)
{
}


int
PaperSourceCap::ID() const
{
	return fPaperSource;
}


ResolutionCap::ResolutionCap(const string &label, bool isDefault,
	int id, int xResolution, int yResolution)
	:
	BaseCap(label, isDefault),
	fID(id),
	fXResolution(xResolution),
	fYResolution(yResolution)
{
}


int
ResolutionCap::ID() const
{
	return fID;
}


OrientationCap::OrientationCap(const string &label, bool isDefault,
	JobData::Orientation orientation)
	:
	BaseCap(label, isDefault),
	fOrientation(orientation)
{
}


int
OrientationCap::ID() const
{
	return fOrientation;
}


PrintStyleCap::PrintStyleCap(const string &label, bool isDefault,
	JobData::PrintStyle printStyle)
	:
	BaseCap(label, isDefault),
	fPrintStyle(printStyle)
{
}


int
PrintStyleCap::ID() const
{
	return fPrintStyle;
}


BindingLocationCap::BindingLocationCap(const string &label, bool isDefault,
	JobData::BindingLocation bindingLocation)
	:
	BaseCap(label, isDefault),
	fBindingLocation(bindingLocation)
{
}


int
BindingLocationCap::ID() const
{
	return fBindingLocation;
}


ColorCap::ColorCap(const string &label, bool isDefault, JobData::Color color)
	:
	BaseCap(label, isDefault),
	fColor(color)
{
}


int
ColorCap::ID() const
{
	return fColor;
}


ProtocolClassCap::ProtocolClassCap(const string &label, bool isDefault,
	int protocolClass, const string &description)
	:
	BaseCap(label, isDefault),
	fProtocolClass(protocolClass),
	fDescription(description)
{
}


int
ProtocolClassCap::ID() const
{
	return fProtocolClass;
}


PrinterCap::PrinterCap(const PrinterData *printer_data)
	: fPrinterData(printer_data), 
	fPrinterID(kUnknownPrinter)
{
}


PrinterCap::~PrinterCap()
{
}


const BaseCap *
PrinterCap::getDefaultCap(CapID category) const
{
	int count = countCap(category);
	if (count <= 0)
		return NULL;

	const BaseCap **base_cap = enumCap(category);
	while (count--) {
		if ((*base_cap)->fIsDefault) {
			return *base_cap;
		}
		base_cap++;
	}

	return enumCap(category)[0];
}


const BaseCap*
PrinterCap::findCap(CapID category, int id) const
{
	int count = countCap(category);
	if (count <= 0)
		return NULL;

	const BaseCap **base_cap = enumCap(category);
	while (count--) {
		if ((*base_cap)->ID() == id) {
			return *base_cap;
		}
		base_cap++;
	}
	return NULL;
}


const BaseCap*
PrinterCap::findCap(CapID category, const char* label) const
{
	int count = countCap(category);
	if (count <= 0)
		return NULL;

	const BaseCap **base_cap = enumCap(category);
	while (count--) {
		if ((*base_cap)->fLabel == label) {
			return *base_cap;
		}
		base_cap++;
	}
	return NULL;

}

int
PrinterCap::getProtocolClass() const {
	return fPrinterData->getProtocolClass();
}


const
PrinterData *PrinterCap::getPrinterData() const
{
	return fPrinterData;
}


int
PrinterCap::getPrinterId() const
{
	return fPrinterID;
}


void
PrinterCap::setPrinterId(int id)
{
	fPrinterID = id;
}


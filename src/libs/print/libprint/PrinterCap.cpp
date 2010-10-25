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


PaperCap::PaperCap(const string &label, bool isDefault, JobData::Paper paper,
	const BRect &paperRect, const BRect &physicalRect)
	:
	BaseCap(label, isDefault),
	fPaper(paper),
	fPaperRect(paperRect),
	fPhysicalRect(physicalRect)
{
}


PaperSourceCap::PaperSourceCap(const string &label, bool isDefault,
	JobData::PaperSource paperSource)
	:
	BaseCap(label, isDefault),
	fPaperSource(paperSource)
{
}


ResolutionCap::ResolutionCap(const string &label, bool isDefault,
	int xResolution, int yResolution)
	:
	BaseCap(label, isDefault),
	fXResolution(xResolution),
	fYResolution(yResolution)
{
}


OrientationCap::OrientationCap(const string &label, bool isDefault,
	JobData::Orientation orientation)
	:
	BaseCap(label, isDefault),
	fOrientation(orientation)
{
}


PrintStyleCap::PrintStyleCap(const string &label, bool isDefault,
	JobData::PrintStyle printStyle)
	:
	BaseCap(label, isDefault),
	fPrintStyle(printStyle)
{
}


BindingLocationCap::BindingLocationCap(const string &label, bool isDefault,
	JobData::BindingLocation bindingLocation)
	:
	BaseCap(label, isDefault),
	fBindingLocation(bindingLocation)
{
}


ColorCap::ColorCap(const string &label, bool isDefault, JobData::Color color)
	:
	BaseCap(label, isDefault),
	fColor(color)
{
}


ProtocolClassCap::ProtocolClassCap(const string &label, bool isDefault,
	int protocolClass, const string &description)
	:
	BaseCap(label, isDefault),
	fProtocolClass(protocolClass),
	fDescription(description)
{
}


PrinterCap::PrinterCap(const PrinterData *printer_data)
	: fPrinterData(printer_data), 
	fPrinterID(kUnknownPrinter)
{
}


PrinterCap::~PrinterCap()
{
}


const BaseCap *PrinterCap::getDefaultCap(CapID id) const
{
	int count = countCap(id);
	if (count > 0) {
		const BaseCap **base_cap = enumCap(id);
		while (count--) {
			if ((*base_cap)->fIsDefault) {
				return *base_cap;
			}
			base_cap++;
		}
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


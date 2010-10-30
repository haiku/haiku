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


const char*
BaseCap::Key() const
{
	return fKey.c_str();
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


int32
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


int32
PaperSourceCap::ID() const
{
	return fPaperSource;
}


ResolutionCap::ResolutionCap(const string &label, bool isDefault,
	int32 id, int xResolution, int yResolution)
	:
	BaseCap(label, isDefault),
	fID(id),
	fXResolution(xResolution),
	fYResolution(yResolution)
{
}


int32
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


int32
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


int32
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


int32
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


int32
ColorCap::ID() const
{
	return fColor;
}


ProtocolClassCap::ProtocolClassCap(const string &label, bool isDefault,
	int32 protocolClass, const string &description)
	:
	BaseCap(label, isDefault),
	fProtocolClass(protocolClass),
	fDescription(description)
{
}


int32
ProtocolClassCap::ID() const
{
	return fProtocolClass;
}


DriverSpecificCap::DriverSpecificCap(const string& label, int32 category,
	Type type)
	:
	BaseCap(label, false),
	fCategory(category),
	fType(type)
{
}


int32
DriverSpecificCap::ID() const
{
	return fCategory;
}


ListItemCap::ListItemCap(const string& label, bool isDefault, int32 id)
	:
	BaseCap(label, isDefault),
	fID(id)
{
}


int32
ListItemCap::ID() const
{
	return fID;
}


PrinterCap::PrinterCap(const PrinterData *printer_data)
	:
	fPrinterData(printer_data)
{
}


PrinterCap::~PrinterCap()
{
}


const BaseCap*
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


template<typename Predicate>
const BaseCap*
PrinterCap::findCap(CapID category, Predicate& predicate) const
{
	int count = countCap(category);
	if (count <= 0)
		return NULL;

	const BaseCap **base_cap = enumCap(category);
	while (count--) {
		if (predicate(*base_cap)) {
			return *base_cap;
		}
		base_cap++;
	}
	return NULL;

}

const BaseCap*
PrinterCap::findCap(CapID category, int id) const
{
	IDPredicate predicate(id);
	return findCap(category, predicate);
}


const BaseCap*
PrinterCap::findCap(CapID category, const char* label) const
{
	LabelPredicate predicate(label);
	return findCap(category, predicate);
}


const BaseCap*
PrinterCap::findCapWithKey(CapID category, const char* key) const
{
	KeyPredicate predicate(key);
	return findCap(category, predicate);
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

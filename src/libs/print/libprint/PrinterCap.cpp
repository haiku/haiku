/*
 * PrinterCap.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "PrinterCap.h"
#include "PrinterData.h"

BaseCap::BaseCap(const string &label)
	:
	fLabel(label)
{
}


BaseCap::~BaseCap()
{
}


const char*
BaseCap::Label() const
{
	return fLabel.c_str();
}


EnumCap::EnumCap(const string &label, bool isDefault)
	:
	BaseCap(label),
	fIsDefault(isDefault)
{
}


const char*
EnumCap::Key() const
{
	return fKey.c_str();
}


PaperCap::PaperCap(const string &label, bool isDefault, JobData::Paper paper,
	const BRect &paperRect, const BRect &physicalRect)
	:
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, isDefault),
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
	EnumCap(label, false),
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
	EnumCap(label, isDefault),
	fID(id)
{
}


int32
ListItemCap::ID() const
{
	return fID;
}


BooleanCap::BooleanCap(const string& label, bool defaultValue)
	:
	BaseCap(label),
	fDefaultValue(defaultValue)
{
}


bool
BooleanCap::DefaultValue() const
{
	return fDefaultValue;
}


IntRangeCap::IntRangeCap(const string& label, int lower, int upper,
	int defaultValue)
	:
	BaseCap(label),
	fLower(lower),
	fUpper(upper),
	fDefaultValue(defaultValue)
{
}


int32
IntRangeCap::Lower() const
{
	return fLower;
}


int32
IntRangeCap::Upper() const
{
	return fUpper;
}


int32
IntRangeCap::DefaultValue() const
{
	return fDefaultValue;
}


DoubleRangeCap::DoubleRangeCap(const string& label, double lower, double upper,
	double defaultValue)
	:
	BaseCap(label),
	fLower(lower),
	fUpper(upper),
	fDefaultValue(defaultValue)
{
}


double
DoubleRangeCap::Lower() const
{
	return fLower;
}


double
DoubleRangeCap::Upper() const
{
	return fUpper;
}


double
DoubleRangeCap::DefaultValue() const
{
	return fDefaultValue;
}


PrinterCap::PrinterCap(const PrinterData *printer_data)
	:
	fPrinterData(printer_data)
{
}


PrinterCap::~PrinterCap()
{
}


const EnumCap*
PrinterCap::getDefaultCap(CapID category) const
{
	int count = countCap(category);
	if (count <= 0)
		return NULL;

	const BaseCap **base_cap = enumCap(category);
	while (count--) {
		const EnumCap* enumCap = dynamic_cast<const EnumCap*>(*base_cap);
		if (enumCap == NULL)
			return NULL;

		if (enumCap->fIsDefault) {
			return enumCap;
		}

		base_cap++;
	}

	return static_cast<const EnumCap*>(enumCap(category)[0]);
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

const EnumCap*
PrinterCap::findCap(CapID category, int id) const
{
	IDPredicate predicate(id);
	return static_cast<const EnumCap*>(findCap(category, predicate));
}


const BaseCap*
PrinterCap::findCap(CapID category, const char* label) const
{
	LabelPredicate predicate(label);
	return findCap(category, predicate);
}


const EnumCap*
PrinterCap::findCapWithKey(CapID category, const char* key) const
{
	KeyPredicate predicate(key);
	return static_cast<const EnumCap*>(findCap(category, predicate));
}


const BooleanCap*
PrinterCap::findBooleanCap(CapID category) const
{
	if (countCap(category) != 1)
		return NULL;
	return dynamic_cast<const BooleanCap*>(enumCap(category)[0]);
}


const IntRangeCap*
PrinterCap::findIntRangeCap(CapID category) const
{
	if (countCap(category) != 1)
		return NULL;
	return dynamic_cast<const IntRangeCap*>(enumCap(category)[0]);
}


const DoubleRangeCap*
PrinterCap::findDoubleRangeCap(CapID category) const
{
	if (countCap(category) != 1)
		return NULL;
	return dynamic_cast<const DoubleRangeCap*>(enumCap(category)[0]);
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

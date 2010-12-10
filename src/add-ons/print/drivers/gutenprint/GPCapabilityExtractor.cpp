/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include "GPCapabilityExtractor.h"

#include "PrinterCap.h"


const char* kInputSlot = "InputSlot";


#if 1
#define GP_PRINT(...) \
	fprintf(stderr, __VA_ARGS__)
#else
#define GP_PRINT(...) \
	{}
#endif

GPCapabilityExtractor::GPCapabilityExtractor(GPCapabilities* capabilities)
	:
	fState(kIgnoreParameter),
	fCapabilities(capabilities),
	fIndex(0),
	fNextDriverSpecificCategoryID(PrinterCap::kDriverSpecificCapabilitiesBegin)
{

}


bool
GPCapabilityExtractor::BeginParameter(const char* name, const char* displayName,
	stp_parameter_class_t parameterClass)
{
	fState = kIgnoreParameter;
	if (strcmp(kPageSize, name) == 0) {
		GP_PRINT("Supported parameter: %s\n", name);
		fState = kExtractPageSizeParameter;
	} else if (strcmp(kResolution, name) == 0) {
		GP_PRINT("Supported parameter: %s\n", name);
		fState = kExtractResolutionParameter;
	} else if (strcmp(kInputSlot, name) == 0) {
		GP_PRINT("Supported parameter: %s\n", name);
		fState = kExtractInputSlotParameter;
	} else if (strcmp(kPrintingMode, name) == 0) {
		GP_PRINT("Supported parameter: %s\n", name);
		fState = kExtractPrintingModeParameter;
	} else {
		GP_PRINT("Parameter: %s - %s\n", name, displayName);
		if (!Supportsed(parameterClass))
			return false;

		fState = kExtractParameter;
		DriverSpecificCap* capability = new DriverSpecificCap(displayName,
			fNextDriverSpecificCategoryID, DriverSpecificCap::kList);
		capability->fKey = name;

		fDriverSpecificCategories.push_back(capability);
	}
	return true;
}


void
GPCapabilityExtractor::DefaultStringParameter(const char* name,
		const char* key)
{
	if (key == NULL)
		fDefaultKey = "";
	else
		fDefaultKey = key;
}


void
GPCapabilityExtractor::StringParameterSize(const char* name, int size)
{
	fIndex = 0;

	switch (fState) {
		case kExtractPageSizeParameter:
			fCapabilities->fPageSizes.SetSize(size);
			break;

		case kExtractResolutionParameter:
			fCapabilities->fResolutions.SetSize(size);
			break;

		case kExtractInputSlotParameter:
			fCapabilities->fInputSlots.SetSize(size);
			break;

		case kExtractPrintingModeParameter:
			fCapabilities->fPrintingModes.SetSize(size);
			break;

		case kExtractParameter:
			fState = kExtractListParameter;
			fCapabilities->fDriverSpecificCapabilities[
							 fNextDriverSpecificCategoryID].SetSize(size);
			break;

		default:
			break;
	}
}


void
GPCapabilityExtractor::StringParameter(const char* name, const char* key,
		const char* displayName)
{
	bool isDefault = fDefaultKey == key;
	EnumCap* capability;

	switch (fState) {
		case kExtractResolutionParameter:
			GP_PRINT("GPCapabilityExtractor: ResolutionParameter expected\n");
			break;

		case kExtractInputSlotParameter:
			capability = new PaperSourceCap(displayName, isDefault,
				static_cast<JobData::PaperSource>(fIndex));
			AddCapability(fCapabilities->fInputSlots, capability, key);
			break;

		case kExtractPrintingModeParameter:
			capability = new ColorCap(displayName, isDefault,
				static_cast<JobData::Color>(fIndex));
			AddCapability(fCapabilities->fPrintingModes, capability, key);
			break;

		case kExtractListParameter:
			capability = new ListItemCap(displayName, isDefault, fIndex);
			AddCapability(fCapabilities->fDriverSpecificCapabilities[
				 fNextDriverSpecificCategoryID], capability, key);
			break;

		default:
			break;
	}

}


void
GPCapabilityExtractor::ResolutionParameter(const char* name, const char* key,
		const char* displayName, int x, int y)
{
	bool isDefault = fDefaultKey == key;
	EnumCap* capability;
	int resolution;

	switch (fState) {
		case kExtractResolutionParameter:
			if (x <= 0 || y <= 0) {
				// usually this is the entry for the "Default" resolution
				// if we want to show this in the UI, we need a way to
				// determine the resolution (x and y) for it, because
				// libprint needs it for rasterization
				fCapabilities->fResolutions.DecreaseSize();
				break;
			}

			// TODO remove this workaround when libprint supports x != y too
			// for now use the maximum resolution to render the page bands
			resolution = max_c(x, y);

			capability = new ResolutionCap(displayName,	isDefault, fIndex,
				resolution, resolution);
			AddCapability(fCapabilities->fResolutions, capability, key);
			break;

		default:
			break;
	}
}


void
GPCapabilityExtractor::PageSizeParameter(const char* name, const char* key,
	const char* displayName, BSize pageSize, BRect imageableArea)
{
	bool isDefault = fDefaultKey == key;
	EnumCap* capability;

	switch (fState) {
		case kExtractPageSizeParameter:
			capability = new PaperCap(displayName, isDefault,
				static_cast<JobData::Paper>(fIndex),
				BRect(0, 0, pageSize.width, pageSize.height),
				imageableArea);
			AddCapability(fCapabilities->fPageSizes, capability, key);
			break;

		default:
			break;
	}
}


void
GPCapabilityExtractor::EndParameter(const char* name)
{
	if (fState == kExtractListParameter) {
		fNextDriverSpecificCategoryID ++;
	}
}


void
GPCapabilityExtractor::BooleanParameter(const char* name,
	const char* displayName, bool defaultValue,
	stp_parameter_class_t parameterClass)
{
	if (!Supportsed(parameterClass))
		return;

	BooleanCap* capability = new BooleanCap(displayName, defaultValue);
	AddDriverSpecificCapability(name, displayName, DriverSpecificCap::kBoolean,
		capability);
}


void
GPCapabilityExtractor::DoubleParameter(const char* name,
	const char* displayName, double lower, double upper, double defaultValue,
	stp_parameter_class_t parameterClass)
{
	if (!Supportsed(parameterClass))
		return;

	DoubleRangeCap* capability = new DoubleRangeCap(displayName, lower, upper,
		defaultValue);
	AddDriverSpecificCapability(name, displayName,
		DriverSpecificCap::kDoubleRange, capability);
}


void
GPCapabilityExtractor::IntParameter(const char* name, const char* displayName,
	int lower, int upper, int defaultValue,
	stp_parameter_class_t parameterClass)
{
	if (!Supportsed(parameterClass))
		return;

	IntRangeCap* capability = new IntRangeCap(displayName, lower, upper,
		defaultValue);
	AddDriverSpecificCapability(name, displayName, DriverSpecificCap::kIntRange,
		capability);
}


void
GPCapabilityExtractor::DimensionParameter(const char* name,
	const char* displayName, int lower, int upper, int defaultValue,
	stp_parameter_class_t parameterClass)
{
	if (!Supportsed(parameterClass))
		return;

	IntRangeCap* capability = new IntRangeCap(displayName, lower, upper,
		defaultValue);
	AddDriverSpecificCapability(name, displayName,
		DriverSpecificCap::kIntDimension, capability);
}


void
GPCapabilityExtractor::EndVisit()
{
	if (fCapabilities->fInputSlots.Size() == 0)
		AddDefaultInputSlot();
	SetDriverSpecificCategories();
}


bool
GPCapabilityExtractor::Supportsed(stp_parameter_class_t parameterClass)
{
	return parameterClass == STP_PARAMETER_CLASS_FEATURE
		|| parameterClass == STP_PARAMETER_CLASS_OUTPUT;
}


void
GPCapabilityExtractor::AddDefaultInputSlot()
{
	BeginParameter(kInputSlot, "Input Slot", STP_PARAMETER_CLASS_FEATURE);
	DefaultStringParameter(kInputSlot, "");
	StringParameterSize(kInputSlot, 1);
	StringParameter(kInputSlot, "", "Default");
	EndParameter(kInputSlot);
}


void
GPCapabilityExtractor::SetDriverSpecificCategories()
{
	int size = fDriverSpecificCategories.size();
	if (size == 0)
		return;

	fCapabilities->fDriverSpecificCategories.SetSize(size);
	struct BaseCap** array = fCapabilities->fDriverSpecificCategories.Array();
	list<DriverSpecificCap*>::iterator it = fDriverSpecificCategories.begin();
	for (int index = 0; it != fDriverSpecificCategories.end(); it ++,
		index ++) {
		array[index] = *it;
	}
}

void
GPCapabilityExtractor::AddCapability(GPArray<struct BaseCap>& array,
	EnumCap* capability, const char* key)
{
	capability->fKey = key;
	array.Array()[fIndex] = capability;
	fIndex ++;
}


void
GPCapabilityExtractor::AddDriverSpecificCapability(const char* name,
	const char*	displayName, DriverSpecificCap::Type type, BaseCap* capability)
{
	DriverSpecificCap* parent = new DriverSpecificCap(displayName,
		fNextDriverSpecificCategoryID, type);
	parent->fKey = name;

	fDriverSpecificCategories.push_back(parent);

	GPArray<struct BaseCap>& array = fCapabilities->fDriverSpecificCapabilities
		[fNextDriverSpecificCategoryID];
	array.SetSize(1);
	array.Array()[0] = capability;

	fNextDriverSpecificCategoryID++;
}

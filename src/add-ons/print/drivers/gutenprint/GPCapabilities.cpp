/*
* Copyright 1999-2000 Y.Takagi. All Rights Reserved.
*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/


#include "GPCapabilities.h"

#include "GPBinding.h"
#include "GPData.h"
#include "PrinterData.h"


GPCapabilities::GPCapabilities(const PrinterData* printer_data)
	:
	PrinterCap(printer_data)
{
	InitCapabilitiesFromGutenprint();
}


GPCapabilities::~GPCapabilities()
{
}

int
GPCapabilities::CountCap(CapID category) const
{
	const GPArray<struct BaseCap>* capabilities;

	switch (category) {
		case kPaper:
			return fPageSizes.Size();
		case kPaperSource:
			return fInputSlots.Size();
		case kResolution:
			return fResolutions.Size();
		case kColor:
			return fPrintingModes.Size();
		case kDriverSpecificCapabilities:
			return fDriverSpecificCategories.Size();

		default:
			capabilities = DriverSpecificCapabilities(category);
			if (capabilities == NULL)
				return 0;
			return capabilities->Size();
	}
}


const BaseCap**
GPCapabilities::GetCaps(CapID category) const
{
	typedef const BaseCap** ArrayType;
	const GPArray<struct BaseCap>* capabilities;

	switch (category) {
		case kPaper:
			return (ArrayType)fPageSizes.Array();
		case kPaperSource:
			return (ArrayType)fInputSlots.Array();
		case kResolution:
			return (ArrayType)fResolutions.Array();
		case kColor:
			return (ArrayType)fPrintingModes.Array();
		case kDriverSpecificCapabilities:
			return (ArrayType)fDriverSpecificCategories.Array();

		default:
			capabilities = DriverSpecificCapabilities(category);
			if (capabilities == NULL)
				return NULL;
			return (ArrayType)capabilities->Array();
	}
}


bool
GPCapabilities::Supports(CapID category) const
{
	switch (category) {
		case kPaper:
		case kPaperSource:
		case kResolution:
		case kColor:
			return true;

		default:
			return CountCap(category) > 0;
	}
}

void
GPCapabilities::InitCapabilitiesFromGutenprint()
{
	const GPData* data = dynamic_cast<const GPData*>(GetPrinterData());
	ASSERT(data != NULL);
	// capabilities are available only after printer model
	// has been selected
	if (data->fGutenprintDriverName == "")
		return;


	GPBinding binding;
	binding.GetCapabilities(data->fGutenprintDriverName.String(), this);
}


const GPArray<struct BaseCap>*
GPCapabilities::DriverSpecificCapabilities(int32 category) const
{
	DriverSpecificCapabilitiesType::const_iterator it =
		fDriverSpecificCapabilities.find(category);
	if (it == fDriverSpecificCapabilities.end())
		return NULL;
	return &it->second;
}


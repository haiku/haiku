/*
* Copyright 1999-2000 Y.Takagi. All Rights Reserved.
*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*
*/
#ifndef GP_CAPABILITIES_H
#define GP_CAPABILITIES_H


#include "PrinterCap.h"

#include "GPArray.h"

#include <map>

class GPCapabilityExtractor;

class GPCapabilities : public PrinterCap {
public:
					GPCapabilities(const PrinterData* printer_data);
					~GPCapabilities();

	virtual	int		countCap(CapID) const;
	virtual	bool	isSupport(CapID) const;
	virtual	const	BaseCap **enumCap(CapID) const;

private:
	void			InitCapabilitiesFromGutenprint();
	const GPArray<struct BaseCap>*
					DriverSpecificCapabilities(int32 category) const;

	GPArray<struct BaseCap>	fPageSizes;
	GPArray<struct BaseCap>	fResolutions;
	GPArray<struct BaseCap>	fInputSlots;
	GPArray<struct BaseCap>	fPrintingModes;

	GPArray<struct BaseCap> fDriverSpecificCategories;

	typedef map<int32, GPArray<struct BaseCap> > DriverSpecificCapabilitiesType;
	DriverSpecificCapabilitiesType fDriverSpecificCapabilities;

	friend class GPCapabilityExtractor;
};


#endif

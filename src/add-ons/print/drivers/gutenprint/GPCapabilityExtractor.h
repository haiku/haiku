/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_CAPABILITY_EXTRACTOR_H
#define GP_CAPABILITY_EXTRACTOR_H

#include "GPParameterVisitor.h"

#include <list>

#include "GPCapabilities.h"

enum GPCapabilityExtractorState {
	kIgnoreParameter,
	kExtractPageSizeParameter,
	kExtractResolutionParameter,
	kExtractInputSlotParameter,
	kExtractPrintingModeParameter,
	kExtractParameter,
	kExtractListParameter,

};

class GPCapabilityExtractor : public GPParameterVisitor
{
public:
			GPCapabilityExtractor(GPCapabilities* cap);

	bool	BeginParameter(const char* name, const char* displayName,
				stp_parameter_class_t parameterClass);
	void	DefaultStringParameter(const char* name,
		const char* key);
	void	StringParameterSize(const char* name, int size);
	void	StringParameter(const char* name, const char* key,
		const char* displayName);
	void	ResolutionParameter(const char* name, const char* key,
		const char* displayName, int x, int y);
	void	PageSizeParameter(const char* name, const char* key,
		const char* displayName, BSize pageSize, BRect imageableArea);
	void	EndParameter(const char* name);
	void	BooleanParameter(const char* name, const char* displayName,
						bool defaultValue,
						stp_parameter_class_t parameterClass);
	void	DoubleParameter(const char* name, const char* displayName,
						double lower, double upper, double defaultValue,
						stp_parameter_class_t parameterClass);
	void	IntParameter(const char* name, const char* displayName, int lower,
						int upper, int defaultValue,
						stp_parameter_class_t parameterClass);
	void	DimensionParameter(const char* name, const char* displayName,
						int lower, int upper, int defaultValue,
						stp_parameter_class_t parameterClass);
	void	EndVisit();

protected:
	bool	Supportsed(stp_parameter_class_t parameterClass);
	void	AddDefaultInputSlot();
	void	SetDriverSpecificCategories();
	void	AddCapability(GPArray<struct BaseCap>& array, EnumCap* capability,
		const char* key);
	void	AddDriverSpecificCapability(const char* name, const char*
				displayName, DriverSpecificCap::Type type, BaseCap* capability);

private:
	GPCapabilityExtractorState	fState;
	GPCapabilities*				fCapabilities;
	int							fIndex;
	BString						fDefaultKey;

	list<DriverSpecificCap*>	fDriverSpecificCategories;
	int32						fNextDriverSpecificCategoryID;
};

#endif

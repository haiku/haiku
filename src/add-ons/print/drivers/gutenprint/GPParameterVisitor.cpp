/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include "GPParameterVisitor.h"

#include <String.h>


const char* kJobMode = "JobMode";
const char* kJob = "Job";

const char* kPrintingMode = "PrintingMode";
const char* kColor = "Color";
const char* kBlackAndWhite = "BW";

const char* kResolution = "Resolution";
const char* kFakeResolutionKey = "";

const char* kPageSize = "PageSize";

const char* kChannelBitDepth = "ChannelBitDepth";


GPParameterVisitor::GPParameterVisitor()
	:
	fVariables(NULL),
	fHasResolutionParameter(false)
{
}


GPParameterVisitor::~GPParameterVisitor()
{
}


void
GPParameterVisitor::Visit(const stp_printer_t* printer)
{
	// this code is based on Gutenprint printer_options.c
	const stp_vars_t* defaultVariables = stp_printer_get_defaults(printer);
	stp_vars_t* variables = stp_vars_create_copy(defaultVariables);
	fVariables = variables;

	stp_set_string_parameter(variables, kJobMode, kJob);

	stp_parameter_t printingMode;
	stp_describe_parameter(variables, kPrintingMode, &printingMode);
	bool isColorPrinter = stp_string_list_is_present(printingMode.bounds.str,
		kColor) != 0;
	stp_parameter_description_destroy(&printingMode);

	if (isColorPrinter)
		stp_set_string_parameter(variables, kPrintingMode, kColor);
	else
		stp_set_string_parameter(variables, kPrintingMode, kBlackAndWhite);

	stp_set_string_parameter(variables, kChannelBitDepth, "8");

	stp_parameter_list_t list = stp_get_parameter_list(variables);
	int size = stp_parameter_list_count(list);

	for (int i = 0; i < size; i ++) {
		const stp_parameter_t* parameter = stp_parameter_list_param(list, i);
		stp_parameter_t description;
		stp_describe_parameter(fVariables, parameter->name, &description);
		VisitParameter(list, parameter, &description);
		stp_parameter_description_destroy(&description);
	}

	// TODO check if this can really happen
	if (!fHasResolutionParameter) {
		AddMissingResolution();
	}

	EndVisit();

	stp_parameter_list_destroy(list);
	stp_vars_destroy(variables);
	fVariables = NULL;
}


void
GPParameterVisitor::AddMissingResolution()
{
	// some printer definitions don't have resolution parameter
	// however "libprint" needs to know it for rasterization

	// TODO find out if other parameters influence the resolution
	// e.g. color vs black and white
	int x, y;
	stp_describe_resolution(fVariables, &x, &y);

	BeginParameter(kResolution, "Resolution", STP_PARAMETER_CLASS_FEATURE);
	DefaultStringParameter(kResolution, kFakeResolutionKey);
	StringParameterSize(kResolution, 1);

	if (x <= 0 || y <= 0) {
		// TODO decide if more resolutions (150, 600) should be possible
		x = 300;
		y = 300;
	}

	BString displayName;
	if (x != y)
		displayName << x << " x " << y << " DPI";
	else
		displayName << x << " DPI";

	ResolutionParameter(kResolution, kFakeResolutionKey, displayName.String(),
		x, y);

	EndParameter(kResolution);
}


void
GPParameterVisitor::VisitParameter(stp_parameter_list_t list,
	const stp_parameter_t* parameter, stp_parameter_t* description)
{
	// TODO decide which parameters should be revealed to user
	// e.g. up to STP_PARAMETER_LEVEL_ADVANCED4;
	// const stp_parameter_level_t kMaxLevel = STP_PARAMETER_LEVEL_ADVANCED4;
	const stp_parameter_level_t kMaxLevel = STP_PARAMETER_LEVEL_BASIC;
	stp_parameter_class_t parameterClass = parameter->p_class;
	if (parameter->read_only ||
		(parameter->p_level > kMaxLevel
			&& strcmp(parameter->name, kResolution) != 0)
		|| (parameterClass != STP_PARAMETER_CLASS_OUTPUT
			&& parameterClass != STP_PARAMETER_CLASS_CORE
			&& parameterClass != STP_PARAMETER_CLASS_FEATURE))
		return;

	if (!description->is_active)
		return;

	switch (description->p_type) {
		case STP_PARAMETER_TYPE_STRING_LIST:
			if (!BeginParameter(description->name, description->text,
				parameterClass))
				return;
			VisitStringList(description);
			EndParameter(description->name);
			break;

		case STP_PARAMETER_TYPE_BOOLEAN:
			VisitBooleanParameter(description, parameterClass);
			break;

		case STP_PARAMETER_TYPE_DOUBLE:
			VisitDoubleParameter(description, parameterClass);
			break;

		case STP_PARAMETER_TYPE_INT:
			VisitIntParameter(description, parameterClass);
			break;

		case STP_PARAMETER_TYPE_DIMENSION:
			VisitDimensionParameter(description, parameterClass);
			break;

		default:
			break;
	}

}


void
GPParameterVisitor::VisitStringList(stp_parameter_t* parameter)
{
	stp_string_list_t* list = parameter->bounds.str;
	int count = stp_string_list_count(list);
	if (count <= 0)
		return;

	const char* name = parameter->name;
	if (parameter->is_mandatory)
		DefaultStringParameter(name, parameter->deflt.str);
	else
		DefaultStringParameter(name, NULL);

	StringParameterSize(name, count);

	for (int i = 0; i < count; i ++) {
		const stp_param_string_t* entry = stp_string_list_param(list, i);
		const char* key = entry->name;
		const char* displayName = entry->text;
		if (strcmp(name, kResolution) == 0) {
			stp_set_string_parameter(fVariables, kResolution, key);

			int x, y;
			stp_describe_resolution(fVariables, &x, &y);

			ResolutionParameter(name, key, displayName, x, y);

			fHasResolutionParameter = true;
		} else if (strcmp(name, kPageSize) == 0) {
			stp_set_string_parameter(fVariables, kPageSize, key);

			stp_dimension_t width;
			stp_dimension_t height;
			stp_get_media_size(fVariables, &width, &height);
			BSize pageSize(width, height);

			stp_dimension_t left, right, top, bottom;
			stp_get_imageable_area(fVariables, &left, &right, &bottom, &top);
			BRect imageableArea(left, top, right, bottom);

			PageSizeParameter(name, key, displayName, pageSize, imageableArea);
		} else {
			StringParameter(name, key, displayName);
		}
	}
}


void
GPParameterVisitor::VisitBooleanParameter(stp_parameter_t* description,
	stp_parameter_class_t parameterClass)
{
	bool defaultValue = true;
	if (description->is_mandatory)
		defaultValue = description->deflt.boolean;
	BooleanParameter(description->name, description->text, defaultValue,
		parameterClass);
}


void
GPParameterVisitor::VisitDoubleParameter(stp_parameter_t* description,
			stp_parameter_class_t parameterClass)
{
	const char* name = description->name;
	const char* text = description->text;
	double lower = description->bounds.dbl.lower;
	double upper = description->bounds.dbl.upper;
	double defaultValue = description->deflt.dbl;
	if (lower <= defaultValue && defaultValue <= upper)
		DoubleParameter(name, text, lower, upper, defaultValue, parameterClass);
}


void
GPParameterVisitor::VisitIntParameter(stp_parameter_t* description,
			stp_parameter_class_t parameterClass)
{
	const char* name = description->name;
	const char* text = description->text;
	int lower = description->bounds.integer.lower;
	int upper = description->bounds.integer.upper;
	int defaultValue = description->deflt.integer;
	if (lower <= defaultValue && defaultValue <= upper)
		IntParameter(name, text, lower, upper, defaultValue, parameterClass);
}


void
GPParameterVisitor::VisitDimensionParameter(stp_parameter_t* description,
			stp_parameter_class_t parameterClass)
{
	const char* name = description->name;
	const char* text = description->text;
	double lower = description->bounds.dimension.lower;
	double upper = description->bounds.dimension.upper;
	double defaultValue = description->deflt.dimension;
	if (lower <= defaultValue && defaultValue <= upper)
		DimensionParameter(name, text, lower, upper, defaultValue,
			parameterClass);
}

//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _SETTINGS_TOOLS__H
#define _SETTINGS_TOOLS__H

#include <driver_settings.h>


// TODO: remove this as soon as we get the extended driver_settings API
extern driver_settings *dup_driver_settings(const driver_settings *settings);
extern void free_driver_settings(driver_settings *settings);

extern bool equal_driver_settings(const driver_settings *lhs,
	const driver_settings *rhs);
extern bool equal_driver_parameters(const driver_parameter *lhs,
	const driver_parameter *rhs);
extern bool equal_interface_settings(const driver_settings *lhs,
	const driver_settings *rhs);
	// this compares only the relevant parts of the interface settings

extern ppp_side get_side_string_value(const char *sideString, ppp_side unknownValue);
extern bool get_boolean_value(const char *string, bool unknownValue);
extern const driver_parameter *get_parameter_with_name(const char *name,
	const driver_settings *settings);
extern const char *get_settings_value(const char *name,
	const driver_settings *settings);


inline
const driver_parameter*
get_parameter_with_name(const char *name, const driver_parameter *parameters)
{
	return get_parameter_with_name(name,
		parameters ? (driver_settings*) &parameters->parameter_count : NULL);
}


inline
const char*
get_parameter_value(const char *name, const driver_parameter *parameters)
{
	return get_settings_value(name,
		parameters ? (driver_settings*) &parameters->parameter_count : NULL);
}


#endif

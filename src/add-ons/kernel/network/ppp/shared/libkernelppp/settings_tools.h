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

driver_settings *dup_driver_settings(const driver_settings *settings);
void free_driver_settings(driver_settings *settings);

bool equal_driver_settings(const driver_settings *lhs, const driver_settings *rhs);
bool equal_driver_parameters(const driver_parameter *lhs, const driver_parameter *rhs);

ppp_side get_side_string_value(const char *sideString, ppp_side unknownValue);
bool get_boolean_value(const char *string, bool unknownValue);
const char *get_settings_value(const char *name, const driver_settings *settings);


inline
const char*
get_parameter_value(const char *name, const driver_parameter *parameters)
{
	return get_settings_value(name,
		parameters ? (driver_settings*) &parameters->parameter_count : NULL);
}


#endif

//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _SETTINGS_TOOLS__H
#define _SETTINGS_TOOLS__H

// remove this as soon as we get the extended driver_settings API

struct driver_settings;
struct driver_parameter;


driver_settings *dup_driver_settings(const driver_settings *settings);
void free_driver_settings(driver_settings *settings);


ppp_side get_side_string_value(const char *sideString, ppp_side unknownValue);
bool get_boolean_value(const char *string, bool unknownValue);
const char *get_settings_value(const char *name, const driver_settings *settings);
const char *get_parameter_value(const char *name, const driver_parameter *parameters)
	{ return get_settings_value(name, (driver_settings*) &parameters->parameter_count); }

#endif

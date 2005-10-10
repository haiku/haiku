/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef _SETTINGS_TOOLS__H
#define _SETTINGS_TOOLS__H

#include <driver_settings.h>


// TODO: remove this as soon as we get the extended driver_settings API
extern driver_settings *dup_driver_settings(const driver_settings *settings);
extern void free_driver_settings(driver_settings *settings);
extern void free_driver_parameter(driver_parameter *parameter);
extern void free_driver_parameter_fields(driver_parameter *parameter);

extern driver_settings *new_driver_settings();
extern driver_parameter *new_driver_parameter(const char *name);
extern bool copy_driver_parameter(const driver_parameter *from, driver_parameter *to);
extern bool set_driver_parameter_name(const char *name, driver_parameter *parameter);
extern bool add_driver_parameter_value(const char *value, driver_parameter *to);
extern bool add_driver_parameter(driver_parameter *add, driver_settings *to);

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
bool
add_driver_parameter(driver_parameter *add, driver_parameter *to)
{
	if(!to)
		return false;
	
	return add_driver_parameter(add, (driver_settings*) &to->parameter_count);
}


inline
const driver_parameter*
get_parameter_with_name(const char *name, const driver_parameter *parameters)
{
	if(!parameters)
		return NULL;
	
	return get_parameter_with_name(name,
		(driver_settings*) &parameters->parameter_count);
}


inline
const char*
get_parameter_value(const char *name, const driver_parameter *parameters)
{
	if(!parameters)
		return NULL;
	
	return get_settings_value(name, (driver_settings*) &parameters->parameter_count);
}


#endif

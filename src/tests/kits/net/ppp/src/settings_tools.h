#ifndef _SETTINGS_TOOLS__H
#define _SETTINGS_TOOLS__H

struct driver_settings;
struct driver_parameter;


driver_settings *dup_driver_settings(const driver_settings *settings);
void free_driver_settings(driver_settings *settings);

void add_settings(const driver_settings *from, driver_settings *to);
void add_settings(const driver_settings *from, driver_parameter *to)
	{ add_settings(from, (driver_settings*) &to->parameter_count); }
void add_parameter(const driver_parameter *from, driver_settings *to)
	{ add_settings((driver_settings*) &from->parameter_count, to); }
void add_parameter(const driver_parameter *from, driver_parameter *to)
	{ add_settings((driver_settings*) &from->parameter_count,
		(driver_settings*) &to->parameter_count); }

bool get_boolean_value(const char *string, bool unknownValue);
const char *get_settings_value(const char *name, const driver_settings *settings);


#endif

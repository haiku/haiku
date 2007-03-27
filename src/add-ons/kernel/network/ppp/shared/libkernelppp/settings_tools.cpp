/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <KPPPDefs.h>
#include <driver_settings.h>

#include "settings_tools.h"

#include <cstring>
#include <malloc.h>


static const char *sSkipInterfaceParameters[] = {
	PPP_ASK_BEFORE_CONNECTING_KEY,
	PPP_DISONNECT_AFTER_IDLE_SINCE_KEY,
	PPP_CONNECT_RETRIES_LIMIT_KEY,
	PPP_CONNECT_RETRY_DELAY_KEY,
	PPP_AUTO_RECONNECT_KEY,
	PPP_RECONNECT_DELAY_KEY,
	NULL
};


driver_settings*
dup_driver_settings(const driver_settings *dup)
{
	if (!dup)
		return NULL; // we got a NULL pointer, so return nothing
	
	driver_settings *ret = new_driver_settings();
	
	ret->parameter_count = dup->parameter_count;
	
	if (ret->parameter_count > 0) {
		ret->parameters = (driver_parameter*)
			malloc(ret->parameter_count * sizeof(driver_parameter));
		memset(ret->parameters, 0, ret->parameter_count * sizeof(driver_parameter));
	} else
		ret->parameters = NULL;
	
	for (int32 index = 0; index < ret->parameter_count; index++)
		copy_driver_parameter(&dup->parameters[index], &ret->parameters[index]);
	
	return ret;
}


void
free_driver_settings(driver_settings *settings)
{
	if (!settings)
		return;
	
	for (int32 index = 0; index < settings->parameter_count; index++)
		free_driver_parameter_fields(&settings->parameters[index]);
	
	free(settings->parameters);
	free(settings);
}


void
free_driver_parameter(driver_parameter *parameter)
{
	free_driver_parameter_fields(parameter);
	free(parameter);
}


void
free_driver_parameter_fields(driver_parameter *parameter)
{
	free(parameter->name);
	
	for (int32 index = 0; index < parameter->value_count; index++)
		free(parameter->values[index]);
	
	free(parameter->values);
	
	for (int32 index = 0; index < parameter->parameter_count; index++)
		free_driver_parameter_fields(&parameter->parameters[index]);
	
	free(parameter->parameters);
}


driver_settings*
new_driver_settings()
{
	driver_settings *settings = (driver_settings*) malloc(sizeof(driver_settings));
	memset(settings, 0, sizeof(driver_settings));
	
	return settings;
}


driver_parameter*
new_driver_parameter(const char *name)
{
	driver_parameter *parameter = (driver_parameter*) malloc(sizeof(driver_parameter));
	memset(parameter, 0, sizeof(driver_parameter));
	
	set_driver_parameter_name(name, parameter);
	
	return parameter;
}


bool
copy_driver_parameter(const driver_parameter *from, driver_parameter *to)
{
	if (!from || !to)
		return false;
	
	free_driver_parameter_fields(to);
	
	if (from->name)
		to->name = strdup(from->name);
	else
		to->name = NULL;
	
	to->value_count = from->value_count;
	if (from->value_count > 0)
		to->values = (char**) malloc(from->value_count * sizeof(char*));
	else
		to->values = NULL;
	
	for (int32 index = 0; index < to->value_count; index++)
		to->values[index] = strdup(from->values[index]);
	
	to->parameter_count = from->parameter_count;
	
	if (to->parameter_count > 0) {
		to->parameters =
			(driver_parameter*) malloc(to->parameter_count * sizeof(driver_parameter));
		memset(to->parameters, 0, to->parameter_count * sizeof(driver_parameter));
	} else
		to->parameters = NULL;
	
	for (int32 index = 0; index < to->parameter_count; index++)
		copy_driver_parameter(&from->parameters[index], &to->parameters[index]);
	
	return true;
}


bool
set_driver_parameter_name(const char *name, driver_parameter *parameter)
{
	if (!parameter)
		return false;
	
	free(parameter->name);
	
	if (name)
		parameter->name = strdup(name);
	else
		parameter->name = NULL;
	
	return true;
}


bool
add_driver_parameter_value(const char *value, driver_parameter *to)
{
	if (!value || !to)
		return false;
	
	int32 oldCount = to->value_count;
	char **old = to->values;
	
	to->values = (char**) malloc((oldCount + 1) * sizeof(char*));
	
	if (!to->values) {
		to->values = old;
		return false;
	}
	
	to->value_count++;
	memcpy(to->values, old, oldCount * sizeof(char*));
	to->values[oldCount] = strdup(value);
	
	return true;
}


bool
add_driver_parameter(driver_parameter *add, driver_settings *to)
{
	if (!add || !to)
		return false;
	
	int32 oldCount = to->parameter_count;
	driver_parameter *old = to->parameters;
	
	to->parameters =
		(driver_parameter*) malloc((oldCount + 1) * sizeof(driver_parameter));
	
	if (!to->parameters) {
		to->parameters = old;
		return false;
	}
	
	to->parameter_count++;
	memcpy(to->parameters, old, oldCount * sizeof(driver_parameter));
	memcpy(to->parameters + oldCount, add, sizeof(driver_parameter));
	
	return true;
}


bool
equal_driver_settings(const driver_settings *lhs, const driver_settings *rhs)
{
	if (!lhs && !rhs)
		return true;
	else if (!lhs || !rhs)
		return false;
	
	if (lhs->parameter_count != rhs->parameter_count)
		return false;
	
	for (int32 index = 0; index < lhs->parameter_count; index++) {
		if (!equal_driver_parameters(&lhs->parameters[index], &rhs->parameters[index]))
			return false;
	}
	
	return true;
}


bool
equal_driver_parameters(const driver_parameter *lhs, const driver_parameter *rhs)
{
	if (!lhs && !rhs)
		return true;
	else if (!lhs || !rhs)
		return false;
	
	if (lhs->name && rhs->name) {
		if (strcmp(lhs->name, rhs->name))
			return false;
	} else if (lhs->name != rhs->name)
		return false;
	
	if (lhs->value_count != rhs->value_count
			|| lhs->parameter_count != rhs->parameter_count)
		return false;
	
	for (int32 index = 0; index < lhs->value_count; index++) {
		if (strcmp(lhs->values[index], rhs->values[index]))
			return false;
	}
	
	for (int32 index = 0; index < lhs->parameter_count; index++) {
		if (!equal_driver_parameters(&lhs->parameters[index], &rhs->parameters[index]))
			return false;
	}
	
	return true;
}


bool
skip_interface_parameter(const driver_parameter *parameter)
{
	if (!parameter || !parameter->name)
		return false;
	
	for (int32 index = 0; sSkipInterfaceParameters[index]; index++)
		if (!strcasecmp(parameter->name, sSkipInterfaceParameters[index]))
			return true;
	
	return false;
}


bool
equal_interface_settings(const driver_settings *lhs, const driver_settings *rhs)
{
	if (!lhs && !rhs)
		return true;
	else if (!lhs || !rhs)
		return false;
	
	int32 lhsIndex = 0, rhsIndex = 0;
	for (; lhsIndex < lhs->parameter_count; lhsIndex++) {
		if (skip_interface_parameter(&lhs->parameters[lhsIndex]))
			continue;
		
		for (; rhsIndex < rhs->parameter_count; rhsIndex++)
			if (!skip_interface_parameter(&rhs->parameters[rhsIndex]))
				break;
		
		if (rhsIndex >= rhs->parameter_count)
			return false;
		
		if (!equal_driver_parameters(&lhs->parameters[lhsIndex],
				&rhs->parameters[rhsIndex]))
			return false;
	}
	
	for (; rhsIndex < rhs->parameter_count; rhsIndex++)
		if (!skip_interface_parameter(&rhs->parameters[rhsIndex]))
			return false;
	
	return true;
}


ppp_side
get_side_string_value(const char *sideString, ppp_side unknownValue)
{
	if (!sideString)
		return unknownValue;
	
	if (!strcasecmp(sideString, "local"))
		return PPP_LOCAL_SIDE;
	if (!strcasecmp(sideString, "peer"))
		return PPP_PEER_SIDE;
	if (!strcasecmp(sideString, "none")
			|| !strcasecmp(sideString, "no"))
		return PPP_NO_SIDE;
	if (!strcasecmp(sideString, "both"))
		return PPP_BOTH_SIDES;
	
	// no correct value has been found => return default value
	return unknownValue;
}


bool
get_boolean_value(const char *string, bool unknownValue)
{
	if (!string)
		return unknownValue;
	
	if (!strcmp(string, "1")
			|| !strcasecmp(string, "true")
			|| !strcasecmp(string, "yes")
			|| !strcasecmp(string, "on")
			|| !strcasecmp(string, "enable")
			|| !strcasecmp(string, "enabled"))
		return true;
	
	if (!strcmp(string, "0")
			|| !strcasecmp(string, "false")
			|| !strcasecmp(string, "no")
			|| !strcasecmp(string, "off")
			|| !strcasecmp(string, "disable")
			|| !strcasecmp(string, "disabled"))
		return false;
	
	// no correct value has been found => return default value
	return unknownValue;
}


const driver_parameter*
get_parameter_with_name(const char *name, const driver_settings *settings)
{
	if (!name || !settings)
		return NULL;
	
	for (int32 index = 0; index < settings->parameter_count; index++)
		if (!strcasecmp(settings->parameters[index].name, name))
			return &settings->parameters[index];
	
	return NULL;
}


const char*
get_settings_value(const char *name, const driver_settings *settings)
{
	const driver_parameter *parameter = get_parameter_with_name(name, settings);
	
	if (parameter && parameter->value_count > 0 && parameter->values)
		return parameter->values[0];
	
	return NULL;
}

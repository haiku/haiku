//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPDefs.h>
#include <driver_settings.h>

#include "settings_tools.h"

#include <cstring>
#include <malloc.h>


static void copy_parameter(const driver_parameter *from, driver_parameter *to);
static void free_driver_parameter(driver_parameter *p);


driver_settings*
dup_driver_settings(const driver_settings *dup)
{
	if(!dup)
		return NULL; // we got a NULL pointer, so return nothing
	
	driver_settings *ret = (driver_settings*) malloc(sizeof(driver_settings));
	
	ret->parameter_count = dup->parameter_count;
	
	if(ret->parameter_count > 0)
		ret->parameters =
			(driver_parameter*) malloc(ret->parameter_count * sizeof(driver_parameter));
	else
		ret->parameters = NULL;
	
	for(int32 index = 0; index < ret->parameter_count; index++)
		copy_parameter(&dup->parameters[index], &ret->parameters[index]);
	
	return ret;
}


static
void
copy_parameter(const driver_parameter *from, driver_parameter *to)
{
	to->name = strdup(from->name);
	to->value_count = from->value_count;
	
	if(to->value_count > 0)
		to->values = (char**) malloc(to->value_count * sizeof(char*));
	else
		to->values = NULL;
	
	for(int32 index = 0; index < to->value_count; index++)
		to->values[index] = strdup(from->values[index]);
	
	to->parameter_count = from->parameter_count;
	
	if(to->parameter_count > 0)
		to->parameters =
			(driver_parameter*) malloc(to->parameter_count * sizeof(driver_parameter));
	else
		to->parameters = NULL;
	
	for(int32 index = 0; index < to->parameter_count; index++)
		copy_parameter(&from->parameters[index], &to->parameters[index]);
}


void
free_driver_settings(driver_settings *settings)
{
	if(!settings)
		return;
	
	for(int32 index = 0; index < settings->parameter_count; index++)
		free_driver_parameter(&settings->parameters[index]);
	
	free(settings->parameters);
	free(settings);
}


static
void
free_driver_parameter(driver_parameter *p)
{
	free(p->name);
	
	for(int32 index = 0; index < p->value_count; index++)
		free(p->values[index]);
	
	free(p->values);
	
	for(int32 index = 0; index < p->parameter_count; index++)
		free_driver_parameter(&p->parameters[index]);
	
	free(p->parameters);
}


bool
equal_driver_settings(const driver_settings *lhs, const driver_settings *rhs)
{
	if(!lhs && !rhs)
		return true;
	else if(!lhs || !rhs)
		return false;
	
	if(lhs->parameter_count != rhs->parameter_count)
		return false;
	
	for(int32 index = 0; index < lhs->parameter_count; index++) {
		if(!equal_driver_parameters(&lhs->parameters[index], &rhs->parameters[index]))
			return false;
	}
	
	return true;
}


bool
equal_driver_parameters(const driver_parameter *lhs, const driver_parameter *rhs)
{
	if(!lhs && !rhs)
		return true;
	else if(!lhs || !rhs)
		return false;
	
	if(lhs->name && rhs->name) {
		if(strcmp(lhs->name, rhs->name))
			return false;
	} else if(lhs->name != rhs->name)
		return false;
	
	if(lhs->value_count != rhs->value_count
			|| lhs->parameter_count != rhs->parameter_count)
		return false;
	
	for(int32 index = 0; index < lhs->value_count; index++) {
		if(strcmp(lhs->values[index], rhs->values[index]))
			return false;
	}
	
	for(int32 index = 0; index < lhs->parameter_count; index++) {
		if(!equal_driver_parameters(&lhs->parameters[index], &rhs->parameters[index]))
			return false;
	}
	
	return true;
}


ppp_side
get_side_string_value(const char *sideString, ppp_side unknownValue)
{
	if(!sideString)
		return unknownValue;
	
	if(!strcasecmp(sideString, "local"))
		return PPP_LOCAL_SIDE;
	if(!strcasecmp(sideString, "peer"))
		return PPP_PEER_SIDE;
	if(!strcasecmp(sideString, "none")
			|| !strcasecmp(sideString, "no"))
		return PPP_NO_SIDE;
	if(!strcasecmp(sideString, "both"))
		return PPP_BOTH_SIDES;
	
	// no correct value has been found => return default value
	return unknownValue;
}


bool
get_boolean_value(const char *string, bool unknownValue)
{
	if(!string)
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


const char*
get_settings_value(const char *name, const driver_settings *settings)
{
	if(!name || !settings)
		return NULL;
	
	for(int32 index = 0; index < settings->parameter_count; index++)
		if(!strcasecmp(settings->parameters[index].name, name)
				&& settings->parameters[index].value_count > 0
				&& settings->parameters[index].values)
			return settings->parameters[index].values[0];
	
	return NULL;
}

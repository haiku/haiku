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
	ret->parameters = (driver_parameter*) malloc(ret->parameter_count * sizeof(driver_parameter));
	
	for(int32 i=0; i < ret->parameter_count; i++)
		copy_parameter(&dup->parameters[i], &ret->parameters[i]);
	
	return ret;
}


static
void
copy_parameter(const driver_parameter *from, driver_parameter *to)
{
	to->name = strdup(from->name);
	to->value_count = from->value_count;
	
	to->values = (char**) malloc(to->value_count * sizeof(char*));
	
	for(int32 i=0; i < to->value_count; i++)
		to->values[i] = strdup(from->values[i]);
	
	to->parameter_count = from->parameter_count;
	
	to->parameters = (driver_parameter*) malloc(to->parameter_count * sizeof(driver_parameter));
	
	for(int32 i=0; i < to->parameter_count; i++)
		copy_parameter(&from->parameters[i], &to->parameters[i]);
}


void
free_driver_settings(driver_settings *settings)
{
	for(int32 i=0; i < settings->parameter_count; i++)
		free_driver_parameter(&settings->parameters[i]);
	
	free(settings->parameters);
	free(settings);
}


void
free_driver_parameter(driver_parameter *p)
{
	free(p->name);
	
	for(int32 i=0; i < p->value_count; i++)
		free(p->values[i]);
	
	free(p->values);
	
	for(int32 i=0; i < p->parameter_count; i++)
		free_driver_parameter(&p->parameters[i]);
	
	free(p->parameters);
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
get_string_value(const char *string, bool unknownValue)
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
	
	for(int32 i=0; i < settings->parameter_count; i++)
		if(!strcasecmp(settings->parameters[i].name, name)
			&& settings->parameters[i].value_count > 0)
			return settings->parameters[i].values[0];
	
	return NULL;
}

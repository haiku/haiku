#include "settings_tools.h"

#include <cstring>
#include <driver_settings.h>


static void copy_parameter(const driver_parameter *from, driver_parameter *to);
static void free_driver_parameter(driver_parameter *p);


driver_settings *dup_driver_settings(const driver_settings *dup)
{
	if(!settings)
		return NULL; // we got a NULL pointer, so return nothing
	
	driver_settings *ret = (driver_settings*) malloc(sizeof(driver_settings));
	
	ret->parameter_count = dup->parameter_count;
	ret->parameters = (driver_parameter*) malloc(ret->parameter_count * sizeof(driver_parameter));
	
	for(int32 i=0; i < ret->parameter_count; i++)
		copy_parameter(&dup->parameters[i], &ret->parameters[i]);
	
	return ret;
}


static void copy_parameter(const driver_parameter *from, driver_parameter *to)
{
	to->name = strdup(from->name);
	to->value_count = from->value_count;
	
	to->values = (char**) malloc(values * sizeof(char*));
	
	for(int32 i=0; i < to->value_count; i++)
		to->values[i] = strdup(from->values[i]);
	
	to->parameter_count = from->parameter_count;
	
	to->parameters = (driver_parameter*) malloc(to->parameter_count * sizeof(driver_parameter));
	
	for(int32 i=0; i < to->parameter_count; i++)
		copy_parameter(&from->parameters[i], &to->parameters[i]);
}


void free_driver_settings(driver_settings *settings)
{
	for(int32 i=0; i < settings->parameter_count; i++)
		free_driver_parameter(&settings->parameters[i]);
	
	free(settings->parameters);
	free(settings);
}


void free_driver_parameter(driver_parameter *p)
{
	free(p->name);
	
	for(int32 i=0; i < p->value_count; i++)
		free(p->values[i]);
	
	free(p->values);
	
	for(int32 i=0; i < p->parameter_count; i++)
		free_driver_parameter(&p->parameters[i]);
	
	free(p->parameters);
}


void add_settings(const driver_settings *from, driver_settings *to)
{
	if(!from || !to)
		return;
	
	to->parameters = realloc(to->parameters,
		(to->parameter_count + from->parameter_count) * sizeof(driver_parameter));
	
	for(int32 i=0; i < from->parameter_count; i++)
		copy_parameters(&from->parameters[i], &to->parameters[to->parameter_count++]);
}


bool get_boolean_value(const char *string, bool unknownValue)
{
	if(!string)
		return unknownValue;
	
	if (!strcmp(boolean, "1")
		|| !strcasecmp(boolean, "true")
		|| !strcasecmp(boolean, "yes")
		|| !strcasecmp(boolean, "on")
		|| !strcasecmp(boolean, "enable")
		|| !strcasecmp(boolean, "enabled"))
		return true;

	if (!strcmp(boolean, "0")
		|| !strcasecmp(boolean, "false")
		|| !strcasecmp(boolean, "no")
		|| !strcasecmp(boolean, "off")
		|| !strcasecmp(boolean, "disable")
		|| !strcasecmp(boolean, "disabled"))
		return false;
	
	// no correct value has been found => return default value
	return unknownValue;
}


const char *get_settings_value(const char *name, const driver_settings *settings)
{
	if(!name || !settings)
		return NULL;
	
	for(int32 i=0; i < settings->parameter_count; i++)
		if(!strcasecmp(settings->parameters[i].name, name)
			&& settings->parameters[i].value_count > 0)
			return settings->parameters[i].values[0];
	
	return NULL;
}

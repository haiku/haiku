//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

/*!	\class KPPPProfile
	\brief Manages profiles for KPPPInterface.
	
	PPP modules should store their user-specific and private settings in profiles.
	This allows one interface description to be used by multiple users although
	they have different logins. Temporary profiles are used when you enter a password
	in the connection prompt.
	PPP servers may use \c Request() to communicate with the userland PPP add-ons.
	These add-ons could, for instance, forward all requests to a central server or
	database.
*/

#include <cstdio>

#include <KPPPInterface.h>
#include "settings_tools.h"


//!	Initializes profile.
KPPPProfile::KPPPProfile(KPPPInterface& interface)
	: fInterface(interface),
	fSettings(NULL)
{
}


//!	Frees profile settings.
KPPPProfile::~KPPPProfile()
{
	if(fSettings != Interface().Settings())
		free_driver_settings(fSettings);
}


/*!	\brief Loads profile settings.
	
	Search order for profiles:
	- given profile
	- profile mentioned in interface settings ("profile $type { name $name }")
	- profile description file with name of this interface
	- the interface's settings (to stay compatible with the simple in-settings
		description format)
	
	\param profile The dynamically created profile for this interface (if defined).
	\param interfaceSettings The interface's settings.
*/
void
KPPPProfile::LoadSettings(const driver_settings *profile,
	driver_settings *interfaceSettings)
{
	if(fSettings != Interface().Settings())
		free_driver_settings(fSettings);
	
	// -----------------------------
	// given profile
	// -----------------------------
	if(profile) {
		fSettings = dup_driver_settings(profile);
		return;
	}
	
	// -----------------------------
	// profile in settings and
	// profile with interface's name
	// TODO: try /etc/ppp/profile if local profile could not be found
	// -----------------------------
	const char *name = NULL;
	char path[B_PATH_NAME_LENGTH];
	const driver_parameter *parameter = get_parameter_with_name("profile",
		interfaceSettings);
	
	// TODO: add support for "ppp_server" profile type
	if(parameter && parameter->value_count > 0 && parameter->parameter_count > 0
			&& !strcasecmp(parameter->values[0], "file"))
		name = get_parameter_value("name", parameter);
	else if(Interface().Name())
		name = Interface().Name();
	
	if(name) {
		sprintf(path, "pppidf/profile/%s", name);
		void *handle = load_driver_settings(path);
		if(handle) {
			fSettings = dup_driver_settings(get_driver_settings(handle));
			unload_driver_settings(handle);
			
			if(fSettings)
				return;
		}
	}
	
	// -----------------------------
	// interface's settings
	// -----------------------------
	fSettings = interfaceSettings;
}


/*!	\brief Finds a parameter for a module of type \a type and name \a name.
	
	This method is intended for modules.
	
	\param type Module's type (e.g.: "authenticator").
	\param name Module's name (e.g.: "pap").
	
	\return
		\c NULL: No parameter was found.
*/
const driver_parameter*
KPPPProfile::SettingsFor(const char *type, const char *name) const
{
	if(!Settings() || !type)
		return NULL;
	
	const driver_parameter *parameter;
	for(int32 index = 0; index < Settings()->parameter_count; index++) {
		parameter = &Settings()->parameters[index];
		
		if(!strcasecmp(parameter->name, type)) {
			if(name) {
				for(int32 valueIndex = 0; valueIndex < parameter->value_count;
						valueIndex++)
					if(!strcasecmp(parameter->values[valueIndex], name))
						return parameter;
			} else
				return parameter;
		}
	}
	
	return NULL;
}

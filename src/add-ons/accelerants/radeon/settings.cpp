/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Settings file
	
	We shouldn't really need settings as this info
	should be stored by app_server, but especially
	BWindowScreen programs cannot now about extra
	features/settings, so we need to store the flags
	internally (until I have a better idea ;)
	
	Especially "SwapWindow" should be mode-independant
	(you don't swap monitors when you select another
	workspace, do you?)
*/

#include "radeon_accelerant.h"
#include "generic.h"
#include "GlobalData.h"

#include <FindDirectory.h>
#include <Path.h>
#include <File.h>

void Radeon_ReadSettings( virtual_card *vc )
{
	BPath path;
	int32 tmp;

	vc->swap_displays = false;
	vc->use_laptop_panel = false;
	vc->tv_standard = ts_ntsc;
	
	// this is problematic during boot: if there is	multi-user support,
	// you don't have a user when app_server gets launched;
	// on the other hand, storing settings globally is not user-friendly...
	if( find_directory( B_USER_SETTINGS_DIRECTORY, &path ) != B_OK )
		return;
		
	path.Append( "radeon" );
	
	BFile file( path.Path(), B_READ_ONLY );
	
	if( file.InitCheck() != B_OK )
		return;
		
	BMessage settings;
	
	if( settings.Unflatten( &file ) != B_OK )
		return;
	
	settings.FindBool( "SwapDisplays", &vc->swap_displays );
	settings.FindBool( "UseLaptopPanel", &vc->use_laptop_panel );
	settings.FindInt32( "TVStandard", &tmp );
	
	if( tmp >= 0 && tmp <= ts_max )
		vc->tv_standard = (tv_standard_e)tmp;
}

void Radeon_WriteSettings( virtual_card *vc )
{
	BPath path;
	int32 tmp;
	
	// this is problematic during boot: if there is	multi-user support,
	// you don't have a user when app_server gets launched;
	// on the other hand, storing settings globally is not user-friendly...
	if( find_directory( B_USER_SETTINGS_DIRECTORY, &path ) != B_OK )
		return;
		
	path.Append( "radeon" );
	
	BFile file( path.Path(), B_CREATE_FILE | B_WRITE_ONLY );
	
	if( file.InitCheck() != B_OK )
		return;
		
	BMessage settings;
	
	settings.AddBool( "SwapDisplays", vc->swap_displays );
	settings.AddBool( "UseLaptopPanel", vc->use_laptop_panel );
	tmp = vc->tv_standard;
	settings.AddInt32( "TVStandard", tmp );
	
	settings.Flatten( &file );
}

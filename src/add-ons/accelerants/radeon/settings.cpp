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

#ifdef ENABLE_SETTINGS_FILE
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>
#endif

void Radeon_ReadSettings( virtual_card *vc )
{
#ifdef ENABLE_SETTINGS_FILE
	BPath path;
	int32 tmp;

	// per default we enable combine mode;
	// if actual mode isn't combine mode, we fall back to clone mode	
	vc->wanted_multi_mode = mm_combine;
	vc->swapDisplays = false;
	
	// per default, show overlay on first port
	//vc->whished_overlay_port = 0;
	
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
	
	if( settings.FindBool( "SwapDisplays", &vc->swapDisplays ) != B_OK )
		vc->swapDisplays = false;

	if( settings.FindInt32( "MultiMonitorMode", &tmp ) != B_OK )
		tmp = mm_combine;
		
	switch( tmp ) {
	case mm_none:
	case mm_mirror:
	case mm_combine:
	case mm_clone:
		vc->wanted_multi_mode = (multi_mode_e) tmp;
		break;
	default:
		vc->wanted_multi_mode = mm_combine;
	}
	
	if( settings.FindInt32( "OverlayPort", &tmp ) != B_OK )
		tmp = 0;
		
	//vc->whished_overlay_port = tmp;
#else
	vc->wanted_multi_mode = mm_combine;
	vc->swapDisplays = false;
	vc->swapDisplays = false;
#endif
}

void Radeon_WriteSettings( virtual_card *vc )
{
#ifdef ENABLE_SETTINGS_FILE
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
	
	settings.AddBool( "SwapDisplays", vc->swapDisplays );
	tmp = vc->wanted_multi_mode;
	settings.AddInt32( "MultiMonitorMode", tmp );
	/*tmp = vc->whished_overlay_port;
	settings.AddInt32( "OverlayPort", tmp );*/
	
	settings.Flatten( &file );
#endif
}

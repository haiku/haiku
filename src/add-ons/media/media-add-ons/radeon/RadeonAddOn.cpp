/******************************************************************************
/
/	File:			RadeonAddOn.cpp
/
/	Description:	ATI Radeon Video Capture Media AddOn for BeOS.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <support/Autolock.h>
#include <media/MediaFormats.h>
#include <Directory.h>
#include <Entry.h>
#include <Debug.h>
#include <File.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <storage/FindDirectory.h>
#include <String.h>

#include "RadeonAddOn.h"
#include "RadeonProducer.h"

#define DPRINT(args) { PRINT(("\x1b[0;30;35m")); PRINT(args); PRINT(("\x1b[0;30;47m")); }

CRadeonPlug::CRadeonPlug( CRadeonAddOn *aaddon, const BPath &adev_path, int aid )
  : addon( aaddon ), dev_path( adev_path ), id( aid ), node( NULL )
{
	fFlavorInfo.name = const_cast<char *>("Radeon In");
	fFlavorInfo.info = const_cast<char *>("Radeon Video In Media Node");
	fFlavorInfo.kinds = B_BUFFER_PRODUCER | B_CONTROLLABLE | B_PHYSICAL_INPUT;
	fFlavorInfo.flavor_flags = 0;
	fFlavorInfo.internal_id = aid;
	fFlavorInfo.possible_count = 1;
	
	fFlavorInfo.in_format_count = 0;
	fFlavorInfo.in_format_flags = 0;
	fFlavorInfo.in_formats = NULL;
	
	fFlavorInfo.out_format_count = 4;
	//fFlavorInfo.out_format_count = 1;
	fFlavorInfo.out_format_flags = 0;
	
	fMediaFormat[0].type = B_MEDIA_RAW_VIDEO;
	fMediaFormat[0].u.raw_video = media_raw_video_format::wildcard;
	fMediaFormat[0].u.raw_video.interlace = 1;
	fMediaFormat[0].u.raw_video.display.format = B_RGB32;
	
	fMediaFormat[1].type = B_MEDIA_RAW_VIDEO;
	fMediaFormat[1].u.raw_video = media_raw_video_format::wildcard;
	fMediaFormat[1].u.raw_video.interlace = 1;
	fMediaFormat[1].u.raw_video.display.format = B_RGB16;
	
	fMediaFormat[2].type = B_MEDIA_RAW_VIDEO;
	fMediaFormat[2].u.raw_video = media_raw_video_format::wildcard;
	fMediaFormat[2].u.raw_video.interlace = 1;
	fMediaFormat[2].u.raw_video.display.format = B_RGB15;

	fMediaFormat[3].type = B_MEDIA_RAW_VIDEO;
	fMediaFormat[3].u.raw_video = media_raw_video_format::wildcard;
	fMediaFormat[3].u.raw_video.interlace = 1;
	fMediaFormat[3].u.raw_video.display.format = B_YCbCr422;
	
	/*fMediaFormat[0].type = B_MEDIA_RAW_VIDEO;
	fMediaFormat[0].u.raw_video = media_raw_video_format::wildcard;
	fMediaFormat[0].u.raw_video.interlace = 1;
	fMediaFormat[0].u.raw_video.display.format = B_YCbCr422;*/
	
	fFlavorInfo.out_formats = fMediaFormat;
	
	readSettings();
}

BPath 
CRadeonPlug::getSettingsPath()
{
	BPath path;
	
	if( find_directory( B_USER_CONFIG_DIRECTORY, &path ) != B_OK )
		return BPath();
		
	path.Append( "settings/Media/RadeonIn" );
	
	create_directory( path.Path(), 755 );
		
	BString id_string;
	
	id_string << "settings" << id;
	
	path.Append( id_string.String() );
	
	return path;
}

void 
CRadeonPlug::writeSettings( BMessage *new_settings )
{
	BMessage cur_settings;

	// if new_settings are provided, use them; else, ask node for settings
	// (needed during shutdown where node cannot reply anymore)
	if( new_settings != NULL ) {
		cur_settings = *new_settings;
		
	} else {
		if( node == NULL )
			return;
				
		if( addon->GetConfigurationFor( node, &cur_settings ) != B_OK )
			return;
	}
			
	BMallocIO old_settings_flat, new_settings_flat;
	
	settings.Flatten( &old_settings_flat );
	cur_settings.Flatten( &new_settings_flat );
	
	if( old_settings_flat.BufferLength() == new_settings_flat.BufferLength() &&
		memcmp( old_settings_flat.Buffer(), new_settings_flat.Buffer(), old_settings_flat.BufferLength() ) == 0 )
		return;
	
	settings = cur_settings;

	BPath settings_path = getSettingsPath();
	
	BFile file( settings_path.Path(), B_WRITE_ONLY | B_CREATE_FILE );
	
	settings.Flatten( &file );
}

void
CRadeonPlug::readSettings()
{
	BPath settings_path = getSettingsPath();
	
	BFile file( settings_path.Path(), B_READ_ONLY );
		
	settings.Unflatten( &file );
}


CRadeonAddOn::CRadeonAddOn(image_id imid)
	: BMediaAddOn(imid),
	settings_thread( -1 ), settings_thread_sem( -1 )
{
	DPRINT(("CRadeonAddOn::CRadeonAddOn()\n"));
			
	fInitStatus = B_NO_INIT;

	if( RecursiveScan( "/dev/video/radeon" ) != B_OK )
		return;
		
	if( (settings_thread_sem = create_sem( 0, "Radeon In settings" )) < 0 )
		return;
		
	if( INIT_BEN( plug_lock, "Radeon device list" ) < 0 )
		return;
		
	if( (settings_thread = spawn_thread( 
		settings_writer, "Radeon In settings", B_LOW_PRIORITY, this )) < 0 )
		return;
		
	resume_thread( settings_thread );

	fInitStatus = B_OK;
}

CRadeonAddOn::~CRadeonAddOn()
{
	status_t dummy;
	
	DPRINT(("CRadeonAddOn::~CRadeonAddOn()\n"));
	
	release_sem( settings_thread_sem );
	wait_for_thread( settings_thread, &dummy );
	
	delete_sem( settings_thread_sem );
	
	for( int32 i = 0; i < fDevices.CountItems(); ++i ) {
		CRadeonPlug *plug = (CRadeonPlug *)fDevices.ItemAt(i);
		
		delete plug;
	}
	
	DELETE_BEN( plug_lock );
}


status_t 
CRadeonAddOn::InitCheck(const char **out_failure_text)
{
	DPRINT(("CRadeonAddOn::InitCheck()\n"));
	
	if (fInitStatus < B_OK) {
		*out_failure_text = "Unknown error";
		return fInitStatus;
	}

	return B_OK;
}

int32 
CRadeonAddOn::settings_writer( void *param )
{
	((CRadeonAddOn *)param)->settingsWriter();
	return B_OK;
}

void 
CRadeonAddOn::writeSettings()
{
	ACQUIRE_BEN( plug_lock );
	
	for( int32 i = 0; i < fDevices.CountItems(); ++i ) {
		CRadeonPlug *plug = (CRadeonPlug *)fDevices.ItemAt(i);

		plug->writeSettings( NULL );
	}
	
	RELEASE_BEN( plug_lock );
}

void
CRadeonAddOn::settingsWriter()
{
	while( acquire_sem_etc( settings_thread_sem, 1, B_RELATIVE_TIMEOUT, 10000000 ) == B_TIMED_OUT ) {
		writeSettings();
	}
}

void
CRadeonAddOn::UnregisterNode( BMediaNode *node, BMessage *settings )
{
	ACQUIRE_BEN( plug_lock );
	
	for( int32 i = 0; i < fDevices.CountItems(); ++i ) {
		CRadeonPlug *plug = (CRadeonPlug *)fDevices.ItemAt(i);
		
		if( plug->getNode() == node ) {
			// write last settings, so they don't get lost
			plug->writeSettings( settings );
			plug->setNode( NULL );
			break;
		}
	}
	
	RELEASE_BEN( plug_lock );
}

int32 
CRadeonAddOn::CountFlavors()
{
	DPRINT(("CRadeonAddOn::CountFlavors()\n"));
	
	if (fInitStatus < B_OK)
		return fInitStatus;

	return fDevices.CountItems();
}

/*
 * The pointer to the flavor received only needs to be valid between 
 * successive calls to BCRadeonAddOn::GetFlavorAt().
 */
status_t 
CRadeonAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	DPRINT(("CRadeonAddOn::GetFlavorAt()\n"));
	
	if (fInitStatus < B_OK)
		return fInitStatus;

	if (n < 0 || n >= fDevices.CountItems() )
		return B_BAD_INDEX;

	/* Return the flavor defined in the constructor */
	*out_info = ((CRadeonPlug *)fDevices.ItemAt(n))->getFlavorInfo();
	return B_OK;
}

BMediaNode *
CRadeonAddOn::InstantiateNodeFor(
		const flavor_info *info, BMessage *config, status_t *out_error)
{
	DPRINT(("CRadeonAddOn::InstantiateNodeFor()\n"));
	
	CRadeonProducer *node;

	if (fInitStatus < B_OK)
		return NULL;

	if (info->internal_id < 0 || info->internal_id >= fDevices.CountItems())
		return NULL;

	CRadeonPlug *plug = (CRadeonPlug *)fDevices.ItemAt( info->internal_id );
	
	ACQUIRE_BEN( plug_lock );
	
	if( plug->getNode() != NULL ) {
		*out_error = B_BUSY;
		node = NULL;
		
	} else {
		BMessage single_settings;
		
		// under R5, configuration is always an empty message, so we need to
		// get our own configuration
		if( config == NULL || 1 )
			config = plug->getSettings();
		
		node = new CRadeonProducer( this, plug->getName(), 
			plug->getDeviceName(), info->internal_id, config );
			
		if (node && (node->InitCheck() < B_OK)) {
			*out_error = node->InitCheck();
			delete node;
			node = NULL;
		}
	}
	
	plug->setNode( node );
		
	RELEASE_BEN( plug_lock );

	return node;
}

status_t CRadeonAddOn::GetConfigurationFor(
	BMediaNode *your_node,
	BMessage *into_message )
{
	port_id reply_port;
	CRadeonProducer::configuration_msg msg;
	status_t res;
	
	reply_port = create_port( 1, "GetConfiguration Reply" );
	if( reply_port < 0 )
		return reply_port;
		
	msg.reply_port = reply_port;
	
	res = write_port_etc( your_node->ControlPort(), CRadeonProducer::C_GET_CONFIGURATION, 
		&msg, sizeof( msg ), B_TIMEOUT, 10000000 );
	if( res == B_OK ) {
		ssize_t reply_size;
		CRadeonProducer::configuration_msg_reply *reply;
		int32 code;
		
		reply_size = port_buffer_size_etc( reply_port, B_TIMEOUT, 10000000 );
		if( reply_size < 0 )
			res = reply_size;
		else {
			reply = (CRadeonProducer::configuration_msg_reply *)malloc( reply_size );
		
			res = read_port( reply_port, &code, reply, reply_size );
			if( res == reply_size ) {
				if( code != CRadeonProducer::C_GET_CONFIGURATION_REPLY )
					res = B_ERROR;
				else {
					res = reply->res;
					
					if( res == B_OK )
						res = into_message->Unflatten( &reply->config );
				}
			}
			
			free( reply );
		}
	}
	
	delete_port( reply_port );
	
	return res;
}

status_t
CRadeonAddOn::RecursiveScan(const char* rootPath, BEntry *rootEntry = NULL)
{	
	BDirectory root;
	
	if( rootEntry != NULL )
		root.SetTo( rootEntry );
	else if( rootPath != NULL ) {
		root.SetTo( rootPath );
	} else {
		PRINT(("Error in MultiAudioAddOn::RecursiveScan null params\n"));
		return B_ERROR;
	}
	
	BEntry entry;
	int cur_id = 0;
	
	while( root.GetNextEntry( &entry ) > B_ERROR ) {
		if(entry.IsDirectory()) {
			RecursiveScan( rootPath, &entry );
			
		} else {
			BPath path;
			
			entry.GetPath(&path);
			
			CRadeon device( path.Path() );
			
			if( device.InitCheck() != B_OK)
				continue;
				
			CVIPPort vip_port( device );
		
			// if there is a Rage Theatre, then there should be Video-In	
			if( vip_port.InitCheck() == B_OK &&
				((vip_port.FindVIPDevice( C_THEATER100_VIP_DEVICE_ID ) >= 0) 
					|| (vip_port.FindVIPDevice( C_THEATER200_VIP_DEVICE_ID ) >= 0)))
			{
				fDevices.AddItem( new CRadeonPlug( this, path, cur_id++ ));
			}
		}
	}
	
	return B_OK;
}


BMediaAddOn *
make_media_addon(image_id imid)
{
	return new CRadeonAddOn(imid);
}

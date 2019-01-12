/*
 * Copyright 2001-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */
#ifndef REGISTRAR_DEFS_H
#define REGISTRAR_DEFS_H


//!	API classes - registrar interface.


#include <Errors.h>
#include <Roster.h>


namespace BPrivate {


// names
extern const char* kRAppLooperPortName;


#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	define B_REGISTRAR_SIGNATURE "application/x-vnd.haiku-registrar"
#	define B_REGISTRAR_PORT_NAME "system:roster"
#else
#	define B_REGISTRAR_SIGNATURE "application/x-vnd.test-registrar"
#	define B_REGISTRAR_PORT_NAME "haiku-test:roster"
#endif

#define B_REGISTRAR_AUTHENTICATION_PORT_NAME	"auth"


// message constants
enum {
	// replies
	B_REG_SUCCESS							= 'rgsu',
	B_REG_ERROR								= 'rger',
	B_REG_RESULT							= 'rgrz',

	// general requests
	B_REG_GET_MIME_MESSENGER				= 'rgmm',
	B_REG_GET_CLIPBOARD_MESSENGER			= 'rgcm',
	B_REG_GET_DISK_DEVICE_MESSENGER			= 'rgdm',
	B_REG_SHUT_DOWN							= 'rgsh',

	// roster requests
	B_REG_ADD_APP							= 'rgaa',
	B_REG_COMPLETE_REGISTRATION				= 'rgcr',
	B_REG_IS_APP_REGISTERED					= 'rgip',
	B_REG_REMOVE_PRE_REGISTERED_APP			= 'rgrp',
	B_REG_REMOVE_APP						= 'rgra',
	B_REG_SET_THREAD_AND_TEAM				= 'rgtt',
	B_REG_SET_SIGNATURE						= 'rgss',
	B_REG_GET_APP_INFO						= 'rgai',
	B_REG_GET_APP_LIST						= 'rgal',
	B_REG_UPDATE_ACTIVE_APP					= 'rgua',
	B_REG_BROADCAST							= 'rgbc',
	B_REG_START_WATCHING					= 'rgwa',
	B_REG_STOP_WATCHING						= 'rgsw',
	B_REG_GET_RECENT_DOCUMENTS				= 'rggd',
	B_REG_GET_RECENT_FOLDERS				= 'rggf',
	B_REG_GET_RECENT_APPS					= 'rgga',
	B_REG_ADD_TO_RECENT_DOCUMENTS			= 'rg2d',
	B_REG_ADD_TO_RECENT_FOLDERS				= 'rg2f',
	B_REG_ADD_TO_RECENT_APPS				= 'rg2a',
	B_REG_CLEAR_RECENT_DOCUMENTS			= 'rgxd',
	B_REG_CLEAR_RECENT_FOLDERS				= 'rgxf',
	B_REG_CLEAR_RECENT_APPS					= 'rgxa',
	B_REG_LOAD_RECENT_LISTS					= 'rglr',
	B_REG_SAVE_RECENT_LISTS					= 'rgsr',

	// MIME requests
	B_REG_MIME_SET_PARAM					= 'rgsp',
	B_REG_MIME_DELETE_PARAM					= 'rgdp',
	B_REG_MIME_START_WATCHING				= 'rgwb',
	B_REG_MIME_STOP_WATCHING				= 'rgwe',
	B_REG_MIME_INSTALL						= 'rgin',
	B_REG_MIME_DELETE						= 'rgdl',
	B_REG_MIME_GET_INSTALLED_TYPES			= 'rgit',
	B_REG_MIME_GET_INSTALLED_SUPERTYPES		= 'rgis',
	B_REG_MIME_GET_SUPPORTING_APPS			= 'rgsa',
	B_REG_MIME_GET_ASSOCIATED_TYPES			= 'rgat',
	B_REG_MIME_SNIFF						= 'rgsn',
	B_REG_MIME_UPDATE_MIME_INFO				= 'rgup',
	B_REG_MIME_CREATE_APP_META_MIME			= 'rgca',
	B_REG_MIME_UPDATE_THREAD_FINISHED		= 'rgtf',

	// message runner requests
	B_REG_REGISTER_MESSAGE_RUNNER			= 'rgrr',
	B_REG_UNREGISTER_MESSAGE_RUNNER			= 'rgru',
	B_REG_SET_MESSAGE_RUNNER_PARAMS			= 'rgrx',
	B_REG_GET_MESSAGE_RUNNER_INFO			= 'rgri',

	// internal registrar messages
	B_REG_SHUTDOWN_FINISHED					= 'rgsf',
	B_REG_ROSTER_DEVICE_RESCAN				= 'rgrs',

	// clipboard handler requests
	B_REG_ADD_CLIPBOARD						= 'rgCa',
	B_REG_GET_CLIPBOARD_COUNT				= 'rgCc',
	B_REG_CLIPBOARD_START_WATCHING			= 'rgCw',
	B_REG_CLIPBOARD_STOP_WATCHING			= 'rgCx',
	B_REG_DOWNLOAD_CLIPBOARD				= 'rgCd',
	B_REG_UPLOAD_CLIPBOARD					= 'rgCu',

	// disk device request
	B_REG_NEXT_DISK_DEVICE					= 'rgnx',
	B_REG_GET_DISK_DEVICE					= 'rgdd',
	B_REG_UPDATE_DISK_DEVICE				= 'rgud',
	B_REG_DEVICE_START_WATCHING				= 'rgwd',
	B_REG_DEVICE_STOP_WATCHING				= 'rgsd',

	// debug_server notifications
	B_REG_TEAM_DEBUGGER_ALERT				= 'rtda',

	// authentication requests
	B_REG_GET_PASSWD_DB						= 'rpdb',
	B_REG_GET_GROUP_DB						= 'rgdb',
	B_REG_GET_SHADOW_PASSWD_DB				= 'rsdb',
	B_REG_GET_USER							= 'rgus',
	B_REG_GET_GROUP							= 'rggr',
	B_REG_GET_USER_GROUPS					= 'rgug',
	B_REG_UPDATE_USER						= 'ruus',
	B_REG_DELETE_USER						= 'rdus',
	B_REG_UPDATE_GROUP						= 'rugr',
	B_REG_DELETE_GROUP						= 'rdgr',

	// package watching requests
	B_REG_PACKAGE_START_WATCHING			= 'rgPw',
	B_REG_PACKAGE_STOP_WATCHING				= 'rgPx',
};

// B_REG_MIME_SET_PARAM "which" constants
enum {
	B_REG_MIME_APP_HINT				= 'rgmh',
	B_REG_MIME_ATTR_INFO			= 'rgma',
	B_REG_MIME_DESCRIPTION			= 'rgmd',
	B_REG_MIME_FILE_EXTENSIONS		= 'rgmf',
	B_REG_MIME_ICON					= 'rgmi',
	B_REG_MIME_ICON_FOR_TYPE		= 'rgm4',
	B_REG_MIME_PREFERRED_APP		= 'rgmp',
	B_REG_MIME_SNIFFER_RULE			= 'rgmr',
	B_REG_MIME_SUPPORTED_TYPES		= 'rgms',
};

// B_REG_UPDATE_DISK_DEVICE "update_policy" constants
enum {
  B_REG_DEVICE_UPDATE_CHECK,
  B_REG_DEVICE_UPDATE_CHANGED,
  B_REG_DEVICE_UPDATE_DEVICE_CHANGED,
};

// type constants
enum {
	B_REG_APP_INFO_TYPE				= 'rgai',	// app_info
};

// error constants
#define B_REGISTRAR_ERROR_BASE		(B_ERRORS_END + 1)

enum {
	B_REG_ALREADY_REGISTERED		= B_REGISTRAR_ERROR_BASE,
		// A team tries to register a second time.
	B_REG_APP_NOT_REGISTERED,
	B_REG_APP_NOT_PRE_REGISTERED,
};

// misc constants
enum {
	B_REG_DEFAULT_APP_FLAGS			= B_MULTIPLE_LAUNCH,
	B_REG_APP_LOOPER_PORT_CAPACITY	= 100,
};

// structs

// a flat app_info -- to be found in B_REG_APP_INFO_TYPE message fields
struct flat_app_info {
	thread_id	thread;
	team_id		team;
	port_id		port;
	uint32		flags;
	dev_t		ref_device;
	ino_t		ref_directory;
	char		signature[B_MIME_TYPE_LENGTH];
	char		ref_name[B_FILE_NAME_LENGTH + 1];
} _PACKED;


}	// namespace BPrivate


#endif	// REGISTRAR_DEFS_H


//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AppDefs.h
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	Message codes and the global cursors.
//------------------------------------------------------------------------------

#ifndef _APP_DEFS_H
#define _APP_DEFS_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// Global Cursors --------------------------------------------------------------

// Old-style cursors
extern const unsigned char B_HAND_CURSOR[];
extern const unsigned char B_I_BEAM_CURSOR[];

// New-style cursors
#ifdef  __cplusplus
class BCursor;
extern const BCursor *B_CURSOR_SYSTEM_DEFAULT;
extern const BCursor *B_CURSOR_I_BEAM;
#endif

// System Message Codes --------------------------------------------------------

enum {
	B_ABOUT_REQUESTED			= '_ABR',
	B_WINDOW_ACTIVATED			= '_ACT',
	B_APP_ACTIVATED				= '_ACT',	// Same as B_WINDOW_ACTIVATED
	B_ARGV_RECEIVED 			= '_ARG',
	B_QUIT_REQUESTED 			= '_QRQ',
	B_CLOSE_REQUESTED 			= '_QRQ',	// Obsolete; use B_QUIT_REQUESTED
	B_CANCEL					= '_CNC',
	B_KEY_DOWN 					= '_KYD',
	B_KEY_UP 					= '_KYU',
	B_UNMAPPED_KEY_DOWN 		= '_UKD',
	B_UNMAPPED_KEY_UP 			= '_UKU',
	B_MODIFIERS_CHANGED			= '_MCH',
	B_MINIMIZE					= '_WMN',
	B_MOUSE_DOWN 				= '_MDN',
	B_MOUSE_MOVED 				= '_MMV',
	B_MOUSE_ENTER_EXIT			= '_MEX',
	B_MOUSE_UP 					= '_MUP',
	B_MOUSE_WHEEL_CHANGED		= '_MWC',
	B_OPEN_IN_WORKSPACE			= '_OWS',
	B_PRINTER_CHANGED			= '_PCH',
	B_PULSE 					= '_PUL',
	B_READY_TO_RUN 				= '_RTR',
	B_REFS_RECEIVED 			= '_RRC',
	B_RELEASE_OVERLAY_LOCK		= '_ROV',
	B_ACQUIRE_OVERLAY_LOCK		= '_AOV',
	B_SCREEN_CHANGED 			= '_SCH',
	B_VALUE_CHANGED 			= '_VCH',
	B_VIEW_MOVED 				= '_VMV',
	B_VIEW_RESIZED 				= '_VRS',
	B_WINDOW_MOVED 				= '_WMV',
	B_WINDOW_RESIZED 			= '_WRS',
	B_WORKSPACES_CHANGED		= '_WCG',
	B_WORKSPACE_ACTIVATED		= '_WAC',
	B_ZOOM						= '_WZM',
	_APP_MENU_					= '_AMN',
	_BROWSER_MENUS_				= '_BRM',
	_MENU_EVENT_ 				= '_MEV',
	_PING_						= '_PBL',
	_QUIT_ 						= '_QIT',
	_VOLUME_MOUNTED_ 			= '_NVL',
	_VOLUME_UNMOUNTED_			= '_VRM',
	_MESSAGE_DROPPED_ 			= '_MDP',
	_DISPOSE_DRAG_ 				= '_DPD',
	_MENUS_DONE_				= '_MND',
	_SHOW_DRAG_HANDLES_			= '_SDH',
	_EVENTS_PENDING_ 			= '_EVP',
	_UPDATE_ 					= '_UPD',
	_UPDATE_IF_NEEDED_			= '_UPN',
	_PRINTER_INFO_				= '_PIN',
	_SETUP_PRINTER_				= '_SUP',
	_SELECT_PRINTER_			= '_PSL'
	// Media Kit reserves all reserved codes starting in '_TR'
};


// Other Commands --------------------------------------------------------------

enum {
	B_SET_PROPERTY				= 'PSET',
	B_GET_PROPERTY				= 'PGET',
	B_CREATE_PROPERTY			= 'PCRT',
	B_DELETE_PROPERTY			= 'PDEL',
	B_COUNT_PROPERTIES			= 'PCNT',
	B_EXECUTE_PROPERTY			= 'PEXE',
	B_GET_SUPPORTED_SUITES		= 'SUIT',
	B_UNDO						= 'UNDO',
	B_CUT 						= 'CCUT',
	B_COPY 						= 'COPY',
	B_PASTE 					= 'PSTE',
	B_SELECT_ALL				= 'SALL',
	B_SAVE_REQUESTED 			= 'SAVE',
	B_MESSAGE_NOT_UNDERSTOOD	= 'MNOT',
	B_NO_REPLY 					= 'NONE',
	B_REPLY 					= 'RPLY',
	B_SIMPLE_DATA				= 'DATA',
	B_MIME_DATA					= 'MIME',
	B_ARCHIVED_OBJECT			= 'ARCV',
	B_UPDATE_STATUS_BAR			= 'SBUP',
	B_RESET_STATUS_BAR			= 'SBRS',
	B_NODE_MONITOR				= 'NDMN',
	B_QUERY_UPDATE				= 'QUPD',
	B_ENDORSABLE				= 'ENDO',
	B_COPY_TARGET				= 'DDCP',
	B_MOVE_TARGET				= 'DDMV',
	B_TRASH_TARGET				= 'DDRM',
	B_LINK_TARGET				= 'DDLN',
	B_INPUT_DEVICES_CHANGED		= 'IDCH',
	B_INPUT_METHOD_EVENT		= 'IMEV',
	B_WINDOW_MOVE_TO			= 'WDMT',
	B_WINDOW_MOVE_BY			= 'WDMB',
	B_SILENT_RELAUNCH			= 'AREL',
	B_OBSERVER_NOTICE_CHANGE 	= 'NTCH',
	B_CONTROL_INVOKED			= 'CIVK',
	B_CONTROL_MODIFIED			= 'CMOD'

	// Media Kit reserves all reserved codes starting in 'TRI'
};
//------------------------------------------------------------------------------

#endif	// _APP_DEFS_H

/*
 * $Log $
 *
 * $Id  $
 *
 */


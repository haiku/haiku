/*

	ReadableLong.cpp

	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	

*/

#include "ReadableLong.h"
#include "ctype.h"
#include "stdio.h"

typedef struct {
	const char	*name;
	ulong		value;
} TBeValue;

const TBeValue	bevalue[] = {
	{ "B_ERROR", 0xffff },				
	{ "B_OK", 0 },
	// AppDefs.h
	{ "B_ABOUT_REQUESTED", '_ABR' },				
	{ "B_APP_ACTIVATED", '_ACT' },		// Also B_WINDOW_ACTIVATED	
	{ "B_ARGV_RECEIVED", '_ARG' },				
	{ "B_QUIT_REQUESTED", '_QRQ' },		// Also B_CLOSE_REQUESTED		
	{ "B_CANCEL", '_CNC' },				
	{ "B_KEY_DOWN", '_KYD' },				
	{ "B_KEY_UP", '_KYU' },				
	{ "B_UNMAPPED_KEY_DOWN", '_UKD' },				
	{ "B_UNMAPPED_KEY_UP", '_UKU' },				
	{ "B_MODIFIERS_CHANGED", '_MCH' },				
	{ "B_MINIMIZE", '_WMN' },				
	{ "B_MOUSE_DOWN", '_MDN' },				
	{ "B_MOUSE_MOVED", '_MMV' },				
	{ "B_MOUSE_ENTER_EXIT", '_MEX' },				
	{ "B_MOUSE_UP", '_MUP' },				
	{ "B_OPEN_IN_WORKSPACE", '_OWS' },				
	{ "B_PULSE", '_PUL' },				
	{ "B_READY_TO_RUN", '_RTR' },				
	{ "B_REFS_RECEIVED", '_RRC' },				
	{ "B_SCREEN_CHANGED", '_SCH' },				
	{ "B_VALUE_CHANGED", '_VCH' },				
	{ "B_VIEW_MOVED", '_VMV' },				
	{ "B_VIEW_RESIZED", '_VRS' },				
	{ "B_WINDOW_MOVED", '_WMV' },				
	{ "B_WINDOW_RESIZED", '_WRS' },				
	{ "B_WORKSPACES_CHANGED", '_WCG' },				
	{ "B_WORKSPACE_ACTIVATED", '_WAC' },				
	{ "B_ZOOM", '_WZM' },				
	{ "_APP_MENU_", '_AMN' },				
	{ "_BROWSER_MENUS_", '_BRM' },				
	{ "_MENU_EVENT_", '_MEV' },				
	{ "_PING_", '_PBL' },				
	{ "_QUIT_", '_QIT' },				
	{ "_VOLUME_MOUNTED_", '_NVL' },				
	{ "_VOLUME_UNMOUNTED_", '_VRM' },				
	{ "_MESSAGE_DROPPED_", '_MDP' },				
	{ "_DISPOSE_DRAG_", '_DPD' },				
	{ "_MENUS_DONE_", '_MND' },				
	{ "_SHOW_DRAG_HANDLES_", '_SDH' },				
	{ "_EVENTS_PENDING_", '_EVP' },				
	{ "_UPDATE_", '_UPD' },				
	{ "_UPDATE_IF_NEEDED_", '_UPN' },				
	{ "_PRINTER_INFO_", '_PIN' },				
	{ "_SETUP_PRINTER_", '_SUP' },				
	{ "_SELECT_PRINTER_", '_PSL' },				
	{ "B_SET_PROPERTY", 'PSET' },				
	{ "B_GET_PROPERTY", 'PGET' },				
	{ "B_CREATE_PROPERTY", 'PCRT' },				
	{ "B_DELETE_PROPERTY", 'PDEL' },				
	{ "B_COUNT_PROPERTIES", 'PCNT' },				
	{ "B_EXECUTE_PROPERTY", 'PEXE' },				
	{ "B_GET_SUPPORTED_SUITES", 'SUIT' },				
	{ "B_UNDO", 'UNDO' },				
	{ "B_CUT", 'CCUT' },				
	{ "B_COPY", 'COPY' },				
	{ "B_PASTE", 'PSTE' },				
	{ "B_SELECT_ALL", 'SALL' },				
	{ "B_SAVE_REQUESTED", 'SAVE' },				
	{ "B_MESSAGE_NOT_UNDERSTOOD", 'MNOT' },				
	{ "B_NO_REPLY", 'NONE' },				
	{ "B_REPLY", 'RPLY' },				
	{ "B_SIMPLE_DATA", 'DATA' },				
	{ "B_MIME_DATA", 'MIME' },				
	{ "B_ARCHIVED_OBJECT", 'ARCV' },				
	{ "B_UPDATE_STATUS_BAR", 'SBUP' },				
	{ "B_RESET_STATUS_BAR", 'SBRS' },				
	{ "B_NODE_MONITOR", 'NDMN' },				
	{ "B_QUERY_UPDATE", 'QUPD' },				
	{ "B_ENDORSABLE", 'ENDO' },				
	{ "B_COPY_TARGET", 'DDCP' },				
	{ "B_MOVE_TARGET", 'DDMV' },				
	{ "B_TRASH_TARGET", 'DDRM' },				
	{ "B_LINK_TARGET", 'DDLN' },				
	{ "B_INPUT_DEVICES_CHANGED", 'IDCH' },				
	{ "B_INPUT_METHOD_EVENT", 'IMEV' },
	{ "B_WINDOW_MOVE_TO", 'WDMT' },
	{ "B_WINDOW_MOVE_BY", 'WDMB' },
	{ "B_SILENT_RELAUNCH", 'AREL' },
	// Clipboard.h
	{ "B_CLIPBOARD_CHANGED", 'CLCH' },
	// MediaNode.h
	{ "B_NODE_FAILED_START", 'TRI0' },				
	{ "B_NODE_FAILED_STOP", 'TRI1' },				
	{ "B_NODE_FAILED_SEEK", 'TRI2' },				
	{ "B_NODE_FAILED_SET_RUN_MODE", 'TRI3' },				
	{ "B_NODE_FAILED_TIME_WARP", 'TRI4' },				
	{ "B_NODE_FAILED_PREROLL", 'TRI5' },				
	{ "B_NODE_FAILED_SET_TIME_SOURCE_FOR", 'TRI6' },				
	{ "B_NODE_IN_DISTRESS", 'TRI7' },
	// TypeConstants.h
	{ "B_ANY_TYPE", 'ANYT' },				
	{ "B_ASCII_TYPE", 'TEXT' },				
	{ "B_BOOL_TYPE", 'BOOL' },				
	{ "B_CHAR_TYPE", 'CHAR' },				
	{ "B_COLOR_8_BIT_TYPE", 'CLRB' },				
	{ "B_DOUBLE_TYPE", 'DBLE' },				
	{ "B_FLOAT_TYPE", 'FLOT' },				
	{ "B_GRAYSCALE_8_BIT_TYPE", 'GRYB' },				
	{ "B_INT64_TYPE", 'LLNG' },				
	{ "B_INT32_TYPE", 'LONG' },				
	{ "B_INT16_TYPE", 'SHRT' },				
	{ "B_INT8_TYPE", 'BYTE' },				
	{ "B_MESSAGE_TYPE", 'MSGG' },				
	{ "B_MESSENGER_TYPE", 'MSNG' },				
	{ "B_MIME_TYPE", 'MIME' },				
	{ "B_MONOCHROME_1_BIT_TYPE", 'MNOB' },				
	{ "B_OBJECT_TYPE", 'OPTR' },				
	{ "B_OFF_T_TYPE", 'OFFT' },				
	{ "B_PATTERN_TYPE", 'PATN' },				
	{ "B_POINTER_TYPE", 'PNTR' },				
	{ "B_POINT_TYPE", 'BPNT' },				
	{ "B_RAW_TYPE", 'RAWT' },				
	{ "B_RECT_TYPE", 'RECT' },				
	{ "B_REF_TYPE", 'RREF' },				
	{ "B_RGB_32_BIT_TYPE", 'RGBB' },				
	{ "B_RGB_COLOR_TYPE", 'RGBC' },				
	{ "B_SIZE_T_TYPE", 'SIZT' },	
	{ "B_SSIZE_T_TYPE", 'SSZT' },				
	{ "B_STRING_TYPE", 'CSTR' },				
	{ "B_TIME_TYPE", 'TIME' },				
	{ "B_UINT64_TYPE", 'ULLG' },				
	{ "B_UINT32_TYPE", 'ULNG' },				
	{ "B_UINT16_TYPE", 'USHT' },				
	{ "B_UINT8_TYPE", 'UBYT' },				
	{ "B_MEDIA_PARAMETER_TYPE", 'BMCT' },				
	{ "B_MEDIA_PARAMETER_WEB_TYPE", 'BMCW' },				
	{ "B_MEDIA_PARAMETER_GROUP_TYPE", 'BMCG' },				
	{ "_DEPRECATED_TYPE_1_", 'PATH' },
	// Mime.h
	{ "B_MIME_STRING_TYPE", 'MIMS' },				
	{ "B_META_MIME_CHANGED", 'MMCH' },
	// NetPositive.h
	{ "B_NETPOSITIVE_OPEN_URL", 'NPOP'},	
	{ "B_NETPOSITIVE_BACK", 'NPBK' },				
	{ "B_NETPOSITIVE_FORWARD", 'NPFW' },				
	{ "B_NETPOSITIVE_HOME", 'NPHM' },				
	{ "B_NETPOSITIVE_RELOAD", 'NPRL' },				
	{ "B_NETPOSITIVE_STOP", 'NPST' },
	// ParameterWeb.h
    { "B_MEDIA_PARAMETER_TYPE", 'BMCT' },
    { "B_MEDIA_PARAMETER_WEB_TYPE", 'BMCW' },
    { "B_MEDIA_PARAMETER_GROUP_TYPE", 'BMCG' },
	// PropertyInfo.h
	{ "B_PROPERTY_INFO_TYPE", 'SCTD' },
	// ResourceStrings.h
	{ "RESOURCE_TYPE", 'CSTR' },
	// Roster.h
	{ "B_SOME_APP_LAUNCHED", 'BRAS' },				
	{ "B_SOME_APP_QUIT", 'BRAQ' },				
	{ "B_SOME_APP_ACTIVATED", 'BRAW' },
	// TranslatorFormats.h
	{ "B_TRANSLATOR_BITMAP", 'bits' },				
	{ "B_TRANSLATOR_PICTURE", 'pict' },				
	{ "B_TRANSLATOR_TEXT", 'TEXT' },				
	{ "B_TRANSLATOR_SOUND", 'nois' },				
	{ "B_TRANSLATOR_MIDI", 'midi' },				
	{ "B_TRANSLATOR_MEDIA", 'mhi!' },				
	{ "B_TRANSLATOR_NONE", 'none' },				
	{ "B_GIF_FORMAT", 'GIF ' },				
	{ "B_JPEG_FORMAT", 'JPEG' },				
	{ "B_PNG_FORMAT", 'PNG ' },				
	{ "B_PPM_FORMAT", 'PPM ' },				
	{ "B_TGA_FORMAT", 'TGA ' },				
	{ "B_BMP_FORMAT", 'BMP ' },				
	{ "B_TIFF_FORMAT", 'TIFF' },				
	{ "B_DXF_FORMAT", 'DXF ' },				
	{ "B_EPS_FORMAT", 'EPS ' },				
	{ "B_PICT_FORMAT", 'PICT' },				
	{ "B_WAV_FORMAT", 'WAV ' },				
	{ "B_AIFF_FORMAT", 'AIFF' },				
	{ "B_CD_FORMAT", 'CD  ' },				
	{ "B_AU_FORMAT", 'AU  ' },				
	{ "B_STYLED_TEXT_FORMAT", 'STXT' },
	{ "STYLE_HEADER_MAGIC", 'STYL' },
	// TranslationUtils.h
	{ "B_TRANSLATION_MENU", 'BTMN' },				
	// USB.h
	{ "USB_SUPPORT_DESCRIPTOR", '_USB' },				
	// MediaDefs.h
	{ "B_MEDIA_NODE_CREATED", 'TRIA' },				
	{ "B_MEDIA_NODE_DELETED", 'TRIB' },				
	{ "B_MEDIA_CONNECTION_MADE", 'TRIC' },				
	{ "B_MEDIA_CONNECTION_BROKEN", 'TRID' },				
	{ "B_MEDIA_BUFFER_CREATED", 'TRIE' },				
	{ "B_MEDIA_BUFFER_DELETED", 'TRIF' },				
	{ "B_MEDIA_TRANSPORT_STATE", 'TRIG' },				
	{ "B_MEDIA_PARAMETER_CHANGED", 'TRIH' },				
	{ "B_MEDIA_FORMAT_CHANGED", 'TRII' },				
	{ "B_MEDIA_WEB_CHANGED", 'TRIJ' },
	{ "B_MEDIA_DEFAULT_CHANGED", 'TRIK' },
	{ NULL, 0 }
};

const char* long_to_string(ulong ulg)
{
	static char string[10];
//	string[0]=(ulg >> 24) & 0xff;
//	string[1]=(ulg >> 16) & 0xff;
//	string[2]=(ulg >> 8) & 0xff;
//	string[3]=ulg & 0xff;
//	string[4]=0;
	string[6]=0;
	string[0]='\'';
	string[1]=(ulg >> 24) & 0xff;
	string[2]=(ulg >> 16) & 0xff;
	string[3]=(ulg >> 8) & 0xff;
	string[4]=ulg & 0xff;
	string[5]='\'';
	if (!(isprint(string[1]) && isprint(string[2]) && isprint(string[3]) && isprint(string[4])))
		sprintf(string, "0x%.8lX", ulg);
	return string;
}

ulong string_to_long(void* st)
{
	char*	string=(char*) st;
	ulong	ulg;
	ulg=string[3]+(string[2] << 8)+(string[1] << 16)+(string[0] << 24);
	return ulg;
}

const char* long_to_be_string(ulong type)
{
	int k=0;
	while (bevalue[k].name!=NULL) {
		if (bevalue[k].value==type)
			return bevalue[k].name;
		k++;
	}
	return long_to_string(type);
}

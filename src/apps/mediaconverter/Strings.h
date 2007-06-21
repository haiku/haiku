// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef STRINGS_H
#define STRINGS_H


#include <BeBuild.h>


#if B_BEOS_VERSION >= 0x520
	typedef const char Char;
	#if B_BEOS_VERSION>=0x530 // 0x530 RC3
		#include <locale/Locale.h>
	#else
		#include <Locale.h>
	#endif
#else 
	#define _T(str) str 
	typedef char Char;
#endif

#define	VERSION						"1.3.0"

#define SOURCE_BOX_LABEL			_T("Source files")
#define INFO_BOX_LABEL				_T("File details")
#define OUTPUT_BOX_LABEL			_T("Output format")
#define FORMAT_LABEL 				_T("File format")
#define VIDEO_LABEL		 			_T("Video encoding")
#define AUDIO_LABEL 				_T("Audio encoding")
//
#define OUTPUT_FOLDER_LABEL			_T("Output Folder")
#define PREVIEW_BUTTON_LABEL		_T("Preview")
#define START_LABEL 				_T("Start mSec ")
#define END_LABEL 					_T("End   mSec ")
//
#define FORMAT_MENU_LABEL			_T("Format")
#define VIDEO_MENU_LABEL 			_T("Video")
#define AUDIO_MENU_LABEL 			_T("Audio")
#define DURATION_LABEL				_T("Duration:")
#define VIDEO_INFO_LABEL			_T("Video:")
#define AUDIO_INFO_LABEL			_T("Audio:")
#define CONVERT_LABEL				_T("Convert")
#define CANCEL_LABEL				_T("Cancel")
#define VIDEO_QUALITY_LABEL			_T("Video quality %3d%%")
#define AUDIO_QUALITY_LABEL			_T("Audio quality %3d%%")
#define SLIDER_LOW_LABEL			_T("Low")
#define SLIDER_HIGH_LABEL			_T("High")
#define NONE_LABEL					_T("None available")
#define DROP_MEDIA_FILE_LABEL		_T("Drop media files onto this window")
#define CANCELLING_LABEL			_T("Cancelling")
#define ABOUT_TITLE_LABEL			_T("About")
#define OK_LABEL					_T("OK")
#define CONV_COMPLETE_LABEL			_T("Conversion completed")
#define CONV_CANCEL_LABEL			_T("Conversion cancelled")
#define SELECT_DIR_LABEL			_T("Select This Directory")
#define SELECT_LABEL				_T("Select")
#define SAVE_DIR_LABEL				_T("MediaConverter+:SaveDirectory")
#define NO_FILE_LABEL				_T("No file selected")
//
#define FILE_MENU_LABEL				_T("File")
#define OPEN_MENU_LABEL				_T("Open")
#define ABOUT_MENU_LABEL			_T("About")
#define QUIT_MENU_LABEL				_T("Quit")

#define WRITE_AUDIO_STRING			_T("Writing audio track:")
#define WRITE_VIDEO_STRING			_T("Writing video track:")
#define COMPLETE_STRING				_T("complete")
#define OUTPUT_FILE_STRING1			_T("Output file")
#define OUTPUT_FILE_STRING2			_T("created")
#define AUDIO_SUPPORT_STRING		_T("Audio quality not supported")
#define VIDEO_SUPPORT_STRING		_T("Video quality not supported")
#define VIDEO_PARAMFORM_STRING		_T("Video using parameters form settings")
#define CONTINUE_STRING				_T("Continue")
#define FILES						_T("files")
#define FILE						_T("file")
#define NOTRECOGNIZE				_T(" were not recognized as supported media files:")
//
#define LAUNCH_ERROR				_T("Error launching: ")
#define OUTPUT_FILE_STRING3			_T("Error creating")
#define CONVERT_ERROR_STRING		_T("Error converting")
#define ERROR_WRITE_VIDEO_STRING	_T("Error writing video frame ")
#define ERROR_WRITE_AUDIO_STRING	_T("Error writing audio frame ")
#define ERROR_READ_VIDEO_STRING		_T("Error read video frame ") 
#define ERROR_READ_AUDIO_STRING		_T("Error read audio frame ")
#define ERROR_LOAD_STRING			_T("Error loading file")

#define ENCODER_PARAMETERS			_T("Encoder Parameters")


#endif // STRINGS_H

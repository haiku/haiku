/* PoorManLoggingView.cpp
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */
 
#include <Box.h>
#include <Catalog.h>
#include <Locale.h>

#include "constants.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"
#include "PoorManLoggingView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"


PoorManLoggingView::PoorManLoggingView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	PoorManWindow	*	win;
	win = ((PoorManApplication *)be_app)->GetPoorManWindow();
	
	SetViewColor(BACKGROUND_COLOR);
	
	// Console Logging BBox
	BRect consoleLoggingRect;
	consoleLoggingRect = rect;
	consoleLoggingRect.top -= 5.0;
	consoleLoggingRect.left -= 5.0;
	consoleLoggingRect.right -= 7.0;
	consoleLoggingRect.bottom -= 118.0;
	
	BBox * consoleLogging = new BBox(consoleLoggingRect, 
		B_TRANSLATE("Console Logging"));
	consoleLogging->SetLabel(STR_BBX_CONSOLE_LOGGING);
	AddChild(consoleLogging);

	
	// File Logging BBox
	BRect fileLoggingRect;
	fileLoggingRect = consoleLoggingRect;
	fileLoggingRect.top = consoleLoggingRect.bottom + 10.0;
	fileLoggingRect.bottom = fileLoggingRect.top + 100.0;

	BBox * fileLogging = new BBox(fileLoggingRect, 
		B_TRANSLATE("File Logging"));
	fileLogging->SetLabel(STR_BBX_FILE_LOGGING);
	AddChild(fileLogging);
	
	float left = 10.0;
	float top = 20.0;
	float box_size = 13.0;
	BRect tempRect(left, top, consoleLoggingRect.Width() - 5.0, top + box_size);
	
	// Console Logging
	logConsole = new BCheckBox(tempRect, B_TRANSLATE("Log To Console"),
		STR_CBX_LOG_CONSOLE, new BMessage(MSG_PREF_LOG_CBX_CONSOLE));
	// set the checkbox to the value the program has
	SetLogConsoleValue(win->LogConsoleFlag());
	consoleLogging->AddChild(logConsole);
	
	// File Logging
	logFile = new BCheckBox(tempRect, B_TRANSLATE("Log To File"),
		STR_CBX_LOG_FILE, new BMessage(MSG_PREF_LOG_CBX_FILE));
	// set the checkbox to the value the program has
	SetLogFileValue(win->LogFileFlag());
	fileLogging->AddChild(logFile);
	
	// File Name
	tempRect.top = tempRect.bottom + 10.0;
	tempRect.bottom = tempRect.top + box_size;
	tempRect.right -= 5.0;
	
	logFileName = new BTextControl(tempRect, B_TRANSLATE("File Name"),
		STR_TXT_LOG_FILE_NAME, NULL, NULL);
	logFileName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	logFileName->SetDivider(fileLogging->StringWidth(STR_TXT_LOG_FILE_NAME) + 8.0f);
	SetLogFileName(win->LogPath());
	fileLogging->AddChild(logFileName);
	
	// Create Log File
	BRect createLogFileRect;
	createLogFileRect.top = tempRect.bottom + 13.0;
	createLogFileRect.right = tempRect.right + 2.0;
	createLogFileRect.left = createLogFileRect.right 
		- fileLogging->StringWidth(B_TRANSLATE("Create Log File")) - 24.0;
	createLogFileRect.bottom = createLogFileRect.top + 19.0;
	
	createLogFile = new BButton(createLogFileRect, B_TRANSLATE("Create Log File"), 
		STR_BTN_CREATE_LOG_FILE, new BMessage(MSG_PREF_LOG_BTN_CREATE_FILE));
	fileLogging->AddChild(createLogFile);

}

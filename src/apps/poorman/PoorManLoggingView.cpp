/* PoorManLoggingView.cpp
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */
 
#include <Box.h>

#include "constants.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"
#include "PoorManLoggingView.h"

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
	
	BBox * consoleLogging = new BBox(consoleLoggingRect, "Console Logging");
	consoleLogging->SetLabel(STR_BBX_CONSOLE_LOGGING);
	AddChild(consoleLogging);

	
	// File Logging BBox
	BRect fileLoggingRect;
	fileLoggingRect = consoleLoggingRect;
	fileLoggingRect.top = consoleLoggingRect.bottom + 10.0;
	fileLoggingRect.bottom = fileLoggingRect.top + 100.0;

	BBox * fileLogging = new BBox(fileLoggingRect, "File Logging");
	fileLogging->SetLabel(STR_BBX_FILE_LOGGING);
	AddChild(fileLogging);
	
	float left = 10.0;
	float top = 20.0;
	float box_size = 13.0;
	BRect tempRect(left, top, consoleLoggingRect.Width() - 5.0, top + box_size);
	
	// Console Logging
	logConsole = new BCheckBox(tempRect, "Log To Console", STR_CBX_LOG_CONSOLE, new BMessage(MSG_PREF_LOG_CBX_CONSOLE));
	// set the checkbox to the value the program has
	SetLogConsoleValue(win->LogConsoleFlag());
	consoleLogging->AddChild(logConsole);
	
	// File Logging
	logFile = new BCheckBox(tempRect, "Log To File", STR_CBX_LOG_FILE, new BMessage(MSG_PREF_LOG_CBX_FILE));
	// set the checkbox to the value the program has
	SetLogFileValue(win->LogFileFlag());
	fileLogging->AddChild(logFile);
	
	// File Name
	tempRect.top = tempRect.bottom + 10.0;
	tempRect.bottom = tempRect.top + box_size;
	tempRect.right -= 5.0;
	
	logFileName = new BTextControl(tempRect, "File Name", STR_TXT_LOG_FILE_NAME, NULL, NULL);
	logFileName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	logFileName->SetDivider(73.0);
	SetLogFileName(win->LogPath());
	fileLogging->AddChild(logFileName);
	
	// Create Log File
	BRect createLogFileRect;
	createLogFileRect.top = tempRect.bottom + 13.0;
	createLogFileRect.right = tempRect.right + 2.0;
	createLogFileRect.left = createLogFileRect.right - 87.0;
	createLogFileRect.bottom = createLogFileRect.top + 19.0;
	
	createLogFile = new BButton(createLogFileRect, "Create Log File", STR_BTN_CREATE_LOG_FILE, new BMessage(MSG_PREF_LOG_BTN_CREATE_FILE));
	fileLogging->AddChild(createLogFile);

}

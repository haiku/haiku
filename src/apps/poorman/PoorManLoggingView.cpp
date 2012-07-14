/* PoorManLoggingView.cpp
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */
 
#include <Box.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>

#include "constants.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"
#include "PoorManLoggingView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"


PoorManLoggingView::PoorManLoggingView(const char* name)
	:
	BView(name, B_WILL_DRAW, NULL)
{
	PoorManWindow* win;
	win = ((PoorManApplication*)be_app)->GetPoorManWindow();
	
	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BBox* consoleLogging = new BBox(B_TRANSLATE("Console Logging"));
	consoleLogging->SetLabel(STR_BBX_CONSOLE_LOGGING);
	
	// File Logging BBox
	BBox* fileLogging = new BBox(B_TRANSLATE("File Logging"));
	fileLogging->SetLabel(STR_BBX_FILE_LOGGING);
		
	// Console Logging
	fLogConsole = new BCheckBox(B_TRANSLATE("Log To Console"),
		STR_CBX_LOG_CONSOLE, new BMessage(MSG_PREF_LOG_CBX_CONSOLE));
	// set the checkbox to the value the program has
	SetLogConsoleValue(win->LogConsoleFlag());
	
	// File Logging
	fLogFile = new BCheckBox(B_TRANSLATE("Log To File"), STR_CBX_LOG_FILE,
		new BMessage(MSG_PREF_LOG_CBX_FILE));
	// set the checkbox to the value the program has
	SetLogFileValue(win->LogFileFlag());
	
	// File Name
	fLogFileName = new BTextControl(B_TRANSLATE("File Name"),
		STR_TXT_LOG_FILE_NAME, NULL, NULL);
	SetLogFileName(win->LogPath());
	
	// Create Log File
	fCreateLogFile = new BButton(B_TRANSLATE("Create Log File"),
		STR_BTN_CREATE_LOG_FILE, new BMessage(MSG_PREF_LOG_BTN_CREATE_FILE));

	consoleLogging->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(fLogConsole)
			.AddGlue())
		.SetInsets(5, 5, 5, 5));
		
	fileLogging->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(fLogFile)
			.AddGlue())
		.Add(fLogFileName)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(fCreateLogFile))
		.SetInsets(5, 5, 5, 5));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(consoleLogging)
		.Add(fileLogging)
		.SetInsets(5, 5, 5, 5));
}

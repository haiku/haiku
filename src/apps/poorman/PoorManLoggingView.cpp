/* PoorManLoggingView.cpp
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */
 
#include <Box.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
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

	BGroupLayout* consoleLoggingLayout = new BGroupLayout(B_VERTICAL, 0);
	consoleLogging->SetLayout(consoleLoggingLayout);

	BGroupLayout* fileLoggingLayout = new BGroupLayout(B_VERTICAL,
		B_USE_SMALL_SPACING);
	fileLogging->SetLayout(fileLoggingLayout);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_ITEM_INSETS)
		.AddGroup(consoleLoggingLayout)
			.SetInsets(B_USE_ITEM_INSETS)
			.AddGroup(B_HORIZONTAL)
				.SetInsets(0, B_USE_ITEM_INSETS, 0, 0)
				.Add(fLogConsole)
				.AddGlue()
				.End()
			.End()
		.AddGroup(fileLoggingLayout)
			.SetInsets(B_USE_ITEM_INSETS)
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.SetInsets(0, B_USE_ITEM_INSETS, 0, 0)
				.Add(fLogFile, 0, 0)
				.AddTextControl(fLogFileName, 0, 1, B_ALIGN_LEFT, 1, 2)
				.Add(fCreateLogFile, 2, 2);
}

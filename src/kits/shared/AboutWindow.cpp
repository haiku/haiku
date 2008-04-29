/*
 * Copyright 2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */

#include <AboutWindow.h>
#include <Alert.h>
#include <Font.h>
#include <String.h>
#include <TextView.h>

#include <stdarg.h>
#include <time.h>


BAboutWindow::BAboutWindow(char *appName, int32 firstCopyrightYear,
	const char **authors, char *extraInfo)
{
	fAppName = new BString(appName);
	fText = new BString();

	// Get current year
	time_t tp;
	time(&tp);
	char currentYear[5];
	strftime(currentYear, 5, "%Y", localtime(&tp));

	// Build the text to display
	BString text(appName);
	text << "\n\nCopyright " B_UTF8_COPYRIGHT " ";
	text << firstCopyrightYear << "-" << currentYear << " Haiku, Inc.\n\n";
	text << "Written by:\n";
	for (int32 i = 0; authors[i]; i++) {
		text << "    " << authors[i] << "\n";
	}
	
	// The extra information is optional
	if (extraInfo != NULL) {
		text << "\n" << extraInfo << "\n";
	}

	fText->Adopt(text);
}


BAboutWindow::~BAboutWindow()
{
	delete fText;
}


void
BAboutWindow::Show()
{
	BAlert *alert = new BAlert("About" B_UTF8_ELLIPSIS, fText->String(), "Close");
	BTextView *view = alert->TextView();
	BFont font;
	view->SetStylable(true);
	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.7);
	view->SetFontAndColor(0, fAppName->Length(), &font);
	alert->Go();
}


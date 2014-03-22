/*
 * Copyright 2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zhuowei Zhang
 */
#include "ConsoleWindow.h"

#include <Catalog.h>
#include <Message.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <TextControl.h>
#include <ListView.h>
#include <ScrollView.h>

#include "BrowserWindow.h"
#include "BrowserApp.h"
#include "WebViewConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Console Window"


enum {
	EVAL_CONSOLE_WINDOW_COMMAND = 'ecwc',
	CLEAR_CONSOLE_MESSAGES = 'ccms'
};


ConsoleWindow::ConsoleWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Script console"), B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS
			| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0.0));

	fMessagesListView = new BListView("Console messages");
	fClearMessagesButton = new BButton(B_TRANSLATE("Clear"),
		new BMessage(CLEAR_CONSOLE_MESSAGES));
	
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0.0)
		.Add(new BScrollView("Console messages scroll", 
			fMessagesListView, 0, true, true))
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fClearMessagesButton)
			.SetInsets(0, 5, 0, 5)
		)
		.SetInsets(5, 5, 5, 5)
	);
	if (!frame.IsValid())
		CenterOnScreen();
}


void
ConsoleWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ADD_CONSOLE_MESSAGE:
		{
			BString source = message->FindString("source");
			int32 lineNumber = message->FindInt32("line");
			int32 columnNumber = message->FindInt32("column");
			BString text = message->FindString("string");
			BString finalText;
			finalText.SetToFormat("%s:%" B_PRIi32 ":%" B_PRIi32 ": %s\n",
				source.String(), lineNumber, columnNumber, text.String());
			fMessagesListView->AddItem(new BStringItem(finalText.String()));
			break;
		}
		case CLEAR_CONSOLE_MESSAGES:
		{
			int count = fMessagesListView->CountItems();
			for (int i = count - 1; i >= 0; i--) {
				delete fMessagesListView->RemoveItem(i);
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ConsoleWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();
	return false;
}

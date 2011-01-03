/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */


#include "EditServerListWindow.h"
#include "NetworkTime.h"

#include <Application.h>
#include <TextView.h>
#include <ScrollView.h>
#include <Button.h>
#include <Screen.h>

#include <string.h>
#include <stdlib.h>


static const uint32 kMsgServersEdited = 'sedt';

static const char *kServerDelimiters = ", \n\t";


struct tokens {
	char *buffer;
	char *next;
	const char *delimiters;
};


static void
put_tokens(tokens &tokens)
{
	free(tokens.buffer);
	tokens.buffer = NULL;
}


static status_t
prepare_tokens(tokens &tokens, const char *text, const char *delimiters)
{
	tokens.buffer = strdup(text);
	if (tokens.buffer == NULL)
		return B_NO_MEMORY;

	tokens.delimiters = delimiters;
	tokens.next = tokens.buffer;
	return B_OK;
}


static const char *
next_token(tokens &tokens)
{
	char *token = strtok(tokens.next, tokens.delimiters);
	if (tokens.next != NULL)
		tokens.next = NULL;
	if (token == NULL)
		put_tokens(tokens);

	return token;
}


//	#pragma mark -


EditServerListWindow::EditServerListWindow(BRect position, const BMessage &settings)
	: BWindow(position, "Edit Server List", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fSettings(settings)
{
	BRect rect = Bounds();
	BView *view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect = view->Bounds().InsetByCopy(5, 5);

	BButton *button = new BButton(rect, "defaults", "Reset To Defaults",
		new BMessage(kMsgResetServerList), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	float width, height;
	button->GetPreferredSize(&width, &height);
	button->ResizeTo(rect.Width(), height);
	button->MoveBy(0, rect.Height() - height);
	button->SetTarget(be_app);
	view->AddChild(button);

	rect.bottom -= 5 + height;
	rect.InsetBy(2, 2);
	rect.right -= B_V_SCROLL_BAR_WIDTH;

	fServerView = new BTextView(rect, "list",
		rect.OffsetToCopy(B_ORIGIN).InsetByCopy(3, 3), B_FOLLOW_ALL);
	fServerView->SetStylable(true);

	BScrollView *scrollView = new BScrollView("scroller", fServerView, B_FOLLOW_ALL, B_WILL_DRAW, false, true);
	view->AddChild(scrollView);

	UpdateServerView(settings);

	SetSizeLimits(200, 16384, 200, 16384);

	if (Frame().LeftTop() == B_ORIGIN) {
		// center on screen
		BScreen screen;
		MoveTo((screen.Frame().Width() - Bounds().Width()) / 2,
			(screen.Frame().Height() - Bounds().Height()) / 2);
	}
}


EditServerListWindow::~EditServerListWindow()
{
}


void
EditServerListWindow::UpdateDefaultServerView(const BMessage &message)
{
	int32 defaultServer;
	if (message.FindInt32("default server", &defaultServer) != B_OK)
		return;

	BFont font(be_plain_font);
	fServerView->SetFontAndColor(0, fServerView->TextLength(),
		&font, B_FONT_FAMILY_AND_STYLE);

	struct tokens tokens;
	const char *server;
	if (fSettings.FindString("server", defaultServer, &server) == B_OK
		&& prepare_tokens(tokens, fServerView->Text(), kServerDelimiters) == B_OK) {
		const char *token;
		while ((token = next_token(tokens)) != NULL) {
			if (strcasecmp(token, server))
				continue;

			int32 start = int32(token - tokens.buffer);

			BFont font;
			font.SetFace(B_BOLD_FACE);
			fServerView->SetFontAndColor(start, start + strlen(server),
				&font, B_FONT_FAMILY_AND_STYLE);
			
			break;
		}
		put_tokens(tokens);
	}
}


void
EditServerListWindow::UpdateServerView(const BMessage &message)
{
	if (message.HasString("server") || message.HasBool("reset servers"))
		fServerView->SetText("");

	int32 defaultServer;
	if (message.FindInt32("default server", &defaultServer) != B_OK)
		defaultServer = 0;

	const char *server;
	int32 index = 0;
	for (; message.FindString("server", index, &server) == B_OK; index++) {
		int32 start, end;
		fServerView->GetSelection(&start, &end);

		fServerView->Insert(server);
		fServerView->Insert("\n");
	}

	UpdateDefaultServerView(message);
}


void
EditServerListWindow::UpdateServerList()
{
	// get old default server

	int32 defaultServer;
	if (fSettings.FindInt32("default server", &defaultServer) != B_OK)
		defaultServer = 0;
	const char *server;
	if (fSettings.FindString("server", defaultServer, &server) != B_OK)
		server = NULL;

	// take over server list

	struct tokens tokens;
	if (prepare_tokens(tokens, fServerView->Text(), kServerDelimiters) == B_OK) {
		BMessage servers(kMsgUpdateSettings);

		int32 count = 0, newDefaultServer = -1;

		const char *token;
		while ((token = next_token(tokens)) != NULL) {
			servers.AddString("server", token);
			if (!strcasecmp(token, server))
				newDefaultServer = count;

			count++;
		}

		if (count != 0) {
			if (newDefaultServer == -1)
				newDefaultServer = 0;

			servers.AddInt32("default server", newDefaultServer);
			be_app->PostMessage(&servers);
		} else
			be_app->PostMessage(kMsgResetServerList);
	}
}


bool
EditServerListWindow::QuitRequested()
{
	be_app->PostMessage(kMsgEditServerListWindowClosed);

	BMessage update(kMsgUpdateSettings);
	update.AddRect("edit servers frame", Frame());
	be_app->PostMessage(&update);

	UpdateServerList();
	return true;
}


void
EditServerListWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgServersEdited:
			UpdateServerList();
			break;

		case kMsgSettingsUpdated:
			if (message->HasString("server"))
				UpdateServerView(*message);
			else
				UpdateDefaultServerView(*message);
			break;
	}
}


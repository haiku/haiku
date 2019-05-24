/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#include "Status.h"

#include <Button.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <TextControl.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MailApp.h"
#include "MailWindow.h"
#include "Messages.h"


#define STATUS_TEXT			"Status:"
#define STATUS_FIELD_H		 10
#define STATUS_FIELD_V		  8
#define STATUS_FIELD_WIDTH	(STATUS_WIDTH - STATUS_FIELD_H)
#define STATUS_FIELD_HEIGHT	 16

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

#define S_OK_BUTTON_X1		(STATUS_WIDTH - BUTTON_WIDTH - 6)
#define S_OK_BUTTON_Y1		(STATUS_HEIGHT - (BUTTON_HEIGHT + 10))
#define S_OK_BUTTON_X2		(S_OK_BUTTON_X1 + BUTTON_WIDTH)
#define S_OK_BUTTON_Y2		(S_OK_BUTTON_Y1 + BUTTON_HEIGHT)
#define S_OK_BUTTON_TEXT	"OK"

#define S_CANCEL_BUTTON_X1	(S_OK_BUTTON_X1 - (BUTTON_WIDTH + 10))
#define S_CANCEL_BUTTON_Y1	S_OK_BUTTON_Y1
#define S_CANCEL_BUTTON_X2	(S_CANCEL_BUTTON_X1 + BUTTON_WIDTH)
#define S_CANCEL_BUTTON_Y2	S_OK_BUTTON_Y2
#define S_CANCEL_BUTTON_TEXT	"Cancel"

enum status_messages {
	STATUS = 128,
	OK,
	CANCEL
};


TStatusWindow::TStatusWindow(BRect rect, BMessenger target, const char* status)
	: BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE),
	fTarget(target)
{
	BView* view = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	AddChild(view);

	BRect r(STATUS_FIELD_H, STATUS_FIELD_V,
		STATUS_FIELD_WIDTH, STATUS_FIELD_V + STATUS_FIELD_HEIGHT);

	fStatus = new BTextControl(r, "", STATUS_TEXT, status, new BMessage(STATUS));
	view->AddChild(fStatus);

	fStatus->SetDivider(fStatus->StringWidth(STATUS_TEXT) + 6);
	fStatus->BTextControl::MakeFocus(true);

	r.Set(S_OK_BUTTON_X1, S_OK_BUTTON_Y1, S_OK_BUTTON_X2, S_OK_BUTTON_Y2);
	BButton *button = new BButton(r, "", S_OK_BUTTON_TEXT, new BMessage(OK));
	view->AddChild(button);
	button->MakeDefault(true);

	r.Set(S_CANCEL_BUTTON_X1, S_CANCEL_BUTTON_Y1, S_CANCEL_BUTTON_X2, S_CANCEL_BUTTON_Y2);
	button = new BButton(r, "", S_CANCEL_BUTTON_TEXT, new BMessage(CANCEL));
	view->AddChild(button);

	Show();
}


TStatusWindow::~TStatusWindow()
{
}


void
TStatusWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case STATUS:
			break;

		case OK:
		{
			if (!_Exists(fStatus->Text())) {
				int32 index = 0;
				uint32 loop;
				status_t result;
				BDirectory dir;
				BEntry entry;
				BFile file;
				BNodeInfo* node;
				BPath path;

				find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
				dir.SetTo(path.Path());
				if (dir.FindEntry("Mail", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("Mail", &dir);
				if (dir.InitCheck() != B_NO_ERROR)
					goto err_exit;
				if (dir.FindEntry("status", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("status", &dir);
				if (dir.InitCheck() == B_NO_ERROR) {
					char name[B_FILE_NAME_LENGTH];
					char newName[B_FILE_NAME_LENGTH];

					sprintf(name, "%s", fStatus->Text());
					if (strlen(name) > B_FILE_NAME_LENGTH - 10)
						name[B_FILE_NAME_LENGTH - 10] = 0;
					for (loop = 0; loop < strlen(name); loop++) {
						if (name[loop] == '/')
							name[loop] = '\\';
					}
					strcpy(newName, name);
					while (1) {
						if ((result = dir.CreateFile(newName, &file, true)) == B_NO_ERROR)
							break;
						if (result != EEXIST)
							goto err_exit;
						snprintf(newName, B_FILE_NAME_LENGTH, "%s_%" B_PRId32,
							name, index++);
					}
					dir.FindEntry(newName, &entry);
					node = new BNodeInfo(&file);
					node->SetType("text/plain");
					delete node;
					file.Write(fStatus->Text(), strlen(fStatus->Text()) + 1);
					file.SetSize(file.Position());
					file.WriteAttr(INDEX_STATUS, B_STRING_TYPE, 0, fStatus->Text(),
						strlen(fStatus->Text()) + 1);
				}
			}
		err_exit:
			{
				BMessage close(M_CLOSE_CUSTOM);
				close.AddString("status", fStatus->Text());
				fTarget.SendMessage(&close);
				// will fall through
			}
		}
		case CANCEL:
			Quit();
			break;
	}
}


bool
TStatusWindow::_Exists(const char* status)
{
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);

	BQuery query;
	query.SetVolume(&volume);
	query.PushAttr(INDEX_STATUS);
	query.PushString(status);
	query.PushOp(B_EQ);
	query.Fetch();

	BEntry entry;
	if (query.GetNextEntry(&entry) == B_NO_ERROR)
		return true;

	return false;
}


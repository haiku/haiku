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

//--------------------------------------------------------------------
//	
//	Status.cpp
//
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <StorageKit.h>

#include "Mail.h"
#include "Status.h"


//====================================================================

TStatusWindow::TStatusWindow(BRect rect, BWindow *window, const char *status)
	: BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	BRect r(0, 0, STATUS_WIDTH, STATUS_HEIGHT);

	r.InsetBy(-1, -1);
	fView = new TStatusView(r, window, status);
	Lock();
	AddChild(fView);
	Unlock();
	Show();
}


//====================================================================
//	#pragma mark -


TStatusView::TStatusView(BRect rect, BWindow *window, const char *status)
	: BBox(rect, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fWindow = window;
	fString = status;

	SetViewColor(VIEW_COLOR, VIEW_COLOR, VIEW_COLOR);

	BFont font = *be_plain_font;
	font.SetSize(FONT_SIZE);
	SetFont(&font);
}


void
TStatusView::AttachedToWindow()
{
	BRect r(STATUS_FIELD_H, STATUS_FIELD_V,
		STATUS_FIELD_WIDTH, STATUS_FIELD_V + STATUS_FIELD_HEIGHT);

	fStatus = new BTextControl(r, "", STATUS_TEXT, fString, new BMessage(STATUS));
	AddChild(fStatus);

	BFont font = *be_plain_font;
	font.SetSize(FONT_SIZE);
	fStatus->SetFont(&font);
	fStatus->SetDivider(StringWidth(STATUS_TEXT) + 6);
	fStatus->BTextControl::MakeFocus(true);

	r.Set(S_OK_BUTTON_X1, S_OK_BUTTON_Y1, S_OK_BUTTON_X2, S_OK_BUTTON_Y2);
	BButton *button = new BButton(r, "", S_OK_BUTTON_TEXT, new BMessage(OK));
	AddChild(button);
	button->SetTarget(this);
	button->MakeDefault(true);

	r.Set(S_CANCEL_BUTTON_X1, S_CANCEL_BUTTON_Y1, S_CANCEL_BUTTON_X2, S_CANCEL_BUTTON_Y2);
	button = new BButton(r, "", S_CANCEL_BUTTON_TEXT, new BMessage(CANCEL));
	AddChild(button);
	button->SetTarget(this);
}


void
TStatusView::MessageReceived(BMessage *msg)
{
	char			name[B_FILE_NAME_LENGTH];
	char			new_name[B_FILE_NAME_LENGTH];
	int32			index = 0;
	uint32			loop;
	status_t		result;
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	BNodeInfo		*node;
	BPath			path;

	switch (msg->what)
	{
		case STATUS:
			break;

		case OK:
			if (!Exists(fStatus->Text())) {
				find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
				dir.SetTo(path.Path());
				if (dir.FindEntry("bemail", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("bemail", &dir);
				if (dir.InitCheck() != B_NO_ERROR)
					goto err_exit;
				if (dir.FindEntry("status", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("status", &dir);
				if (dir.InitCheck() == B_NO_ERROR) {
					sprintf(name, "%s", fStatus->Text());
					if (strlen(name) > B_FILE_NAME_LENGTH - 10)
						name[B_FILE_NAME_LENGTH - 10] = 0;
					for (loop = 0; loop < strlen(name); loop++) {
						if (name[loop] == '/')
							name[loop] = '\\';
					}
					strcpy(new_name, name);
					while (1) {
						if ((result = dir.CreateFile(new_name, &file, true)) == B_NO_ERROR)
							break;
						if (result != EEXIST)
							goto err_exit;
						sprintf(new_name, "%s_%ld", name, index++);
					}
					dir.FindEntry(new_name, &entry);
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
				BMessage closeCstmMsg(M_CLOSE_CUSTOM);
				closeCstmMsg.AddString("status", fStatus->Text());
				fWindow->PostMessage(&closeCstmMsg);
				// will fall through
			}
		case CANCEL:
			Window()->Quit();
			break;
	}
}


bool
TStatusView::Exists(const char *status)
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


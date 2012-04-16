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
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or
registered trademarks of Be Incorporated in the United States and other
countries. Other brand product names are registered trademarks or trademarks
of their respective holders. All rights reserved.
*/

//--------------------------------------------------------------------
//
//	Enclosures.cpp
//	The enclosures list view (TListView), the list items (TListItem),
//	and the view containing the list
//	and handling the messages (TEnclosuresView).
//--------------------------------------------------------------------

#include "Enclosures.h"

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Locale.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>

#include <MailAttachment.h>
#include <MailMessage.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MailApp.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"


#define B_TRANSLATION_CONTEXT "Mail"


static const float kPlainFontSizeScale = 0.9;

static status_t
GetTrackerIcon(BMimeType &type, BBitmap *icon, icon_size iconSize)
{
	// set some icon size related variables
	status_t error = B_OK;
	BRect bounds;
	switch (iconSize) {
		case B_MINI_ICON:
			bounds.Set(0, 0, 15, 15);
			break;
		case B_LARGE_ICON:
			bounds.Set(0, 0, 31, 31);
			break;
		default:
			error = B_BAD_VALUE;
			break;
	}
	// check parameters and initialization
	if (error == B_OK
		&& (!icon || icon->InitCheck() != B_OK || icon->Bounds() != bounds))
		return B_BAD_VALUE;

	bool success = false;

	// Ask the MIME database for the preferred application for the file type
	// and whether this application has a special icon for the type.
	char signature[B_MIME_TYPE_LENGTH];
	if (type.GetPreferredApp(signature) == B_OK) {
		BMimeType type(signature);
		success = (type.GetIconForType(type.Type(), icon, iconSize) == B_OK);
	}

	// Ask the MIME database whether there is an icon for the node's file type.
	if (error == B_OK && !success)
		success = (type.GetIcon(icon, iconSize) == B_OK);

	// Ask the MIME database for the super type and start all over
	if (error == B_OK && !success) {
		BMimeType super;
		if (type.GetSupertype(&super) == B_OK)
			return GetTrackerIcon(super, icon, iconSize);
	}

	// Return the icon for "application/octet-stream" from the MIME database.
	if (error == B_OK && !success) {
		// get the "application/octet-stream" icon
		BMimeType type("application/octet-stream");
		error = type.GetIcon(icon, iconSize);
	}

	return error;
}


static void
recursive_attachment_search(TEnclosuresView* us, BMailContainer* mail,
	BMailComponent *body)
{
	if (mail == NULL)
		return;

	for (int32 i = 0; i < mail->CountComponents(); i++) {
		BMailComponent *component = mail->GetComponent(i);
		if (component == body)
			continue;

		if (component->ComponentType() == B_MAIL_MULTIPART_CONTAINER) {
			recursive_attachment_search(us,
				dynamic_cast<BMIMEMultipartMailContainer *>(component), body);
		}

		us->fList->AddItem(new TListItem(component));
	}
}


//	#pragma mark -


TEnclosuresView::TEnclosuresView(BRect rect, BRect windowRect)
	:
	BView(rect, "m_enclosures", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW),
	fFocus(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BFont font(be_plain_font);
	font.SetSize(font.Size() * kPlainFontSizeScale);
	SetFont(&font);

	fOffset = 12;

	BRect r;
	r.left = ENCLOSE_TEXT_H + font.StringWidth(
		B_TRANSLATE("Attachments: ")) + 5;
	r.top = ENCLOSE_FIELD_V;
	r.right = windowRect.right - windowRect.left - B_V_SCROLL_BAR_WIDTH - 9;
	r.bottom = Frame().Height() - 8;
	fList = new TListView(r, this);
	fList->SetInvocationMessage(new BMessage(LIST_INVOKED));

	BScrollView	*scroll = new BScrollView("", fList, B_FOLLOW_LEFT_RIGHT |
			B_FOLLOW_TOP, 0, false, true);
	AddChild(scroll);
	scroll->ScrollBar(B_VERTICAL)->SetRange(0, 0);
}


TEnclosuresView::~TEnclosuresView()
{
	for (int32 index = fList->CountItems();index-- > 0;)
	{
		TListItem *item = static_cast<TListItem *>(fList->ItemAt(index));
		fList->RemoveItem(index);

		if (item->Component() == NULL)
			watch_node(item->NodeRef(), B_STOP_WATCHING, this);
		delete item;
	}
}


void
TEnclosuresView::Draw(BRect where)
{
	BView::Draw(where);

	SetHighColor(0, 0, 0);
	SetLowColor(ViewColor());

	font_height fh;
	GetFontHeight(&fh);

	MovePenTo(ENCLOSE_TEXT_H, ENCLOSE_TEXT_V + fh.ascent);
	DrawString(ENCLOSE_TEXT);
}


void
TEnclosuresView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case LIST_INVOKED:
		{
			BListView *list;
			msg->FindPointer("source", (void **)&list);
			if (list)
			{
				TListItem *item = (TListItem *) (list->ItemAt(msg->FindInt32("index")));
				if (item)
				{
					BMessenger tracker("application/x-vnd.Be-TRAK");
					if (tracker.IsValid())
					{
						BMessage message(B_REFS_RECEIVED);
						message.AddRef("refs", item->Ref());

						tracker.SendMessage(&message);
					}
				}
			}
			break;
		}

		case M_REMOVE:
		{
			int32 index;
			while ((index = fList->CurrentSelection()) >= 0)
			{
				TListItem *item = (TListItem *) fList->ItemAt(index);
				fList->RemoveItem(index);

				if (item->Component())
				{
					TMailWindow *window = dynamic_cast<TMailWindow *>(Window());
					if (window && window->Mail())
						window->Mail()->RemoveComponent(item->Component());

					(new BAlert("", B_TRANSLATE(
						"Removing attachments from a forwarded mail is not yet "
						"implemented!\nIt will not yet work correctly."),
						B_TRANSLATE("OK")))->Go();
				}
				else
					watch_node(item->NodeRef(), B_STOP_WATCHING, this);
				delete item;
			}
			break;
		}

		case M_SELECT:
			fList->Select(0, fList->CountItems() - 1, true);
			break;

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		case REFS_RECEIVED:
			if (msg->HasRef("refs"))
			{
				bool badType = false;

				int32 index = 0;
				entry_ref ref;
				while (msg->FindRef("refs", index++, &ref) == B_NO_ERROR)
				{
					BFile file(&ref, O_RDONLY);
					if (file.InitCheck() == B_OK && file.IsFile())
					{
						TListItem *item;
						for (int16 loop = 0; loop < fList->CountItems(); loop++)
						{
							item = (TListItem *) fList->ItemAt(loop);
							if (ref == *(item->Ref()))
							{
								fList->Select(loop);
								fList->ScrollToSelection();
								continue;
							}
						}
						fList->AddItem(item = new TListItem(&ref));
						fList->Select(fList->CountItems() - 1);
						fList->ScrollToSelection();

						watch_node(item->NodeRef(), B_WATCH_NAME, this);
					}
					else
						badType = true;
				}
				if (badType)
				{
					beep();
					(new BAlert("",
						B_TRANSLATE("Only files can be added as attachments."),
						B_TRANSLATE("OK")))->Go();
				}
			}
			break;

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_NO_ERROR)
			{
				dev_t device;
				if (msg->FindInt32("device", &device) < B_OK)
					break;
				ino_t inode;
				if (msg->FindInt64("node", &inode) < B_OK)
					break;

				for (int32 index = fList->CountItems();index-- > 0;)
				{
					TListItem *item = static_cast<TListItem *>(fList->ItemAt(index));

					if (device == item->NodeRef()->device
						&& inode == item->NodeRef()->node)
					{
						if (opcode == B_ENTRY_REMOVED)
						{
							// don't hide the <missing attachment> item

							//fList->RemoveItem(index);
							//
							//watch_node(item->NodeRef(), B_STOP_WATCHING, this);
							//delete item;
						}
						else if (opcode == B_ENTRY_MOVED)
						{
							item->Ref()->device = device;
							msg->FindInt64("to directory", &item->Ref()->directory);

							const char *name;
							msg->FindString("name", &name);
							item->Ref()->set_name(name);
						}

						fList->InvalidateItem(index);
						break;
					}
				}
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}


void
TEnclosuresView::Focus(bool focus)
{
	if (fFocus != focus)
	{
		fFocus = focus;
		Draw(Frame());
	}
}


void
TEnclosuresView::AddEnclosuresFromMail(BEmailMessage *mail)
{
	for (int32 i = 0; i < mail->CountComponents(); i++)
	{
		BMailComponent *component = mail->GetComponent(i);
		if (component == mail->Body())
			continue;

		if (component->ComponentType() == B_MAIL_MULTIPART_CONTAINER)
			recursive_attachment_search(this,dynamic_cast<BMIMEMultipartMailContainer *>(component),mail->Body());

		fList->AddItem(new TListItem(component));
	}
}


//	#pragma mark -


TListView::TListView(BRect rect, TEnclosuresView *view)
	:
	BListView(rect, "", B_MULTIPLE_SELECTION_LIST,
		B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT),
	fParent(view)
{
}


void
TListView::AttachedToWindow()
{
	BListView::AttachedToWindow();

	BFont font(be_plain_font);
	font.SetSize(font.Size() * kPlainFontSizeScale);
	SetFont(&font);
}


void
TListView::MakeFocus(bool focus)
{
	BListView::MakeFocus(focus);
	fParent->Focus(focus);
}


void
TListView::MouseDown(BPoint point)
{
	int32 buttons;
	Looper()->CurrentMessage()->FindInt32("buttons", &buttons);

	BListView::MouseDown(point);

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0 && IndexOf(point) >= 0) {
		BPopUpMenu menu("enclosure", false, false);
		menu.SetFont(be_plain_font);
		menu.AddItem(new BMenuItem(B_TRANSLATE("Open attachment"),
			new BMessage(LIST_INVOKED)));
		menu.AddItem(new BMenuItem(B_TRANSLATE("Remove attachment"),
			new BMessage(M_REMOVE)));

		BPoint menuStart = ConvertToScreen(point);

		BMenuItem* item = menu.Go(menuStart);
		if (item != NULL) {
			if (item->Command() == LIST_INVOKED) {
				BMessage msg(LIST_INVOKED);
				msg.AddPointer("source",this);
				msg.AddInt32("index",IndexOf(point));
				Window()->PostMessage(&msg,fParent);
			} else {
				Select(IndexOf(point));
				Window()->PostMessage(item->Command(),fParent);
			}
		}
	}
}


void
TListView::KeyDown(const char *bytes, int32 numBytes)
{
	BListView::KeyDown(bytes,numBytes);

	if (numBytes == 1 && *bytes == B_DELETE)
		Window()->PostMessage(M_REMOVE, fParent);
}


//	#pragma mark -


TListItem::TListItem(entry_ref *ref)
{
	fComponent = NULL;
	fRef = *ref;

	BEntry entry(ref);
	entry.GetNodeRef(&fNodeRef);
}


TListItem::TListItem(BMailComponent *component)
	:
	fComponent(component)
{
}


void
TListItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);

	if (Height() < 17)	// mini icon height + 1
		SetHeight(17);
}


void
TListItem::DrawItem(BView *owner, BRect r, bool /* complete */)
{
	if (IsSelected()) {
		owner->SetHighColor(180, 180, 180);
		owner->SetLowColor(180, 180, 180);
	} else {
		owner->SetHighColor(255, 255, 255);
		owner->SetLowColor(255, 255, 255);
	}
	owner->FillRect(r);
	owner->SetHighColor(0, 0, 0);

	BFont font = *be_plain_font;
	font.SetSize(font.Size() * kPlainFontSizeScale);
	owner->SetFont(&font);
	owner->MovePenTo(r.left + 24, r.bottom - 4);

	if (fComponent) {
		// if it's already a mail component, we don't have an icon to
		// draw, and the entry_ref is invalid
		BMailAttachment *attachment = dynamic_cast<BMailAttachment *>(fComponent);

		char name[B_FILE_NAME_LENGTH * 2];
		if ((attachment == NULL) || (attachment->FileName(name) < B_OK))
			strcpy(name, "unnamed");

		BMimeType type;
		if (fComponent->MIMEType(&type) == B_OK)
			sprintf(name + strlen(name), ", Type: %s", type.Type());

		owner->DrawString(name);

		BRect iconRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);

		BBitmap bitmap(iconRect, B_RGBA32);
		if (GetTrackerIcon(type, &bitmap, B_MINI_ICON) == B_NO_ERROR) {
			BRect rect(r.left + 4, r.top + 1, r.left + 4 + 15, r.top + 1 + 15);
			owner->SetDrawingMode(B_OP_ALPHA);
			owner->DrawBitmap(&bitmap, iconRect, rect);
			owner->SetDrawingMode(B_OP_COPY);
		} else {
			// ToDo: find some nicer image for this :-)
			owner->SetHighColor(150, 150, 150);
			owner->FillEllipse(BRect(r.left + 8, r.top + 4, r.left + 16, r.top + 13));
		}
		return;
	}

	BFile file(&fRef, O_RDONLY);
	BEntry entry(&fRef);
	BPath path;
	if (entry.GetPath(&path) == B_OK && file.InitCheck() == B_OK) {
		owner->DrawString(path.Path());

		BNodeInfo info(&file);
		BRect sr(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);

		BBitmap bitmap(sr, B_RGBA32);
		if (info.GetTrackerIcon(&bitmap, B_MINI_ICON) == B_NO_ERROR) {
			BRect dr(r.left + 4, r.top + 1, r.left + 4 + 15, r.top + 1 + 15);
			owner->SetDrawingMode(B_OP_ALPHA);
			owner->DrawBitmap(&bitmap, sr, dr);
			owner->SetDrawingMode(B_OP_COPY);
		}
	} else
		owner->DrawString("<missing attachment>");
}


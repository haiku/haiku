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
#include <ControlLook.h>
#include <Debug.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <StringForSize.h>
#include <StringView.h>

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


TEnclosuresView::TEnclosuresView()
	:
	BView("m_enclosures", B_WILL_DRAW),
	fFocus(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BFont font(be_plain_font);
	font.SetSize(font.Size() * kPlainFontSizeScale);
	SetFont(&font);

	fList = new TListView(this);
	fList->SetInvocationMessage(new BMessage(LIST_INVOKED));

	BStringView* label = new BStringView("label", B_TRANSLATE("Attachments:"));
	BScrollView* scroll = new BScrollView("", fList, 0, false, true);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_SMALL_INSETS, 0,
			scroll->ScrollBar(B_VERTICAL)->PreferredSize().width - 2, -2)
		.Add(label)
		.Add(scroll)
	.End();
}


TEnclosuresView::~TEnclosuresView()
{
	for (int32 index = fList->CountItems();index-- > 0;) {
		TListItem *item = static_cast<TListItem *>(fList->ItemAt(index));
		fList->RemoveItem(index);

		if (item->Component() == NULL)
			watch_node(item->NodeRef(), B_STOP_WATCHING, this);
		delete item;
	}
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
			if (list) {
				TListItem *item = (TListItem *) (list->ItemAt(msg->FindInt32("index")));
				if (item) {
					BMessenger tracker("application/x-vnd.Be-TRAK");
					if (tracker.IsValid()) {
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
			while ((index = fList->CurrentSelection()) >= 0) {
				TListItem *item = (TListItem *) fList->ItemAt(index);
				fList->RemoveItem(index);

				if (item->Component()) {
					TMailWindow *window = dynamic_cast<TMailWindow *>(Window());
					if (window && window->Mail())
						window->Mail()->RemoveComponent(item->Component());

					BAlert* alert = new BAlert("", B_TRANSLATE(
						"Removing attachments from a forwarded mail is not yet "
						"implemented!\nIt will not yet work correctly."),
						B_TRANSLATE("OK"));
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				} else
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
			if (msg->HasRef("refs")) {
				bool badType = false;

				int32 index = 0;
				entry_ref ref;
				while (msg->FindRef("refs", index++, &ref) == B_NO_ERROR) {
					BEntry entry(&ref, true);
					entry.GetRef(&ref);
					BFile file(&ref, O_RDONLY);
					if (file.InitCheck() == B_OK && file.IsFile()) {
						TListItem *item;
						bool exists = false;
						for (int32 loop = 0; loop < fList->CountItems(); loop++) {
							item = (TListItem *) fList->ItemAt(loop);
							if (ref == *(item->Ref())) {
								fList->Select(loop);
								fList->ScrollToSelection();
								exists = true;
								continue;
							}
						}
						if (exists == false) {
							fList->AddItem(item = new TListItem(&ref));
							fList->Select(fList->CountItems() - 1);
							fList->ScrollToSelection();

							watch_node(item->NodeRef(), B_WATCH_NAME, this);
						}
					} else
						badType = true;
				}
				if (badType) {
					beep();
					BAlert* alert = new BAlert("",
						B_TRANSLATE("Only files can be added as attachments."),
						B_TRANSLATE("OK"));
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				}
			}
			break;

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_NO_ERROR) {
				dev_t device;
				if (msg->FindInt32("device", &device) < B_OK)
					break;
				ino_t inode;
				if (msg->FindInt64("node", &inode) < B_OK)
					break;

				for (int32 index = fList->CountItems();index-- > 0;) {
					TListItem *item = static_cast<TListItem *>(fList->ItemAt(index));

					if (device == item->NodeRef()->device
						&& inode == item->NodeRef()->node)
					{
						if (opcode == B_ENTRY_REMOVED) {
							// don't hide the <missing attachment> item

							//fList->RemoveItem(index);
							//
							//watch_node(item->NodeRef(), B_STOP_WATCHING, this);
							//delete item;
						} else if (opcode == B_ENTRY_MOVED) {
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
			break;
	}
}


void
TEnclosuresView::Focus(bool focus)
{
	if (fFocus != focus) {
		fFocus = focus;
		Invalidate();
	}
}


void
TEnclosuresView::AddEnclosuresFromMail(BEmailMessage *mail)
{
	for (int32 i = 0; i < mail->CountComponents(); i++) {
		BMailComponent *component = mail->GetComponent(i);
		if (component == mail->Body())
			continue;

		if (component->ComponentType() == B_MAIL_MULTIPART_CONTAINER) {
			recursive_attachment_search(this,
				dynamic_cast<BMIMEMultipartMailContainer *>(component),
				mail->Body());
		}

		fList->AddItem(new TListItem(component));
	}
}


//	#pragma mark -


TListView::TListView(TEnclosuresView *view)
	:
	BListView("", B_MULTIPLE_SELECTION_LIST),
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


BSize
TListView::MinSize()
{
	BSize size = BListView::MinSize();
	size.height = be_control_look->DefaultLabelSpacing() * 11.0f;
	return size;
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
TListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);

	const float minimalHeight =
		be_control_look->ComposeIconSize(B_MINI_ICON).Height() +
		(be_control_look->DefaultLabelSpacing() / 3.0f);
	if (Height() < minimalHeight)
		SetHeight(minimalHeight);
}


void
TListItem::DrawItem(BView *owner, BRect frame, bool /* complete */)
{
	rgb_color kHighlight = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	rgb_color kHighlightText = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	rgb_color kText = ui_color(B_LIST_ITEM_TEXT_COLOR);

	BRect r(frame);

	if (IsSelected()) {
		owner->SetHighColor(kHighlight);
		owner->SetLowColor(kHighlight);
		owner->FillRect(r);
	}

	const BRect iconRect(BPoint(0, 0), be_control_look->ComposeIconSize(B_MINI_ICON));
	BBitmap iconBitmap(iconRect, B_RGBA32);
	status_t iconStatus = B_NO_INIT;
	BString label;

	if (fComponent) {
		// if it's already a mail component, we don't have an icon to
		// draw, and the entry_ref is invalid
		BMailAttachment *attachment = dynamic_cast<BMailAttachment *>(fComponent);

		char name[B_FILE_NAME_LENGTH * 2];
		if ((attachment == NULL) || (attachment->FileName(name) < B_OK))
			strcpy(name, "unnamed");

		BMimeType type;
		if (fComponent->MIMEType(&type) == B_OK)
			label.SetToFormat("%s, Type: %s", name, type.Type());
		else
			label = name;

		iconStatus = type.GetIcon(&iconBitmap,
			(icon_size)(iconRect.IntegerWidth() + 1));
	} else {
		BFile file(&fRef, O_RDONLY);
		BEntry entry(&fRef);
		BPath path;
		if (entry.GetPath(&path) == B_OK && file.InitCheck() == B_OK) {
			off_t bytes;
			file.GetSize(&bytes);
			char size[B_PATH_NAME_LENGTH];
			string_for_size(bytes, size, sizeof(size));
			label << path.Path() << " (" << size << ")";
			BNodeInfo info(&file);
			iconStatus = info.GetTrackerIcon(&iconBitmap,
				(icon_size)(iconRect.IntegerWidth() + 1));
		} else
			label = "<missing attachment>";
	}

	BRect iconFrame(frame);
	iconFrame.left += be_control_look->DefaultLabelSpacing() / 2;
	iconFrame.Set(iconFrame.left, iconFrame.top + 1,
		iconFrame.left + iconRect.Width(),
		iconFrame.top + iconRect.Height() + 1);

	if (iconStatus == B_OK) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		owner->DrawBitmap(&iconBitmap, iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	} else {
		// ToDo: find some nicer image for this :-)
		owner->SetHighColor(150, 150, 150);
		owner->FillEllipse(iconFrame);
	}

	BFont font;
	owner->GetFont(&font);
	font_height finfo;
	font.GetHeight(&finfo);
	owner->MovePenTo(frame.left + (iconFrame.Width() * 1.5f),
		frame.top + ((frame.Height()
			- (finfo.ascent + finfo.descent + finfo.leading)) / 2)
		+ finfo.ascent);

	owner->SetHighColor(IsSelected() ? kHighlightText : kText);
	owner->DrawString(label.String());
}

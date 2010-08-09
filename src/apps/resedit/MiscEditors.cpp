/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "InternalEditors.h"
#include "ResourceData.h"

#include <Message.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <String.h>

#include <stdlib.h>

StringEditor::StringEditor(const BRect &frame, ResourceData *data,
							BHandler *owner)
  :	Editor(frame, data, owner)
{
	if (data->GetName())
		SetTitle(data->GetName());
	
	fView = new StringEditView(Bounds());
	AddChild(fView);
	
	if (data->IsAttribute())
		fView->EnableID(false);
	else
		fView->SetID(data->GetIDString());
	fView->SetName(data->GetName());
	fView->SetValue(data->GetData());
}


void
StringEditor::MessageReceived(BMessage *msg)
{
	if (msg->what == M_UPDATE_RESOURCE) {
		// We have to have an ID, so if the squirrely developer didn't give us
		// one, don't do anything
		if (fView->GetID()) {
			int32 newid = atol(fView->GetID());
			GetData()->SetID(newid);
		}
		GetData()->SetName(fView->GetName());
		GetData()->SetData(fView->GetValue(), strlen(fView->GetValue()));
		
		BMessage updatemsg(M_UPDATE_RESOURCE);
		updatemsg.AddPointer("item", GetData());
		BMessenger msgr(GetOwner());
		msgr.SendMessage(&updatemsg);
		PostMessage(B_QUIT_REQUESTED);
		
	} else {
		Editor::MessageReceived(msg);
	}
}


Editor::Editor(const BRect &frame, ResourceData *data, BHandler *owner)
  :	BWindow(frame, "Untitled", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
  	fResData(data),
  	fOwner(owner)
{
}


Editor::~Editor(void)
{
}


StringEditView::StringEditView(const BRect &frame)
  :	BView(frame, "edit", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BRect r;
	
	float labelwidth = be_plain_font->StringWidth("ID: ");
	float strwidth = be_plain_font->StringWidth("(attr) ");
	
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float strheight = fh.ascent + fh.descent + fh.leading + 5;
	
	fIDBox = new BTextControl(BRect(10, 10, 10 + (strwidth + labelwidth) + 15,
									10 + strheight),
							  "id", "ID: ", "", NULL);
	fIDBox->SetDivider(labelwidth + 5);
	AddChild(fIDBox);
	
	r = fIDBox->Frame();
	r.OffsetBy(r.Width() + 10, 0);
	r.right = Bounds().right - 10;
	fNameBox = new BTextControl(r, "name", "Name: ", "", NULL,
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetDivider(be_plain_font->StringWidth("Name: ") + 5);
	AddChild(fNameBox);
	
	r.OffsetBy(0, r.Height() + 10);
	r.left = 10;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	BRect textRect(r.OffsetToCopy(0.0, 0.0));
	textRect.InsetBy(5.0, 5.0);
	fValueView = new BTextView(r, "value", textRect, B_FOLLOW_ALL);
	
	BScrollView *scrollView = new BScrollView("scrollView", fValueView,
												B_FOLLOW_ALL, 0, false, true);
	AddChild(scrollView);
	
	fOK = new BButton(BRect(10, 10, 11, 11), "ok", "Cancel", new BMessage(M_UPDATE_RESOURCE),
					  B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fOK->ResizeToPreferred();
	fOK->SetLabel("OK");
	AddChild(fOK);
	
	fOK->MoveTo(r.right - fOK->Bounds().Width(), r.bottom + 10);
	r = fOK->Frame();
	r.OffsetBy(-r.Width() - 10, 0);
	fCancel = new BButton(r, "cancel", "Cancel", new BMessage(B_QUIT_REQUESTED),
					  B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fCancel);
}


StringEditView::~StringEditView(void)
{
}


void
StringEditView::AttachedToWindow(void)
{
	if (Bounds().Height() < fCancel->Frame().bottom + 10) {
		BView *view = FindView("scrollView");
		view->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
		fOK->SetResizingMode(B_FOLLOW_RIGHT);
		fCancel->SetResizingMode(B_FOLLOW_RIGHT);
		Window()->ResizeTo(Window()->Bounds().Width(), fCancel->Frame().bottom + 10);
		view->SetResizingMode(B_FOLLOW_ALL);
		fOK->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		fCancel->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	}
	
	Window()->SetSizeLimits(Window()->Bounds().Width(), 30000,
							Window()->Bounds().Height(), 30000);
}


float
StringEditView::GetPreferredWidth(void) const
{
	float idwidth = be_plain_font->StringWidth("ID: ") + 
					be_plain_font->StringWidth("(attr) ") + 15.0;
	float namewidth = be_plain_font->StringWidth("Name: ") + 
					be_plain_font->StringWidth(fNameBox->Text()) + 15.0;
	return idwidth + namewidth + 100;
}


float
StringEditView::GetPreferredHeight(void) const
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float strheight = fh.ascent + fh.descent + fh.leading + 5;
	float lineCount = fValueView->CountLines() < 5.0 ? fValueView->CountLines() : 5.0;
	return fOK->Frame().Height() + (strheight * lineCount) + 40.0;
}


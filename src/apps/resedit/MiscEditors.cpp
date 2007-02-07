#include "InternalEditors.h"
#include "ResourceData.h"
#include <Messenger.h>
#include <Message.h>
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
	
	SetFlags(Flags() | B_NOT_V_RESIZABLE);
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
		GetData()->SetData(fView->GetValue(),strlen(fView->GetValue()));
		
		BMessage updatemsg(M_UPDATE_RESOURCE);
		updatemsg.AddPointer("item",GetData());
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
	
	fIDBox = new BTextControl(BRect(10,10,10 + (strwidth + labelwidth) + 15,
									10 + strheight),
							  "id","ID: ","", NULL);
	fIDBox->SetDivider(labelwidth + 5);
	AddChild(fIDBox);
	
	r = fIDBox->Frame();
	r.OffsetBy(r.Width() + 10, 0);
	r.right = Bounds().right - 10;
	fNameBox = new BTextControl(r,"name","Name: ","", NULL,
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetDivider(be_plain_font->StringWidth("Name: ") + 5);
	AddChild(fNameBox);
	
	r.OffsetBy(0,r.Height() + 10);
	r.left = 10;
	fValueBox = new BTextControl(r,"value","Value: ","", NULL,
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fValueBox->SetDivider(be_plain_font->StringWidth("Value: ") + 5);
	AddChild(fValueBox);
	
	fOK = new BButton(BRect(10,10,11,11),"ok","Cancel",new BMessage(M_UPDATE_RESOURCE),
					  B_FOLLOW_RIGHT);
	fOK->ResizeToPreferred();
	fOK->SetLabel("OK");
	fOK->MakeDefault(true);
	AddChild(fOK);
	
	fOK->MoveTo(r.right - fOK->Bounds().Width(), r.bottom + 10);
	r = fOK->Frame();
	r.OffsetBy(-r.Width() - 10, 0);
	fCancel = new BButton(r,"cancel","Cancel",new BMessage(B_QUIT_REQUESTED),
					  B_FOLLOW_RIGHT);
	AddChild(fCancel);
}


StringEditView::~StringEditView(void)
{
}


void
StringEditView::AttachedToWindow(void)
{
	if (Bounds().Height() < fCancel->Frame().bottom + 10)
		Window()->ResizeTo(Window()->Bounds().Width(), fCancel->Frame().bottom + 10);
	
	Window()->SetSizeLimits(Window()->Bounds().Width(), 30000,
							Window()->Bounds().Height(), 30000);
}


float
StringEditView::GetPreferredWidth(void) const
{
	float idwidth = be_plain_font->StringWidth("ID: ") + 
					be_plain_font->StringWidth("(attr) ") + 15;
	float namewidth = be_plain_font->StringWidth("Name: ") + 
					be_plain_font->StringWidth(fNameBox->Text()) + 15;
	return idwidth + namewidth + 30;
}


float
StringEditView::GetPreferredHeight(void) const
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float strheight = fh.ascent + fh.descent + fh.leading + 5;
	return fOK->Frame().Height() + (strheight * 2) + 40;
}


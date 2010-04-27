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
#include <String.h>

#include <stdlib.h>

DoubleEditor::DoubleEditor(const BRect &frame, ResourceData *data,
							BHandler *owner)
  :	Editor(frame, data, owner)
{
	if (data->GetName())
		SetTitle(data->GetName());
	
	fView = new StringEditView(Bounds());
	AddChild(fView);
	
	fView->SetID(data->GetIDString());
	fView->SetName(data->GetName());
	fView->SetValue(data->GetData());
}


void
DoubleEditor::MessageReceived(BMessage *msg)
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


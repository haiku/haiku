/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditWindow.h"

#include "Constants.h"
#include "DefaultTypes.h"
#include "EditView.h"
#include "ResourceRow.h"

#include <Box.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>


EditWindow::EditWindow(BRect frame, ResourceRow* row)
	:
	BWindow(frame, "Edit Resource",
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 0)
{
	int32 ix = ResourceType::FindIndex(row->ResourceType());

	fRow = row;

	fView = new BView(Bounds(), "fView", B_FOLLOW_ALL, 0);
	fView->SetLayout(new BGroupLayout(B_VERTICAL));

	fIDText = new BTextControl(BRect(0, 0, 0, 0),
		"fIDText", "ID:", row->ResourceStringID(), NULL);
	fIDText->SetDivider(50);

	fNameText = new BTextControl(BRect(0, 0, 0, 0),
		"fNameText", "Name:", row->ResourceName(), NULL);
	fNameText->SetDivider(50);

	fTypePopUp = new BPopUpMenu("(Type)");

	// TODO: (Not so) evil redundancy.
	for (int32 i = 0; kDefaultTypes[i].type != NULL; i++) {
		if (kDefaultTypes[i].size == ~(uint32)0)
			fTypePopUp->AddSeparatorItem();
		else
			fTypePopUp->AddItem(new BMenuItem(kDefaultTypes[i].type,
				new BMessage(MSG_SELECTION)));
	}
	// ---

	fTypePopUp->ItemAt(ix)->SetMarked(true);

	fTypeMenu = new BMenuField(BRect(0, 0, 0, 0),
		"fTypeMenu", "Type:", fTypePopUp);
	fTypeMenu->SetDivider(50);

	fCodeText = new BTextControl(BRect(0, 0, 0, 0),
		"fCodeText", "Code:", row->ResourceStringCode(), NULL);
	fCodeText->SetDivider(50);

	fEditViewBox = new BBox(BRect(0, 0, 0, 0), "fEditViewBox");
	fEditViewBox->SetLayout(new BGroupLayout(B_VERTICAL));

	fEditView = kDefaultTypes[ix].edit;
	fEditView->AttachTo(fEditViewBox);
	fEditView->Edit(fRow);

	fErrorString = new BStringView(BRect(0, 0, 0, 0),
		"fErrorString", "");

	fErrorString->SetHighColor(ui_color(B_FAILURE_COLOR));

	fCancelButton = new BButton(BRect(0, 0, 0, 0),
		"fCancelButton", "Cancel", new BMessage(MSG_CANCEL));
	fOKButton = new BButton(BRect(0, 0, 0, 0),
		"fOKButton", "OK", new BMessage(MSG_ACCEPT));

	fView->AddChild(BGroupLayoutBuilder(B_VERTICAL, 8)
		.Add(fIDText)
		.Add(fNameText)
		.Add(fTypeMenu)
		.Add(fCodeText)
		.Add(fEditViewBox)
		.Add(fErrorString)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 8)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fOKButton)
			.SetInsets(8, 8, 8, 8)
		)
		.SetInsets(8, 8, 8, 8)
	);

	AddChild(fView);
	ResizeTo(250, 350);

	Show();
}


EditWindow::~EditWindow()
{
	fEditView->RemoveSelf();
}


void
EditWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_SELECTION:
		{
			int32 ix = fTypePopUp->FindMarkedIndex();

			fCodeText->SetText(kDefaultTypes[ix].code);

			fEditView->RemoveSelf();

			fEditView = kDefaultTypes[ix].edit;
			fEditView->AttachTo(fEditViewBox);
			fEditView->Edit(fRow);

			break;
		}
		case MSG_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_ACCEPT:
		{
			if (_Validate()) {
				int32 ix = fTypePopUp->FindMarkedIndex();

				fRow->SetResourceStringID(fIDText->Text());
				fRow->SetResourceName(fNameText->Text());
				fRow->SetResourceType(kDefaultTypes[ix].type);
				fRow->SetResourceStringCode(fCodeText->Text());
				fRow->SetResourceSize(kDefaultTypes[ix].size);

				fEditView->Commit();

				PostMessage(B_QUIT_REQUESTED);
			}

			break;
		}
	}
}


bool
EditWindow::_Validate()
{
	// TODO: Implement validation of entered data.
	fErrorString->SetText("");
	return true;
}

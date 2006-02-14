/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypesWindow.h"
#include "NewFileTypeWindow.h"

#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <string.h>


const uint32 kMsgSupertypeChosen = 'sptc';
const uint32 kMsgNewSupertypeChosen = 'nstc';

const uint32 kMsgNameUpdated = 'nmup';

const uint32 kMsgAddType = 'atyp';


NewFileTypeWindow::NewFileTypeWindow(FileTypesWindow* target, const char* currentType)
	: BWindow(BRect(100, 100, 350, 200), "New File Type", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target)
{
	BRect rect = Bounds();
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	rect.InsetBy(8.0f, 8.0f);
	fNameControl = new BTextControl(rect, "internal", "Internal Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);

	float labelWidth = fNameControl->StringWidth(fNameControl->Label()) + 2.0f;
	fNameControl->SetModificationMessage(new BMessage(kMsgNameUpdated));
	fNameControl->SetDivider(labelWidth);
	fNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of a MIME type name
	BTextView* textView = fNameControl->TextView();
	const char* disallowedCharacters = "/<>@,;:\"()[]?=";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	float width, height;
	fNameControl->GetPreferredSize(&width, &height);
	fNameControl->ResizeTo(rect.Width(), height);
	fNameControl->MoveTo(8.0f, 12.0f + fNameControl->Bounds().Height());
	topView->AddChild(fNameControl);

	fSupertypesMenu = new BPopUpMenu("supertypes");
	BMenuItem* item;
	BMessage types;
	if (BMimeType::GetInstalledSupertypes(&types) == B_OK) {
		const char* type;
		int32 i = 0;
		while (types.FindString("super_types", i++, &type) == B_OK) {
			fSupertypesMenu->AddItem(item = new BMenuItem(type,
				new BMessage(kMsgSupertypeChosen)));

			// select super type close to the current type
			if (currentType != NULL) {
				if (!strncmp(type, currentType, strlen(type)))
					item->SetMarked(true);
			} else if (i == 1)
				item->SetMarked(true);
		}

		if (i > 1)
			fSupertypesMenu->AddSeparatorItem();
	}
	fSupertypesMenu->AddItem(new BMenuItem("Add New Group",
		new BMessage(kMsgNewSupertypeChosen)));

	rect.bottom = rect.top + fNameControl->Bounds().Height() + 2.0f;
	BMenuField* menuField = new BMenuField(rect, "supertypes",
		"Group:", fSupertypesMenu);
	menuField->SetDivider(labelWidth);
	menuField->SetAlignment(B_ALIGN_RIGHT);
	menuField->GetPreferredSize(&width, &height);
	menuField->ResizeTo(rect.Width(), height);
	topView->AddChild(menuField);

	fAddButton = new BButton(rect, "add", "Add Type", new BMessage(kMsgAddType),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fAddButton->ResizeToPreferred();
	fAddButton->MoveTo(Bounds().Width() - 8.0f - fAddButton->Bounds().Width(),
		Bounds().Height() - 8.0f - fAddButton->Bounds().Height());
	fAddButton->SetEnabled(false);
	topView->AddChild(fAddButton);

	BButton* button = new BButton(rect, "cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(fAddButton->Frame().left - 10.0f - button->Bounds().Width(),
		fAddButton->Frame().top);
	topView->AddChild(button);

	ResizeTo(labelWidth * 4.0f + 24.0f, fNameControl->Bounds().Height() * 3.0f + 34.0f);
	SetSizeLimits(button->Bounds().Width() + fAddButton->Bounds().Width() + 26.0f,
		32767.0f, Frame().Height(), Frame().Height());

	fAddButton->MakeDefault(true);
	fNameControl->MakeFocus(true);

	target->PlaceSubWindow(this);
}


NewFileTypeWindow::~NewFileTypeWindow()
{
}

#include <stdio.h>
void
NewFileTypeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSupertypeChosen:
			fAddButton->SetLabel("Add Type");
			fNameControl->SetLabel("Internal Name:");
			fNameControl->MakeFocus(true);
			break;

		case kMsgNewSupertypeChosen:
			fAddButton->SetLabel("Add Group");
			fNameControl->SetLabel("Group Name:");
			fNameControl->MakeFocus(true);
			break;

		case kMsgNameUpdated:
		{
			bool empty = fNameControl->Text() == NULL
				|| fNameControl->Text()[0] == '\0';

			if (fAddButton->IsEnabled() == empty)
				fAddButton->SetEnabled(!empty);
			break;
		}

		case kMsgAddType:
		{
			BMenuItem* item = fSupertypesMenu->FindMarked();
			if (item != NULL) {
				BString type;
				if (fSupertypesMenu->IndexOf(item) != fSupertypesMenu->CountItems() - 1) {
					// add normal type
					type = item->Label();
					type.Append("/");
				}

				type.Append(fNameControl->Text());

				BMimeType mimeType(type.String());
				if (mimeType.IsInstalled()) {
					error_alert("This file type already exists.");
					break;
				}

				status_t status = mimeType.Install();
				if (status != B_OK)
					error_alert("Could not install file type", status);
				else {
					BMessage update(kMsgSelectNewType);
					update.AddString("type", type.String());

					fTarget.SendMessage(&update);
				}
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
	}
}


bool
NewFileTypeWindow::QuitRequested()
{
	fTarget.SendMessage(kMsgNewTypeWindowClosed);
	return true;
}

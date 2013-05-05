/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "NewFileTypeWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "New File Type Window"


const uint32 kMsgSupertypeChosen = 'sptc';
const uint32 kMsgNewSupertypeChosen = 'nstc';

const uint32 kMsgNameUpdated = 'nmup';

const uint32 kMsgAddType = 'atyp';


NewFileTypeWindow::NewFileTypeWindow(FileTypesWindow* target,
	const char* currentType)
	:
	BWindow(BRect(100, 100, 350, 200), B_TRANSLATE("New file type"),
		B_MODAL_WINDOW, B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS ),
	fTarget(target)
{
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

	fSupertypesMenu->AddItem(new BMenuItem(B_TRANSLATE("Add new group"),
		new BMessage(kMsgNewSupertypeChosen)));
	BMenuField* typesMenuField = new BMenuField(NULL, fSupertypesMenu);

	BStringView* typesMenuLabel = new BStringView(NULL, B_TRANSLATE("Group:"));
		// Create a separate label view, otherwise things don't line up right
	typesMenuLabel->SetAlignment(B_ALIGN_LEFT);
	typesMenuLabel->SetExplicitAlignment(
		BAlignment(B_ALIGN_LEFT, B_ALIGN_USE_FULL_HEIGHT));

	fNameControl = new BTextControl(B_TRANSLATE("Internal name:"), "", NULL);
	fNameControl->SetModificationMessage(new BMessage(kMsgNameUpdated));

	// filter out invalid characters that can't be part of a MIME type name
	BTextView* nameControlTextView = fNameControl->TextView();
	const char* disallowedCharacters = "/<>@,;:\"()[]?= ";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		nameControlTextView->DisallowChar(disallowedCharacters[i]);
	}

	fAddButton = new BButton(B_TRANSLATE("Add type"),
		new BMessage(kMsgAddType));

	float padding = be_control_look->DefaultItemSpacing();

	BLayoutBuilder::Grid<>(this, padding, padding)
		.SetInsets(padding)
		.Add(typesMenuLabel, 0, 0)
		.Add(typesMenuField, 1, 0, 2)
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 1)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 1, 2)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 2)
		.Add(new BButton(B_TRANSLATE("Cancel"),
			new BMessage(B_QUIT_REQUESTED)), 1, 2)
		.Add(fAddButton, 2, 2)
		.SetColumnWeight(0, 3);

	BAlignment fullSize = BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_USE_FULL_HEIGHT);
	typesMenuField->MenuBar()->SetExplicitAlignment(fullSize);
	fNameControl->TextView()->SetExplicitAlignment(fullSize);

	BLayoutItem* nameControlLabelItem = fNameControl->CreateLabelLayoutItem();
	nameControlLabelItem->SetExplicitMinSize(nameControlLabelItem->MinSize());
		// stops fNameControl's label from truncating under certain conditions

	fAddButton->MakeDefault(true);
	fNameControl->MakeFocus(true);

	target->PlaceSubWindow(this);
}


NewFileTypeWindow::~NewFileTypeWindow()
{
}


void
NewFileTypeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSupertypeChosen:
			fAddButton->SetLabel(B_TRANSLATE("Add type"));
			fNameControl->SetLabel(B_TRANSLATE("Internal name:"));
			fNameControl->MakeFocus(true);
			InvalidateLayout(true);
			break;

		case kMsgNewSupertypeChosen:
			fAddButton->SetLabel(B_TRANSLATE("Add group"));
			fNameControl->SetLabel(B_TRANSLATE("Group name:"));
			fNameControl->MakeFocus(true);
			InvalidateLayout(true);
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
				if (fSupertypesMenu->IndexOf(item)
						!= fSupertypesMenu->CountItems() - 1) {
					// add normal type
					type = item->Label();
					type.Append("/");
				}

				type.Append(fNameControl->Text());

				BMimeType mimeType(type.String());
				if (mimeType.IsInstalled()) {
					error_alert(B_TRANSLATE("This file type already exists"));
					break;
				}

				status_t status = mimeType.Install();
				if (status != B_OK)
					error_alert(B_TRANSLATE("Could not install file type"),
						status);
				else {
					BMessage update(kMsgSelectNewType);
					update.AddString("type", type.String());

					fTarget.SendMessage(&update);
				}
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
NewFileTypeWindow::QuitRequested()
{
	fTarget.SendMessage(kMsgNewTypeWindowClosed);
	return true;
}



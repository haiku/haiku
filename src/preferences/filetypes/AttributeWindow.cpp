/*
 * Copyright 2006-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const uint32 kMsgAttributeUpdated = 'atup';
const uint32 kMsgTypeChosen = 'typc';
const uint32 kMsgDisplayAsChosen = 'dach';
const uint32 kMsgVisibilityChanged = 'vsch';
const uint32 kMsgAlignmentChosen = 'alnc';
const uint32 kMsgAccept = 'acpt';


static int
compare_attributes(const void* _a, const void* _b)
{
	AttributeItem* a = *(AttributeItem **)_a;
	AttributeItem* b = *(AttributeItem **)_b;

	int compare = strcasecmp(a->PublicName(), b->PublicName());
	if (compare != 0)
		return compare;

	return strcmp(a->Name(), b->Name());
}


static bool
compare_display_as(const char* a, const char* b)
{
	bool emptyA = a == NULL || !a[0];
	bool emptyB = b == NULL || !b[0];

	if (emptyA && emptyB)
		return true;
	if (emptyA || emptyB)
		return false;

	const char* end = strstr(a, ": ");
	int32 lengthA = end ? end - a : strlen(a);
	end = strstr(b, ": ");
	int32 lengthB = end ? end - b : strlen(b);

	if (lengthA != lengthB)
		return false;

	return !strncasecmp(a, b, lengthA);
}


static const char*
display_as_parameter(const char* special)
{
	const char* parameter = strstr(special, ": ");
	if (parameter != NULL)
		return parameter + 2;

	return NULL;
}


//	#pragma mark -


AttributeWindow::AttributeWindow(FileTypesWindow* target, BMimeType& mimeType,
		AttributeItem* attributeItem)
	: BWindow(BRect(100, 100, 350, 200), "Attribute", B_MODAL_WINDOW_LOOK,
		B_MODAL_SUBSET_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE
			| B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fMimeType(mimeType.Type())
{
	if (attributeItem != NULL)
		fAttribute = *attributeItem;

	BRect rect = Bounds();
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	rect.InsetBy(8.0f, 8.0f);
	fPublicNameControl = new BTextControl(rect, "public", "Attribute Name:",
		fAttribute.PublicName(), NULL, B_FOLLOW_LEFT_RIGHT);
	fPublicNameControl->SetModificationMessage(new BMessage(kMsgAttributeUpdated));

	float labelWidth = fPublicNameControl->StringWidth(fPublicNameControl->Label()) + 2.0f;
	fPublicNameControl->SetDivider(labelWidth);
	fPublicNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	float width, height;
	fPublicNameControl->GetPreferredSize(&width, &height);
	fPublicNameControl->ResizeTo(rect.Width(), height);
	topView->AddChild(fPublicNameControl);

	rect = fPublicNameControl->Frame();
	rect.OffsetBy(0.0f, rect.Height() + 5.0f);
	fAttributeControl = new BTextControl(rect, "internal", "Internal Name:",
		fAttribute.Name(), NULL, B_FOLLOW_LEFT_RIGHT);
	fAttributeControl->SetModificationMessage(new BMessage(kMsgAttributeUpdated));
	fAttributeControl->SetDivider(labelWidth);
	fAttributeControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of an attribute
	BTextView* textView = fAttributeControl->TextView();
	const char* disallowedCharacters = "/";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

	topView->AddChild(fAttributeControl);

	fTypeMenu = new BPopUpMenu("type");
	BMenuItem* item = NULL;
	for (int32 i = 0; kTypeMap[i].name != NULL; i++) {
		BMessage* message = new BMessage(kMsgTypeChosen);
		message->AddInt32("type", kTypeMap[i].type);

		item = new BMenuItem(kTypeMap[i].name, message);
		fTypeMenu->AddItem(item);

		if (kTypeMap[i].type == fAttribute.Type())
			item->SetMarked(true);
	}

	rect.OffsetBy(0.0f, rect.Height() + 4.0f);
	BMenuField* menuField = new BMenuField(rect, "types",
		"Type:", fTypeMenu);
	menuField->SetDivider(labelWidth);
	menuField->SetAlignment(B_ALIGN_RIGHT);
	menuField->GetPreferredSize(&width, &height);
	menuField->ResizeTo(rect.Width(), height);
	topView->AddChild(menuField);

	rect.OffsetBy(0.0f, rect.Height() + 4.0f);
	rect.bottom = rect.top + fAttributeControl->Bounds().Height() * 2.0f + 18.0f;
	BBox* box = new BBox(rect, "", B_FOLLOW_LEFT_RIGHT);
	topView->AddChild(box);

	fVisibleCheckBox = new BCheckBox(rect, "visible", "Visible",
		new BMessage(kMsgVisibilityChanged));
	fVisibleCheckBox->SetValue(fAttribute.Visible());
	fVisibleCheckBox->ResizeToPreferred();
	box->SetLabel(fVisibleCheckBox);

	labelWidth -= 8.0f;

	BMenu* menu = new BPopUpMenu("display as");
	for (int32 i = 0; kDisplayAsMap[i].name != NULL; i++) {
		BMessage* message = new BMessage(kMsgDisplayAsChosen);
		if (kDisplayAsMap[i].identifier != NULL) {
			message->AddString("identifier", kDisplayAsMap[i].identifier);
			for (int32 j = 0; kDisplayAsMap[i].supported[j]; j++) {
				message->AddInt32("supports", kDisplayAsMap[i].supported[j]);
			}
		}

		item = new BMenuItem(kDisplayAsMap[i].name, message);
		menu->AddItem(item);

		if (compare_display_as(kDisplayAsMap[i].identifier, fAttribute.DisplayAs()))
			item->SetMarked(true);
	}

	rect.OffsetTo(8.0f, fVisibleCheckBox->Bounds().Height());
	rect.right -= 18.0f;
	fDisplayAsMenuField = new BMenuField(rect, "display as",
		"Display As:", menu);
	fDisplayAsMenuField->SetDivider(labelWidth);
	fDisplayAsMenuField->SetAlignment(B_ALIGN_RIGHT);
	fDisplayAsMenuField->ResizeTo(rect.Width(), height);
	box->AddChild(fDisplayAsMenuField);

	fEditableCheckBox = new BCheckBox(rect, "editable", "Editable",
		new BMessage(kMsgAttributeUpdated), B_FOLLOW_RIGHT);
	fEditableCheckBox->SetValue(fAttribute.Editable());
	fEditableCheckBox->ResizeToPreferred();
	fEditableCheckBox->MoveTo(rect.right - fEditableCheckBox->Bounds().Width(),
		rect.top + (fDisplayAsMenuField->Bounds().Height()
		- fEditableCheckBox->Bounds().Height()) / 2.0f);
	box->AddChild(fEditableCheckBox);

	rect.OffsetBy(0.0f, menuField->Bounds().Height() + 4.0f);
	rect.bottom = rect.top + fPublicNameControl->Bounds().Height();
	fSpecialControl = new BTextControl(rect, "special", "Special:",
		display_as_parameter(fAttribute.DisplayAs()), NULL,
		B_FOLLOW_LEFT_RIGHT);
	fSpecialControl->SetModificationMessage(new BMessage(kMsgAttributeUpdated));
	fSpecialControl->SetDivider(labelWidth);
	fSpecialControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fSpecialControl->SetEnabled(false);
	box->AddChild(fSpecialControl);

	char text[64];
	snprintf(text, sizeof(text), "%ld", fAttribute.Width());
	rect.OffsetBy(0.0f, fSpecialControl->Bounds().Height() + 4.0f);
	fWidthControl = new BTextControl(rect, "width", "Width:",
		text, NULL, B_FOLLOW_LEFT_RIGHT);
	fWidthControl->SetModificationMessage(new BMessage(kMsgAttributeUpdated));
	fWidthControl->SetDivider(labelWidth);
	fWidthControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of a width
	textView = fWidthControl->TextView();
	for (int32 i = 0; i < 256; i++) {
		if (!isdigit(i))
			textView->DisallowChar(i);
	}
	textView->SetMaxBytes(4);

	box->AddChild(fWidthControl);

	const struct alignment_map {
		int32		alignment;
		const char*	name;
	} kAlignmentMap[] = {
		{B_ALIGN_LEFT, "Left"},
		{B_ALIGN_RIGHT, "Right"},
		{B_ALIGN_CENTER, "Center"},
		{0, NULL}
	};

	menu = new BPopUpMenu("alignment");
	for (int32 i = 0; kAlignmentMap[i].name != NULL; i++) {
		BMessage* message = new BMessage(kMsgAlignmentChosen);
		message->AddInt32("alignment", kAlignmentMap[i].alignment);

		item = new BMenuItem(kAlignmentMap[i].name, message);
		menu->AddItem(item);

		if (kAlignmentMap[i].alignment == fAttribute.Alignment())
			item->SetMarked(true);
	}

	rect.OffsetBy(0.0f, menuField->Bounds().Height() + 1.0f);
	fAlignmentMenuField = new BMenuField(rect, "alignment",
		"Alignment:", menu);
	fAlignmentMenuField->SetDivider(labelWidth);
	fAlignmentMenuField->SetAlignment(B_ALIGN_RIGHT);
	fAlignmentMenuField->ResizeTo(rect.Width(), height);
	box->AddChild(fAlignmentMenuField);
	box->ResizeBy(0.0f, fAlignmentMenuField->Bounds().Height() * 2.0f
		+ fVisibleCheckBox->Bounds().Height());

	fAcceptButton = new BButton(rect, "add", item ? "Done" : "Add",
		new BMessage(kMsgAccept), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fAcceptButton->ResizeToPreferred();
	fAcceptButton->MoveTo(Bounds().Width() - 8.0f - fAcceptButton->Bounds().Width(),
		Bounds().Height() - 8.0f - fAcceptButton->Bounds().Height());
	fAcceptButton->SetEnabled(false);
	topView->AddChild(fAcceptButton);

	BButton* button = new BButton(rect, "cancel", "Cancel",
		new BMessage(B_QUIT_REQUESTED), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(fAcceptButton->Frame().left - 10.0f - button->Bounds().Width(),
		fAcceptButton->Frame().top);
	topView->AddChild(button);

	ResizeTo(labelWidth * 4.0f + 24.0f, box->Frame().bottom
		+ button->Bounds().Height() + 20.0f);
	SetSizeLimits(fEditableCheckBox->Bounds().Width() + button->Bounds().Width()
		+ fAcceptButton->Bounds().Width() + labelWidth + 24.0f,
		32767.0f, Frame().Height(), Frame().Height());

	fAcceptButton->MakeDefault(true);
	fPublicNameControl->MakeFocus(true);

	target->PlaceSubWindow(this);
	AddToSubset(target);
}


AttributeWindow::~AttributeWindow()
{
}


type_code
AttributeWindow::_CurrentType() const
{
	type_code type = B_STRING_TYPE;
	BMenuItem* item = fTypeMenu->FindMarked();
	if (item != NULL && item->Message() != NULL) {
		int32 value;
		if (item->Message()->FindInt32("type", &value) == B_OK)
			type = value;
	}

	return type;
}


BMenuItem*
AttributeWindow::_DefaultDisplayAs() const
{
	return fDisplayAsMenuField->Menu()->ItemAt(0);
}


void
AttributeWindow::_CheckDisplayAs()
{
	// check display as suported types

	type_code currentType = _CurrentType();

	BMenu* menu = fDisplayAsMenuField->Menu();
	for (int32 i = menu->CountItems(); i-- > 0;) {
		BMenuItem* item = menu->ItemAt(i);
		bool supported = item == _DefaultDisplayAs();
			// the default type is always supported
		type_code type;
		for (int32 j = 0; item->Message()->FindInt32("supports",
				j, (int32*)&type) == B_OK; j++) {
			if (type == currentType) {
				supported = true;
				break;
			}
		}

		item->SetEnabled(supported);
		if (item->IsMarked() && !supported)
			menu->ItemAt(0)->SetMarked(true);
	}

	fSpecialControl->SetEnabled(!_DefaultDisplayAs()->IsMarked());
}


void
AttributeWindow::_CheckAcceptable()
{
	bool enabled = fAttributeControl->Text() != NULL
		&& fAttributeControl->Text()[0] != '\0'
		&& fPublicNameControl->Text() != NULL
		&& fPublicNameControl->Text()[0] != '\0';

	if (enabled) {
		// check for equality
		AttributeItem* item = _NewItemFromCurrent();
		enabled = fAttribute != *item;
		delete item;
	}

	// Update button

	if (fAcceptButton->IsEnabled() != enabled)
		fAcceptButton->SetEnabled(enabled);
}


AttributeItem*
AttributeWindow::_NewItemFromCurrent()
{
	const char* newAttribute = fAttributeControl->Text();

	type_code type = _CurrentType();

	int32 alignment = B_ALIGN_LEFT;
	BMenuItem* item = fAlignmentMenuField->Menu()->FindMarked();
	if (item != NULL && item->Message() != NULL) {
		int32 value;
		if (item->Message()->FindInt32("alignment", &value) == B_OK)
			alignment = value;
	}

	int32 width = atoi(fWidthControl->Text());
	if (width < 0)
		width = 0;

	BString displayAs;
	item = fDisplayAsMenuField->Menu()->FindMarked();
	if (item != NULL) {
		const char* identifier;
		if (item->Message()->FindString("identifier", &identifier) == B_OK) {
			displayAs = identifier;

			if (fSpecialControl->Text() && fSpecialControl->Text()[0]) {
				displayAs += ": ";
				displayAs += fSpecialControl->Text();
			}
		}
	}

	return new AttributeItem(newAttribute,
		fPublicNameControl->Text(), type, displayAs.String(), alignment,
		width, fVisibleCheckBox->Value() == B_CONTROL_ON,
		fEditableCheckBox->Value() == B_CONTROL_ON);
}


void
AttributeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAttributeUpdated:
		case kMsgAlignmentChosen:
		case kMsgTypeChosen:
			_CheckDisplayAs();
			_CheckAcceptable();
			break;

		case kMsgDisplayAsChosen:
			fSpecialControl->SetEnabled(!_DefaultDisplayAs()->IsMarked());
			break;

		case kMsgVisibilityChanged:
		{
			bool enabled = fVisibleCheckBox->Value() != B_CONTROL_OFF;

			fDisplayAsMenuField->SetEnabled(enabled);
			fWidthControl->SetEnabled(enabled);
			fAlignmentMenuField->SetEnabled(enabled);
			fEditableCheckBox->SetEnabled(enabled);

			_CheckDisplayAs();
			_CheckAcceptable();
			break;
		}

		case kMsgAccept:
		{
			BMessage attributes;
			status_t status = fMimeType.GetAttrInfo(&attributes);
			if (status == B_OK) {
				// replace the entry, and remove any equivalent entries
				BList list;

				const char* newAttribute = fAttributeControl->Text();
				list.AddItem(_NewItemFromCurrent());

				const char* attribute;
				for (int32 i = 0; attributes.FindString("attr:name", i,
						&attribute) == B_OK; i++) {
					if (!strcmp(fAttribute.Name(), attribute)
						|| !strcmp(newAttribute, attribute)) {
						// remove this item
						continue;
					}

					AttributeItem* item = create_attribute_item(attributes, i);
					if (item != NULL)
						list.AddItem(item);
				}

				list.SortItems(compare_attributes);

				// Copy them to a new message (their memory is still part of the
				// original BMessage)
				BMessage newAttributes;
				for (int32 i = 0; i < list.CountItems(); i++) {
					AttributeItem* item = (AttributeItem*)list.ItemAt(i);

					newAttributes.AddString("attr:name", item->Name());
					newAttributes.AddString("attr:public_name", item->PublicName());
					newAttributes.AddInt32("attr:type", (int32)item->Type());
					newAttributes.AddString("attr:display_as", item->DisplayAs());
					newAttributes.AddInt32("attr:alignment", item->Alignment());
					newAttributes.AddInt32("attr:width", item->Width());
					newAttributes.AddBool("attr:viewable", item->Visible());
					newAttributes.AddBool("attr:editable", item->Editable());
					
					delete item;
				}

				status = fMimeType.SetAttrInfo(&newAttributes);
			}

			if (status != B_OK)
				error_alert("Could not change attributes", status);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
AttributeWindow::QuitRequested()
{
	return true;
}

/*
 * Copyright 2006-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextControl.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Attribute Window"


const uint32 kMsgAttributeUpdated = 'atup';
const uint32 kMsgTypeChosen = 'typc';
const uint32 kMsgDisplayAsChosen = 'dach';
const uint32 kMsgVisibilityChanged = 'vsch';
const uint32 kMsgAlignmentChosen = 'alnc';
const uint32 kMsgAccept = 'acpt';


static bool
compare_display_as(const char* a, const char* b)
{
	bool emptyA = a == NULL || !a[0];
	bool emptyB = b == NULL || !b[0];

	if (emptyA && emptyB)
		return true;
	if (emptyA || emptyB)
		return false;

	const char* end = strchr(a, ':');
	int32 lengthA = end ? end - a : strlen(a);
	end = strchr(b, ':');
	int32 lengthB = end ? end - b : strlen(b);

	if (lengthA != lengthB)
		return false;

	return !strncasecmp(a, b, lengthA);
}


static const char*
display_as_parameter(const char* special)
{
	const char* parameter = strchr(special, ':');
	if (parameter != NULL)
		return parameter + 1;

	return NULL;
}


//	#pragma mark -


AttributeWindow::AttributeWindow(FileTypesWindow* target, BMimeType& mimeType,
		AttributeItem* attributeItem)
	:
	BWindow(BRect(100, 100, 350, 200), B_TRANSLATE("Attribute"),
		B_MODAL_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS),
	fTarget(target),
	fMimeType(mimeType.Type())
{
	float padding = be_control_look->DefaultItemSpacing();

	if (attributeItem != NULL)
		fAttribute = *attributeItem;

	fPublicNameControl = new BTextControl(B_TRANSLATE("Attribute name:"),
		fAttribute.PublicName(), NULL);
	fPublicNameControl->SetModificationMessage(
		new BMessage(kMsgAttributeUpdated));
	fPublicNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	fAttributeControl = new BTextControl(B_TRANSLATE("Internal name:"),
		fAttribute.Name(), NULL);
	fAttributeControl->SetModificationMessage(
		new BMessage(kMsgAttributeUpdated));
	fAttributeControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of an attribute
	BTextView* textView = fAttributeControl->TextView();
	const char* disallowedCharacters = "/";
	for (int32 i = 0; disallowedCharacters[i]; i++) {
		textView->DisallowChar(disallowedCharacters[i]);
	}

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

	BMenuField* typeMenuField = new BMenuField("types" , B_TRANSLATE("Type:"),
		fTypeMenu);
	typeMenuField->SetAlignment(B_ALIGN_RIGHT);
	// we must set the color manually when adding a menuField directly
	// into a window.
	typeMenuField->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	typeMenuField->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fVisibleCheckBox = new BCheckBox("visible", B_TRANSLATE("Visible"),
		new BMessage(kMsgVisibilityChanged));
	fVisibleCheckBox->SetValue(fAttribute.Visible());

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

		if (compare_display_as(kDisplayAsMap[i].identifier,
				fAttribute.DisplayAs()))
			item->SetMarked(true);
	}

	fDisplayAsMenuField = new BMenuField("display as",
		B_TRANSLATE_COMMENT("Display as:",
			"Tracker offers different display modes for attributes."), menu);
	fDisplayAsMenuField->SetAlignment(B_ALIGN_RIGHT);

	fEditableCheckBox = new BCheckBox("editable",
		B_TRANSLATE_COMMENT("Editable",
			"If Tracker allows to edit this attribute."),
		new BMessage(kMsgAttributeUpdated));
	fEditableCheckBox->SetValue(fAttribute.Editable());

	fSpecialControl = new BTextControl(B_TRANSLATE("Special:"),
		display_as_parameter(fAttribute.DisplayAs()), NULL);
	fSpecialControl->SetModificationMessage(
		new BMessage(kMsgAttributeUpdated));
	fSpecialControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fSpecialControl->SetEnabled(false);

	char text[64];
	snprintf(text, sizeof(text), "%ld", fAttribute.Width());
	fWidthControl = new BTextControl(B_TRANSLATE_COMMENT("Width:",
		"Default column width in Tracker for this attribute."),
		text, NULL);
	fWidthControl->SetModificationMessage(
		new BMessage(kMsgAttributeUpdated));
	fWidthControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	// filter out invalid characters that can't be part of a width
	textView = fWidthControl->TextView();
	for (int32 i = 0; i < 256; i++) {
		if (!isdigit(i))
			textView->DisallowChar(i);
	}
	textView->SetMaxBytes(4);

	const struct alignment_map {
		int32		alignment;
		const char*	name;
	} kAlignmentMap[] = {
		{B_ALIGN_LEFT, B_TRANSLATE_COMMENT("Left",
			"Attribute column alignment in Tracker")},
		{B_ALIGN_RIGHT, B_TRANSLATE_COMMENT("Right",
			"Attribute column alignment in Tracker")},
		{B_ALIGN_CENTER, B_TRANSLATE_COMMENT("Center",
			"Attribute column alignment in Tracker")},
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

	fAlignmentMenuField = new BMenuField("alignment",
		B_TRANSLATE("Alignment:"), menu);
	fAlignmentMenuField->SetAlignment(B_ALIGN_RIGHT);

	fAcceptButton = new BButton("add",
		item ? B_TRANSLATE("Done") : B_TRANSLATE("Add"),
		new BMessage(kMsgAccept));
	fAcceptButton->SetEnabled(false);

	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));

	BBox* visibleBox;
	BLayoutBuilder::Group<>(this, B_VERTICAL, padding)
		.SetInsets(padding, padding, padding, padding)
		.AddGrid(padding, padding / 2)
			.Add(fPublicNameControl->CreateLabelLayoutItem(), 0, 0)
			.Add(fPublicNameControl->CreateTextViewLayoutItem(), 1, 0)
			.Add(fAttributeControl->CreateLabelLayoutItem(), 0, 1)
			.Add(fAttributeControl->CreateTextViewLayoutItem(), 1, 1)
			.Add(typeMenuField->CreateLabelLayoutItem(), 0, 2)
			.Add(typeMenuField->CreateMenuBarLayoutItem(), 1, 2)
			.End()
		.Add(visibleBox = new BBox(B_FANCY_BORDER,
			BLayoutBuilder::Grid<>(padding, padding / 2)
				.Add(fDisplayAsMenuField->CreateLabelLayoutItem(), 0, 0)
				.Add(fDisplayAsMenuField->CreateMenuBarLayoutItem(), 1, 0)
				.Add(fEditableCheckBox, 2, 0)
				.Add(fSpecialControl->CreateLabelLayoutItem(), 0, 1)
				.Add(fSpecialControl->CreateTextViewLayoutItem(), 1, 1, 2)
				.Add(fWidthControl->CreateLabelLayoutItem(), 0, 2)
				.Add(fWidthControl->CreateTextViewLayoutItem(), 1, 2, 2)
				.Add(fAlignmentMenuField->CreateLabelLayoutItem(), 0, 3)
				.Add(fAlignmentMenuField->CreateMenuBarLayoutItem(), 1, 3, 2)
				.SetInsets(padding, padding, padding, padding)
				.View())
			)
		.AddGroup(B_HORIZONTAL, padding)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(cancelButton)
			.Add(fAcceptButton);

	visibleBox->SetLabel(fVisibleCheckBox);

	fAcceptButton->MakeDefault(true);
	fPublicNameControl->MakeFocus(true);

	target->PlaceSubWindow(this);
	AddToSubset(target);

	_CheckDisplayAs();
	_CheckAcceptable();
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
				displayAs += ":";
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
			_CheckAcceptable();
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

				list.AddItem(_NewItemFromCurrent());

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

			if (status != B_OK) {
				error_alert(B_TRANSLATE("Could not change attributes"),
					status);
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
AttributeWindow::QuitRequested()
{
	return true;
}

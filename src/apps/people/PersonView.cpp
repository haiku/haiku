/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "PersonView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BitmapStream.h>
#include <Catalog.h>
#include <fs_attr.h>
#include <Box.h>
#include <ControlLook.h>
#include <GridLayout.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <TranslationUtils.h>
#include <Translator.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "AttributeTextControl.h"
#include "PictureView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "People"


PersonView::PersonView(const char* name, const char* categoryAttribute,
		const entry_ref *ref)
	:
	BGridView(),
	fLastModificationTime(0),
	fGroups(NULL),
	fControls(20, false),
	fCategoryAttribute(categoryAttribute),
	fPictureView(NULL),
	fSaving(false)
{
	SetName(name);
	SetFlags(Flags() | B_WILL_DRAW);

	if (ref)
		fFile = new BFile(ref, O_RDWR);
	else
		fFile = NULL;

	float spacing = be_control_look->DefaultItemSpacing();
	BGridLayout* layout = GridLayout();
	layout->SetInsets(spacing, spacing, spacing, spacing);

	// Add picture "field", using ID photo 35mm x 45mm ratio
	fPictureView = new PictureView(70, 90, ref);

	layout->AddView(fPictureView, 0, 0, 1, 5);
	layout->ItemAt(0, 0)->SetExplicitAlignment(
		BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));

	if (fFile)
		fFile->GetModificationTime(&fLastModificationTime);
}


PersonView::~PersonView()
{
	delete fFile;
}


void
PersonView::AddAttribute(const char* label, const char* attribute)
{
	// Check if this attribute has already been added.
	AttributeTextControl* control = NULL;
	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		if (fControls.ItemAt(i)->Attribute() == attribute) {
			return;
		}
	}

	control = new AttributeTextControl(label, attribute);
	fControls.AddItem(control);

	BGridLayout* layout = GridLayout();
	int32 row = fControls.CountItems();

	if (fCategoryAttribute == attribute) {
		// Special case the category attribute. The Group popup field will
		// be added as the label instead.
		fGroups = new BPopUpMenu(label);
		fGroups->SetRadioMode(false);
		BuildGroupMenu();

		BMenuField* field = new BMenuField("", "", fGroups);
		field->SetEnabled(true);
		layout->AddView(field, 1, row);

		control->SetLabel("");
		layout->AddView(control, 2, row);
	} else {
		layout->AddItem(control->CreateLabelLayoutItem(), 1, row);
		layout->AddItem(control->CreateTextViewLayoutItem(), 2, row);
	}

	SetAttribute(attribute, true);
}


void
PersonView::MakeFocus(bool focus)
{
	if (focus && fControls.CountItems() > 0)
		fControls.ItemAt(0)->MakeFocus();
	else
		BView::MakeFocus(focus);
}


void
PersonView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SAVE:
			Save();
			break;

		case M_REVERT:
			if (fPictureView)
				fPictureView->Revert();

			for (int32 i = fControls.CountItems() - 1; i >= 0; i--)
				fControls.ItemAt(i)->Revert();
			break;

		case M_SELECT:
			for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
				BTextView* text = fControls.ItemAt(i)->TextView();
				if (text->IsFocus()) {
					text->Select(0, text->TextLength());
					break;
				}
			}
			break;

		case M_GROUP_MENU:
		{
			const char* name = NULL;
			if (msg->FindString("group", &name) == B_OK)
				SetAttribute(fCategoryAttribute, name, false);
			break;
		}

	}
}


void
PersonView::Draw(BRect updateRect)
{
	if (!fPictureView)
		return;

	// Draw a alert/get info-like strip
	BRect stripeRect = Bounds();
	stripeRect.right = GridLayout()->HorizontalSpacing()
		+ fPictureView->Bounds().Width() / 2;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);
}


void
PersonView::BuildGroupMenu()
{
	if (fGroups == NULL)
		return;

	BMenuItem* item;
	while ((item = fGroups->ItemAt(0)) != NULL) {
		fGroups->RemoveItem(item);
		delete item;
	}

	int32 count = 0;

	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		BQuery query;
		query.SetVolume(&volume);

		char buffer[256];
		snprintf(buffer, sizeof(buffer), "%s=*", fCategoryAttribute.String());
		query.SetPredicate(buffer);
		query.Fetch();

		BEntry entry;
		while (query.GetNextEntry(&entry) == B_OK) {
			BFile file(&entry, B_READ_ONLY);
			attr_info info;

			if (file.InitCheck() == B_OK
				&& file.GetAttrInfo(fCategoryAttribute, &info) == B_OK
				&& info.size > 1) {
				if (info.size > sizeof(buffer))
					info.size = sizeof(buffer);

				if (file.ReadAttr(fCategoryAttribute.String(), B_STRING_TYPE,
						0, buffer, info.size) < 0) {
					continue;
				}

				const char *text = buffer;
				while (true) {
					char* offset = strstr(text, ",");
					if (offset != NULL)
						offset[0] = '\0';

					if (!fGroups->FindItem(text)) {
						int32 index = 0;
						while ((item = fGroups->ItemAt(index)) != NULL) {
							if (strcmp(text, item->Label()) < 0)
								break;
							index++;
						}
						BMessage* message = new BMessage(M_GROUP_MENU);
						message->AddString("group", text);
						fGroups->AddItem(new BMenuItem(text, message), index);
						count++;
					}
					if (offset) {
						text = offset + 1;
						while (*text == ' ')
							text++;
					}
					else
						break;
				}
			}
		}
	}

	if (count == 0) {
		fGroups->AddItem(item = new BMenuItem(
			B_TRANSLATE_CONTEXT("none", "Groups list"),
			new BMessage(M_GROUP_MENU)));
		item->SetEnabled(false);
	}

	fGroups->SetTargetForItems(this);
}


void
PersonView::CreateFile(const entry_ref* ref)
{
	delete fFile;
	fFile = new BFile(ref, B_READ_WRITE);
	Save();
}


bool
PersonView::IsSaved() const
{
	if (fPictureView && fPictureView->HasChanged())
		return false;

	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		if (fControls.ItemAt(i)->HasChanged())
			return false;
	}

	return true;
}


void
PersonView::Save()
{
	fSaving = true;

	int32 count = fControls.CountItems();
	for (int32 i = 0; i < count; i++) {
		AttributeTextControl* control = fControls.ItemAt(i);
		const char* value = control->Text();
		fFile->WriteAttr(control->Attribute().String(), B_STRING_TYPE, 0,
			value, strlen(value) + 1);
		control->Update();
	}

	// Write the picture, if any, in the person file content
	if (fPictureView) {
		// Trim any previous content
		fFile->Seek(0, SEEK_SET);
		fFile->SetSize(0);

		BBitmap* picture = fPictureView->Bitmap();
		if (picture) {
			BBitmapStream stream(picture);
			// Detach *our* bitmap from stream to avoid its deletion
			// at stream object destruction
			stream.DetachBitmap(&picture);

			BTranslatorRoster* roster = BTranslatorRoster::Default();
			roster->Translate(&stream, NULL, NULL, fFile,
				fPictureView->SuggestedType(), B_TRANSLATOR_BITMAP,
				fPictureView->SuggestedMIMEType());

		}

		fPictureView->Update();
	}

	fFile->GetModificationTime(&fLastModificationTime);

	fSaving = false;
}


const char*
PersonView::AttributeValue(const char* attribute) const
{
	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		if (fControls.ItemAt(i)->Attribute() == attribute)
			return fControls.ItemAt(i)->Text();
	}

	return "";
}


void
PersonView::SetAttribute(const char* attribute, bool update)
{
	char* value = NULL;
	attr_info info;
	if (fFile != NULL && fFile->GetAttrInfo(attribute, &info) == B_OK) {
		value = (char*)calloc(info.size, 1);
		fFile->ReadAttr(attribute, B_STRING_TYPE, 0, value, info.size);
	}

	SetAttribute(attribute, value, update);

	free(value);
}


void
PersonView::SetAttribute(const char* attribute, const char* value,
	bool update)
{
	if (!LockLooper())
		return;

	AttributeTextControl* control = NULL;
	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		if (fControls.ItemAt(i)->Attribute() == attribute) {
			control = fControls.ItemAt(i);
			break;
		}
	}

	if (control == NULL)
		return;

	if (update) {
		control->SetText(value);
		control->Update();
	} else {
		BTextView* text = control->TextView();

		int32 start, end;
		text->GetSelection(&start, &end);
		if (start != end) {
			text->Delete();
			text->Insert(value);
		} else if ((end = text->TextLength())) {
			text->Select(end, end);
			text->Insert(",");
			text->Insert(value);
			text->Select(text->TextLength(), text->TextLength());
		} else
			control->SetText(value);
	}

	UnlockLooper();
}


void
PersonView::UpdatePicture(const entry_ref* ref)
{
	if (fPictureView == NULL)
		return;

	if (fSaving)
		return;

	time_t modificationTime = 0;
	BEntry entry(ref);
	entry.GetModificationTime(&modificationTime);

	if (entry.InitCheck() == B_OK
		&& modificationTime <= fLastModificationTime) {
		return;
	}

	fPictureView->Update(ref);
}


bool
PersonView::IsTextSelected() const
{
	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		BTextView* text = fControls.ItemAt(i)->TextView();

		int32 start, end;
		text->GetSelection(&start, &end);
		if (start != end)
			return true;
	}
	return false;
}

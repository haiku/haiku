//--------------------------------------------------------------------
//	
//	PeopleView.cpp
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PeopleView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs_attr.h>
#include <Box.h>
#include <ControlLook.h>
#include <GridLayout.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "TTextControl.h"


TPeopleView::TPeopleView(const char* name, entry_ref *ref)
	:
	BGridView()
{
	SetName(name);
	if (ref)
		fFile = new BFile(ref, O_RDWR);
	else
		fFile = NULL;
}


TPeopleView::~TPeopleView(void)
{
	delete fFile;
}


void
TPeopleView::AttachedToWindow(void)
{
	BGridLayout* layout = GridLayout();

	int32 row = 0;
	for (int32 i = 0; gFields[i].attribute; i++, row++) {
		const char *name = gFields[i].name;

		if (i == F_NAME)
			name = "Name";

		char *text = NULL;
		attr_info info;
		if (fFile && fFile->GetAttrInfo(gFields[i].attribute, &info) == B_OK) {
			text = (char *)calloc(info.size, 1);
			fFile->ReadAttr(gFields[i].attribute, B_STRING_TYPE,
				0, text, info.size);
		}

		fField[i] = new TTextControl(name, text);
		free(text);

		int32 labelColumn = 0;
		int32 textViewColumn = 1;
		int32 textViewWidth = 3;
		if (i == F_STATE)
			textViewWidth = 1;
		else if (i == F_ZIP) {
			row--;
			labelColumn = 2;
			textViewColumn = 3;
			textViewWidth = 1;
		}
		
		if (i != F_GROUP) {
			layout->AddItem(fField[i]->CreateLabelLayoutItem(),
				labelColumn, row, 1, 1);
			layout->AddItem(fField[i]->CreateTextViewLayoutItem(),
				textViewColumn, row, textViewWidth, 1);
		} else {
			fField[i]->SetLabel("");
			layout->AddView(fField[i], textViewColumn, row, textViewWidth, 1);
		}
	}

	fGroups = new BPopUpMenu(gFields[F_GROUP].name);
	fGroups->SetRadioMode(false);
	BMenuField *field = new BMenuField("", "", fGroups);
	field->SetEnabled(true);
	layout->AddView(field, 0, --row);

	float spacing = be_control_look->DefaultItemSpacing();
	layout->SetSpacing(spacing, spacing);
		// TODO: remove this after #5614 is fixed
	layout->SetInsets(spacing, spacing, spacing, spacing);
	fField[F_NAME]->MakeFocus();
}


void
TPeopleView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SAVE:
			Save();
			break;

		case M_REVERT:
			for (int32 loop = 0; loop < F_END; loop++)
				fField[loop]->Revert();
			break;

		case M_SELECT:
			for (int32 loop = 0; loop < F_END; loop++) {
				BTextView* text = fField[loop]->TextView();
				if (text->IsFocus()) {
					text->Select(0, text->TextLength());
					break;
				}
			}
			break;		
	}
}


void
TPeopleView::BuildGroupMenu(void)
{
	BMenuItem* item;
	while ((item = fGroups->ItemAt(0)) != NULL) {
		fGroups->RemoveItem(item);
		delete item;
	}

	const char *groupAttribute = gFields[F_GROUP].attribute;
	int32 count = 0;

	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		BQuery query;
		query.SetVolume(&volume);

		char buffer[256];
		sprintf(buffer, "%s=*", groupAttribute);
		query.SetPredicate(buffer);
		query.Fetch();

		BEntry entry;
		while (query.GetNextEntry(&entry) == B_OK) {
			BFile file(&entry, B_READ_ONLY);
			attr_info info;

			if (file.InitCheck() == B_OK
				&& file.GetAttrInfo(groupAttribute, &info) == B_OK
				&& info.size > 1) {
				if (info.size > sizeof(buffer))
					info.size = sizeof(buffer);

				if (file.ReadAttr(groupAttribute, B_STRING_TYPE, 0, buffer, info.size) < B_OK)
					continue;

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

	if (!count) {
		fGroups->AddItem(item = new BMenuItem("none", new BMessage(M_GROUP_MENU)));
		item->SetEnabled(false);
	}
}


bool
TPeopleView::CheckSave(void)
{
	for (int32 loop = 0; loop < F_END; loop++) {
		if (fField[loop]->Changed())
			return true;
	}

	return false;
}


const char*
TPeopleView::GetField(int32 index)
{
	if (index < F_END)
		return fField[index]->Text();

	return NULL;
}


void
TPeopleView::NewFile(entry_ref *ref)
{
	delete fFile;
	fFile = new BFile(ref, B_READ_WRITE);
	Save();
}


void
TPeopleView::Save(void)
{
	for (int32 i = 0; gFields[i].attribute; i++) {
		const char *text = fField[i]->Text();
		fFile->WriteAttr(gFields[i].attribute, B_STRING_TYPE, 0, text, strlen(text) + 1);
		fField[i]->Update();
	}
}


void
TPeopleView::SetField(int32 index, bool update)
{
	char *text = NULL;
	attr_info info;
	if (fFile && fFile->GetAttrInfo(gFields[index].attribute, &info) == B_OK) {
		text = (char *)calloc(info.size, 1);
		fFile->ReadAttr(gFields[index].attribute, B_STRING_TYPE, 0, text,
			info.size);
	}

	SetField(index, text, update);
	
	free(text);
}


void
TPeopleView::SetField(int32 index, char *data, bool update)
{
	Window()->Lock();

	if (update) {
		fField[index]->SetText(data);
		fField[index]->Update();
	} else {
		BTextView* text = fField[index]->TextView();

		int32 start, end;
		text->GetSelection(&start, &end);
		if (start != end) {
			text->Delete();
			text->Insert(data);
		} else if ((end = text->TextLength())) {
			text->Select(end, end);
			text->Insert(",");
			text->Insert(data);
			text->Select(text->TextLength(), text->TextLength());
		} else
			fField[index]->SetText(data);
	}

	Window()->Unlock();
}


bool
TPeopleView::TextSelected(void)
{
	for (int32 loop = 0; loop < F_END; loop++) {
		BTextView* text = fField[loop]->TextView();

		int32 start, end;
		text->GetSelection(&start, &end);
		if (start != end)
			return true;
	}
	return false;
}

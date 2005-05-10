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

#include <fs_attr.h>
#include <Window.h>
#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <VolumeRoster.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "PeopleView.h"
#include "TTextControl.h"

//====================================================================

TPeopleView::TPeopleView(BRect rect, char *title, entry_ref *ref)
		   :BView(rect, title, B_FOLLOW_ALL, B_WILL_DRAW)
{
	if (ref)
		fFile = new BFile(ref, O_RDWR);
	else
		fFile = NULL;
}

//--------------------------------------------------------------------

TPeopleView::~TPeopleView(void)
{
	if (fFile)
		delete fFile;
}

//--------------------------------------------------------------------

void TPeopleView::AttachedToWindow(void)
{
	char		*text;
	float		offset;
	BBox		*box;
	BFont		font = *be_plain_font;
	BMenuField	*field;
	BRect		r;
	attr_info	info;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	r = Bounds();
	r.InsetBy(-1, -1);
	box = new BBox(r);
	AddChild(box);

	offset = font.StringWidth(HPHONE_TEXT) + 18;

	text = (char *)malloc(1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(NAME_TEXT) - 11, NAME_V,
		  NAME_H + NAME_WIDTH, NAME_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_NAME, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_NAME, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_NAME] = new TTextControl(r, NAME_TEXT, 256,
								text, M_DIRTY, M_NAME);
	fField[F_NAME]->SetTarget(this);
	box->AddChild(fField[F_NAME]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(NICKNAME_TEXT) - 11, NICKNAME_V,
		  NICKNAME_H + NICKNAME_WIDTH, NICKNAME_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_NICKNAME, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_NICKNAME, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_NICKNAME] = new TTextControl(r, NICKNAME_TEXT, 256,
								text, M_DIRTY, M_NICKNAME);
	fField[F_NICKNAME]->SetTarget(this);
	box->AddChild(fField[F_NICKNAME]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(COMPANY_TEXT) - 11, COMPANY_V,
		  COMPANY_H + COMPANY_WIDTH, COMPANY_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_COMPANY, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_COMPANY, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_COMPANY] = new TTextControl(r, COMPANY_TEXT, 256,
								text, M_DIRTY, M_COMPANY);
	fField[F_COMPANY]->SetTarget(this);
	box->AddChild(fField[F_COMPANY]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(ADDRESS_TEXT) - 11, ADDRESS_V,
		  ADDRESS_H + ADDRESS_WIDTH, ADDRESS_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_ADDRESS, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_ADDRESS, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_ADDRESS] = new TTextControl(r, ADDRESS_TEXT, 256,
								text, M_DIRTY, M_ADDRESS);
	fField[F_ADDRESS]->SetTarget(this);
	box->AddChild(fField[F_ADDRESS]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(CITY_TEXT) - 11, CITY_V,
		  CITY_H + CITY_WIDTH, CITY_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_CITY, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_CITY, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_CITY] = new TTextControl(r, CITY_TEXT, 256,
								text, M_DIRTY, M_CITY);
	fField[F_CITY]->SetTarget(this);
	box->AddChild(fField[F_CITY]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(STATE_TEXT) - 11, STATE_V,
		  STATE_H + STATE_WIDTH, STATE_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_STATE, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_STATE, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_STATE] = new TTextControl(r, STATE_TEXT, 256,
								text, M_DIRTY, M_STATE);
	fField[F_STATE]->SetTarget(this);
	box->AddChild(fField[F_STATE]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(ZIP_H + 11, ZIP_V,
		  ZIP_H + ZIP_WIDTH, ZIP_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_ZIP, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_ZIP, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_ZIP] = new TTextControl(r, ZIP_TEXT, 256,
								text, M_DIRTY, M_ZIP);
	fField[F_ZIP]->SetTarget(this);
	box->AddChild(fField[F_ZIP]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(COUNTRY_TEXT) - 11, COUNTRY_V,
		  COUNTRY_H + COUNTRY_WIDTH, COUNTRY_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_COUNTRY, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_COUNTRY, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_COUNTRY] = new TTextControl(r, COUNTRY_TEXT, 256,
								text, M_DIRTY, M_COUNTRY);
	fField[F_COUNTRY]->SetTarget(this);
	box->AddChild(fField[F_COUNTRY]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(HPHONE_TEXT) - 11, HPHONE_V,
		  HPHONE_H + HPHONE_WIDTH, HPHONE_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_HPHONE, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_HPHONE, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_HPHONE] = new TTextControl(r, HPHONE_TEXT, 256,
								text, M_DIRTY, M_HPHONE);
	fField[F_HPHONE]->SetTarget(this);
	box->AddChild(fField[F_HPHONE]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(WPHONE_TEXT) - 11, WPHONE_V,
		  WPHONE_H + WPHONE_WIDTH, WPHONE_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_WPHONE, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_WPHONE, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_WPHONE] = new TTextControl(r, WPHONE_TEXT, 256,
								text, M_DIRTY, M_WPHONE);
	fField[F_WPHONE]->SetTarget(this);
	box->AddChild(fField[F_WPHONE]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(FAX_TEXT) - 11, FAX_V,
		  FAX_H + FAX_WIDTH, FAX_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_FAX, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_FAX, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_FAX] = new TTextControl(r, FAX_TEXT, 256,
								text, M_DIRTY, M_FAX);
	fField[F_FAX]->SetTarget(this);
	box->AddChild(fField[F_FAX]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(EMAIL_TEXT) - 11, EMAIL_V,
		  EMAIL_H + EMAIL_WIDTH, EMAIL_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_EMAIL, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_EMAIL, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_EMAIL] = new TTextControl(r, EMAIL_TEXT, 256,
								text, M_DIRTY, M_EMAIL);
	fField[F_EMAIL]->SetTarget(this);
	box->AddChild(fField[F_EMAIL]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - font.StringWidth(URL_TEXT) - 11, URL_V,
		  URL_H + URL_WIDTH, URL_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_URL, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_URL, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_URL] = new TTextControl(r, URL_TEXT, 256,
								text, M_DIRTY, M_URL);
	fField[F_URL]->SetTarget(this);
	box->AddChild(fField[F_URL]);

	text = (char *)realloc(text, 1);
	text[0] = 0;
	r.Set(offset - 11, GROUP_V,
		  GROUP_H + GROUP_WIDTH, GROUP_V + TEXT_HEIGHT);
	if (fFile) {
		if (fFile->GetAttrInfo(P_GROUP, &info) == B_NO_ERROR) {
			text = (char *)realloc(text, info.size);
			fFile->ReadAttr(P_GROUP, B_STRING_TYPE, 0, text, info.size);
		}
	}
	fField[F_GROUP] = new TTextControl(r, "", 256,
								text, M_DIRTY, M_GROUP);
	fField[F_GROUP]->SetTarget(this);
	box->AddChild(fField[F_GROUP]);
	free(text);

	r.right = r.left + 3;
	r.left = r.right - font.StringWidth(GROUP_TEXT) - 20;
	r.top -= 1;
	fGroups = new BPopUpMenu("Group");
	fGroups->SetRadioMode(FALSE);
	field = new BMenuField(r, "", "", fGroups);
	field->SetDivider(0.0);
	field->SetFont(&font);
	field->SetEnabled(TRUE);
	box->AddChild(field);

	fField[F_NAME]->MakeFocus();
}

//--------------------------------------------------------------------

void TPeopleView::MessageReceived(BMessage* msg)
{
	int32		loop;
	BTextView	*text;

	switch (msg->what) {
		case M_SAVE:
			Save();
			break;

		case M_REVERT:
			for (loop = 0; loop < F_END; loop++)
				fField[loop]->Revert();
			break;

		case M_SELECT:
			for (loop = 0; loop < F_END; loop++) {
				text = (BTextView *)fField[loop]->ChildAt(0);
				if (text->IsFocus()) {
					text->Select(0, text->TextLength());
					break;
				}
			}
			break;		
	}
}

//--------------------------------------------------------------------

void TPeopleView::BuildGroupMenu(void)
{
	char			*offset;
	char			str[256];
	char			*text;
	char			*text1;
	int32			count = 0;
	int32			index;
	BEntry			entry;
	BFile			file;
	BMessage		*msg;
	BMenuItem		*item;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;
	attr_info		info;

	while ((item = fGroups->ItemAt(0))) {
		fGroups->RemoveItem(item);
		delete item;
	}

	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	sprintf(str, "%s=*", P_GROUP);
	query.SetPredicate(str);
	query.Fetch();

	while (query.GetNextEntry(&entry) == B_NO_ERROR) {
		file.SetTo(&entry, O_RDONLY);
		if ((file.InitCheck() == B_NO_ERROR) &&
			(file.GetAttrInfo(P_GROUP, &info) == B_NO_ERROR) &&
			(info.size > 1)) {
			text = (char *)malloc(info.size);
			text1 = text;
			file.ReadAttr(P_GROUP, B_STRING_TYPE, 0, text, info.size);
			while (1) {
				if ((offset = strstr(text, ",")))
					*offset = 0;
				if (!fGroups->FindItem(text)) {
					index = 0;
					while ((item = fGroups->ItemAt(index))) {
						if (strcmp(text, item->Label()) < 0)
							break;
						index++;
					}
					msg = new BMessage(M_GROUP_MENU);
					msg->AddString("group", text);
					fGroups->AddItem(new BMenuItem(text, msg), index);
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
			free(text1);
		}
	}

	if (!count) {
		fGroups->AddItem(item = new BMenuItem("none", new BMessage(M_GROUP_MENU)));
		item->SetEnabled(FALSE);
	}
}

//--------------------------------------------------------------------

bool TPeopleView::CheckSave(void)
{
	int32	loop;

	for (loop = 0; loop < F_END; loop++)
		if (fField[loop]->Changed())
			return TRUE;
	return FALSE;
}

//--------------------------------------------------------------------

const char* TPeopleView::GetField(int32 index)
{
	if (index < F_END)
		return fField[index]->Text();
	else
		return NULL;
}

//--------------------------------------------------------------------

void TPeopleView::NewFile(entry_ref *ref)
{
	if (fFile)
		delete fFile;
	fFile = new BFile(ref, O_RDWR);
	Save();
}

//--------------------------------------------------------------------

void TPeopleView::Save(void)
{
	const char	*text;

	text = fField[F_NAME]->Text();
	fFile->WriteAttr(P_NAME, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_NAME]->Update();

	text = fField[F_COMPANY]->Text();
	fFile->WriteAttr(P_COMPANY, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_COMPANY]->Update();

	text = fField[F_ADDRESS]->Text();
	fFile->WriteAttr(P_ADDRESS, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_ADDRESS]->Update();

	text = fField[F_CITY]->Text();
	fFile->WriteAttr(P_CITY, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_CITY]->Update();

	text = fField[F_STATE]->Text();
	fFile->WriteAttr(P_STATE, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_STATE]->Update();

	text = fField[F_ZIP]->Text();
	fFile->WriteAttr(P_ZIP, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_ZIP]->Update();

	text = fField[F_COUNTRY]->Text();
	fFile->WriteAttr(P_COUNTRY, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_COUNTRY]->Update();

	text = fField[F_HPHONE]->Text();
	fFile->WriteAttr(P_HPHONE, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_HPHONE]->Update();

	text = fField[F_WPHONE]->Text();
	fFile->WriteAttr(P_WPHONE, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_WPHONE]->Update();

	text = fField[F_FAX]->Text();
	fFile->WriteAttr(P_FAX, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_FAX]->Update();

	text = fField[F_EMAIL]->Text();
	fFile->WriteAttr(P_EMAIL, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_EMAIL]->Update();

	text = fField[F_URL]->Text();
	fFile->WriteAttr(P_URL, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_URL]->Update();

	text = fField[F_GROUP]->Text();
	fFile->WriteAttr(P_GROUP, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_GROUP]->Update();

	text = fField[F_NICKNAME]->Text();
	fFile->WriteAttr(P_NICKNAME, B_STRING_TYPE, 0, text, strlen(text) + 1);
	fField[F_NICKNAME]->Update();
}

//--------------------------------------------------------------------

void TPeopleView::SetField(int32 index, char *data, bool update)
{
	int32		end;
	int32		start;
	BTextView	*text;

	Window()->Lock();
	if (update) {
		fField[index]->SetText(data);
		fField[index]->Update();
	}
	else {
		text = (BTextView *)fField[index]->ChildAt(0);
		text->GetSelection(&start, &end);
		if (start != end) {
			text->Delete();
			text->Insert(data);
		}
		else if ((end = text->TextLength())) {
			text->Select(end, end);
			text->Insert(",");
			text->Insert(data);
			text->Select(text->TextLength(), text->TextLength());
		}
		else
			fField[index]->SetText(data);
	}
	Window()->Unlock();
}

//--------------------------------------------------------------------

bool TPeopleView::TextSelected(void)
{
	int32		end;
	int32		loop;
	int32		start;
	BTextView	*text;

	for (loop = 0; loop < F_END; loop++) {
		text = (BTextView *)fField[loop]->ChildAt(0);
		text->GetSelection(&start, &end);
		if (start != end)
			return TRUE;
	}
	return FALSE;
}

/*
 * Copyright 2003-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 *		Atsushi Takamatsu
 */
#include "HEventList.h"

#include <Alert.h>
#include <Catalog.h>
#include <ColumnTypes.h>
#include <Entry.h>
#include <Locale.h>
#include <MediaFiles.h>
#include <Path.h>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HEventList"


HEventRow::HEventRow(const char* name, const char* path)
	:
	BRow(),
	fName(name)
{
	SetField(new BStringField(name), kEventColumn);
	SetPath(path);
}


HEventRow::~HEventRow()
{
}


void
HEventRow::SetPath(const char* _path)
{
	fPath = _path;
	BPath path(_path);
	SetField(new BStringField(_path ? path.Leaf() : B_TRANSLATE("<none>")),
		kSoundColumn);
}


void
HEventRow::Remove(const char* type)
{
	BMediaFiles().RemoveItem(type, Name());
}


HEventList::HEventList(const char* name)
	:
	BColumnListView(name, 0, B_PLAIN_BORDER, true),
	fType(NULL)
{
	AddColumn(new BStringColumn(B_TRANSLATE("Event"), 180, 50, 500,
		B_TRUNCATE_MIDDLE), kEventColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Sound"), 130, 50, 500,
		B_TRUNCATE_END), kSoundColumn);
}


HEventList::~HEventList()
{
	RemoveAll();
	delete fType;
}


void
HEventList::SetType(const char* type)
{
	RemoveAll();
	BMediaFiles mfiles;
	mfiles.RewindRefs(type);
	delete fType;
	fType = strdup(type);

	BString name;
	entry_ref ref;
	while (mfiles.GetNextRef(&name,&ref) == B_OK) {
		BPath path(&ref);
		if (path.InitCheck() != B_OK || ref.name == NULL
			|| strcmp(ref.name, "") == 0)
			AddRow(new HEventRow(name.String(), NULL));
		else
			AddRow(new HEventRow(name.String(), path.Path()));
	}
}


void
HEventList::RemoveAll()
{
	BRow* row;
	while ((row = RowAt((int32)0, NULL)) != NULL) {
		RemoveRow(row);
		delete row;
	}
}


void
HEventList::SelectionChanged()
{
	BColumnListView::SelectionChanged();

	HEventRow* row = (HEventRow*)CurrentSelection();
	if (row != NULL) {
		entry_ref ref;
		BMediaFiles().GetRefFor(fType, row->Name(), &ref);

		BPath path(&ref);
		if (path.InitCheck() == B_OK || ref.name == NULL
			|| strcmp(ref.name, "") == 0) {
			row->SetPath(path.Path());
			UpdateRow(row);
		} else {
			printf("name %s\n", ref.name);
			BMediaFiles().RemoveRefFor(fType, row->Name(), ref);
			BAlert* alert = new BAlert("alert",
				B_TRANSLATE("No such file or directory"), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			return;
		}
		BMessage msg(M_EVENT_CHANGED);
		msg.AddString("name", row->Name());
		msg.AddString("path", row->Path());
		Window()->PostMessage(&msg);
	}
}


void
HEventList::SetPath(const char* path)
{
	HEventRow* row = (HEventRow*)CurrentSelection();
	if (row != NULL) {
		entry_ref ref;
		BEntry entry(path);
		entry.GetRef(&ref);
		BMediaFiles().SetRefFor(fType, row->Name(), ref);

		row->SetPath(path);
		UpdateRow(row);
	}
}

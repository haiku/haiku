/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 */


#include "PropertyList.h"

#include <Catalog.h>
#include <Clipboard.h>
#include <ColumnTypes.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PropertyList"


PropertyRow::PropertyRow(const char* name, const char* value)
	: BRow(),
	fName(name), fValue(value)
{
	SetField(new BStringField(name), kNameColumn);
	SetField(new BStringField(value), kValueColumn);
}


PropertyRow::~PropertyRow()
{
}


void
PropertyRow::SetName(const char* name)
{
	fName = name;
	SetField(new BStringField(name), kNameColumn);
}


void
PropertyRow::SetValue(const char* value)
{
	fValue = value;
	SetField(new BStringField(value), kValueColumn);
}


PropertyList::PropertyList(const char* name)
	: BColumnListView(BRect(0.0, 0.0, 1.0, 1.0), name, B_FOLLOW_ALL, 0,
		B_NO_BORDER, true)
{
	BStringColumn* nameColumn;
	AddColumn(nameColumn = new BStringColumn(B_TRANSLATE("Name"), 150, 50, 500,
			B_TRUNCATE_MIDDLE),
		kNameColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Value"), 150, 50, 500,
		B_TRUNCATE_END), kValueColumn);
	SetSortColumn(nameColumn, false, true);
}


PropertyList::~PropertyList()
{
	RemoveAll();
}


void
PropertyList::AddAttributes(const Attributes& attributes)
{
	RemoveAll();
	for (unsigned int i = 0; i < attributes.size(); i++) {
		AddRow(new PropertyRow(attributes[i].fName, attributes[i].fValue));
	}
}


void
PropertyList::RemoveAll()
{
	BRow *row;
	while ((row = RowAt((int32)0, NULL))!=NULL) {
		RemoveRow(row);
		delete row;
	}
}


void
PropertyList::SelectionChanged()
{
}


void
PropertyList::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_COPY:
		{
			BString strings;
			uint32 rowsCount = CountRows();

			for (uint32 i = 0; i < rowsCount; i++) {
				PropertyRow* current = (PropertyRow*)RowAt(i);
				if (current->IsSelected())
					strings << current->Name()
						<< "\t" << current->Value() << "\n";
			}

			if (!be_clipboard->Lock())
				break;

			be_clipboard->Clear();
			BMessage* data = be_clipboard->Data();
			if (data != NULL) {
				data->AddData("text/plain", B_MIME_TYPE,
						strings, strings.Length());
				be_clipboard->Commit();
			}

			be_clipboard->Unlock();
			break;
		}
		default:
			BColumnListView::MessageReceived(msg);
			break;
	}
}

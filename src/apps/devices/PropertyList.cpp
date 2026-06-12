/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 */


#include "PropertyList.h"

#include <HashMap.h>
#include <HashString.h>
#include <set>
#include <string.h>

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
	AddColumn(new BStringColumn(B_TRANSLATE("Value"), 400, 100, 1000,
		B_TRUNCATE_END), kValueColumn);
}


PropertyList::~PropertyList()
{
	RemoveAll();
}


static BString
InsertNode(HashMap<HashString, std::set<BString> >& tree, const BString& path,
	const BString& part)
{
	std::set<BString>* children = NULL;
	if (!tree.Get(path.String(), children)) {
		tree.Put(path.String(), std::set<BString>());
		tree.Get(path.String(), children);
	}
	if (children != NULL)
		children->insert(part.String());

	BString nextPath = path;
	nextPath << "/" << part;
	return nextPath;
}


static void
UpdateTree(const BString& name, const BString& value,
	HashMap<HashString, std::set<BString> >& tree, HashMap<HashString, BString>& values)
{
	int32 start = 0;
	int32 nextSlash;
	BString currentPath = "";
	while ((nextSlash = name.FindFirst('/', start)) != B_ERROR) {
		BString part;
		name.CopyInto(part, start, nextSlash - start);
		currentPath = InsertNode(tree, currentPath, part);
		start = nextSlash + 1;
	}
	BString leaf;
	name.CopyInto(leaf, start, name.Length() - start);
	BString fullPath = InsertNode(tree, currentPath, leaf);
	values.Put(fullPath.String(), value);
}


void
AddCollapsedRows(PropertyList* list, PropertyRow* parent, BString path,
	HashMap<HashString, std::set<BString> >& tree, HashMap<HashString, BString>& values)
{
	std::set<BString>* children = NULL;
	if (!tree.Get(path.String(), children))
		return;

	for (std::set<BString>::iterator it = children->begin(); it != children->end(); ++it) {
		BString childName = *it;
		BString fullPath = path;
		fullPath << "/" << childName;

		BString displayName = childName;
		BString currentPath = fullPath;

		std::set<BString>* subChildren = NULL;
		while (tree.Get(currentPath.String(), subChildren) && subChildren->size() == 1
			&& !values.ContainsKey(currentPath.String())) {
			BString nextChild = *subChildren->begin();
			displayName << "/" << nextChild;
			currentPath << "/" << nextChild;
		}

		BString value = "";
		if (values.ContainsKey(currentPath.String()))
			value = values.Get(currentPath.String());

		PropertyRow* newRow = new PropertyRow(displayName.String(), value.String());
		list->AddRow(newRow, parent);

		if (tree.ContainsKey(currentPath.String()))
			AddCollapsedRows(list, newRow, currentPath, tree, values);
	}
}


void
PropertyList::AddAttributes(const Attributes& attributes)
{
	RemoveAll();

	PropertyRow* basicRoot = new PropertyRow(B_TRANSLATE("Basic information"), "");
	PropertyRow* advancedRoot = new PropertyRow(B_TRANSLATE("Attributes"), "");

	AddRow(basicRoot);
	AddRow(advancedRoot);

	HashMap<HashString, std::set<BString> > tree;
	HashMap<HashString, BString> values;

	for (uint32 i = 0; i < attributes.size(); i++) {
		BString name = attributes[i].fName;
		BString value = attributes[i].fValue;

		if (name.FindFirst('/') != B_ERROR)
			UpdateTree(name, value, tree, values);
		else
			AddRow(new PropertyRow(name.String(), value.String()), basicRoot);
	}

	AddCollapsedRows(this, advancedRoot, "", tree, values);

	ExpandOrCollapse(basicRoot, true);
	ExpandOrCollapse(advancedRoot, true);
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

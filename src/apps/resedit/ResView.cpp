/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "ResView.h"

#include <Application.h>
#include <ScrollView.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <stdio.h>
#include <ColumnTypes.h>

#include "App.h"
#include "ResourceData.h"
#include "ResFields.h"
#include "ResWindow.h"
#include "PreviewColumn.h"
#include "Editor.h"

static int32 sUntitled = 1;

ResourceRoster gResRoster;

enum {
	M_NEW_FILE = 'nwfl',
	M_OPEN_FILE,
	M_SAVE_FILE,
	M_SAVE_FILE_AS,
	M_QUIT,
	M_EDIT_RESOURCE
};

ResView::ResView(const BRect &frame, const char *name, const int32 &resize,
				const int32 &flags, const entry_ref *ref)
  :	BView(frame, name, resize, flags),
  	fIsDirty(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	if (ref) {
		fRef = new entry_ref;
		*fRef = *ref;
		fFileName = fRef->name;
	} else {
		fRef = NULL;
		fFileName = "Untitled ";
		fFileName << sUntitled;
		sUntitled++;
	}
	
	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New…", new BMessage(M_NEW_FILE), 'N'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Open…", new BMessage(M_OPEN_FILE), 'O'));
	menu->AddItem(new BMenuItem("Quit", new BMessage(M_QUIT), 'Q'));
	
	BRect r(Bounds());
	r.bottom = 16;
	fBar = new BMenuBar(r, "bar");
	AddChild(fBar);
	
	fBar->AddItem(menu);
	
	r = Bounds();
	r.top = fBar->Frame().bottom + 4;
	fListView = new BColumnListView(r, "gridview", B_FOLLOW_ALL, B_WILL_DRAW,
									B_FANCY_BORDER);
	AddChild(fListView);
	
	rgb_color white = { 255,255,255,255 };
	fListView->SetColor(B_COLOR_BACKGROUND, white);
	fListView->SetColor(B_COLOR_SELECTION, ui_color(B_MENU_BACKGROUND_COLOR));
	
	float width = be_plain_font->StringWidth("00000") + 20;
	fListView->AddColumn(new BStringColumn("ID",width,width,100,B_TRUNCATE_END),0);
	
	fListView->AddColumn(new BStringColumn("Type",width,width,100,B_TRUNCATE_END),1);
	fListView->AddColumn(new BStringColumn("Name",150,50,300,B_TRUNCATE_END),2);
	fListView->AddColumn(new PreviewColumn("Data",150,50,300),3);
	fListView->SetInvocationMessage(new BMessage(M_EDIT_RESOURCE));
	
	width = be_plain_font->StringWidth("1000 bytes") + 20;
	fListView->AddColumn(new BSizeColumn("Size",width,10,100),4);
	
	if (ref)
		OpenFile(*ref);
}


ResView::~ResView(void)
{
	EmptyDataList();
	delete fRef;
}


void
ResView::AttachedToWindow(void)
{
	for(int32 i = 0; i < fBar->CountItems(); i++)
		fBar->SubmenuAt(i)->SetTargetForItems(this);
	fListView->SetTarget(this);
}


void
ResView::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		be_app->PostMessage(msg);
		return;
	}
	
	switch (msg->what) {
		case M_NEW_FILE: {
			BRect r(100,100,400,400);
			if (Window())
				r = Window()->Frame().OffsetByCopy(10,10);
			ResWindow *win = new ResWindow(r);
			win->Show();
			break;
		}
		case M_OPEN_FILE: {
			be_app->PostMessage(M_SHOW_OPEN_PANEL);
			break;
		}
		case M_QUIT: {
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_EDIT_RESOURCE: {
			BRow *row = fListView->CurrentSelection();
			TypeCodeField *field = (TypeCodeField*)row->GetField(1);
			gResRoster.SpawnEditor(field->GetResourceData(),this);
			break;
		}
		case M_UPDATE_RESOURCE: {
			ResourceData *item;
			if (msg->FindPointer("item",(void **)&item) != B_OK)
				break;
			
			for (int32 i = 0; i < fListView->CountRows(); i++) {
				BRow *row = fListView->RowAt(i);
				TypeCodeField *field = (TypeCodeField*)row->GetField(1);
				if (!field || field->GetResourceData() != item)
					continue;
				
				UpdateRow(row);
				break;
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
ResView::OpenFile(const entry_ref &ref)
{
	// Add all the 133t resources and attributes of the file
	
	BFile file(&ref, B_READ_ONLY);
	BResources resources;
	if (resources.SetTo(&file) != B_OK)
		return;
	file.Unset();
	
	resources.PreloadResourceType();
	
	int32 index = 0;
	BRow *row;
	ResourceData *resData = new ResourceData();
	while (resData->SetFromResource(index, resources)) {
		row = new BRow();
		row->SetField(new BStringField(resData->GetIDString()),0);
		row->SetField(new TypeCodeField(resData->GetType(),resData),1);
		row->SetField(new BStringField(resData->GetName()),2);
		BField *field = gResRoster.MakeFieldForType(resData->GetType(),
													resData->GetData(),
													resData->GetLength());
		if (field)
			row->SetField(field,3);
		row->SetField(new BSizeField(resData->GetLength()),4);
		fListView->AddRow(row);
		fDataList.AddItem(resData);
		resData = new ResourceData();
		index++;
	}
	delete resData;

	BNode node;
	if (node.SetTo(&ref) == B_OK) {
		char attrName[B_ATTR_NAME_LENGTH];
		node.RewindAttrs();
		resData = new ResourceData();
		while (node.GetNextAttrName(attrName) == B_OK) {
			if (resData->SetFromAttribute(attrName, node)) {
				row = new BRow();
				row->SetField(new BStringField(resData->GetIDString()),0);
				row->SetField(new TypeCodeField(resData->GetType(),resData),1);
				row->SetField(new BStringField(resData->GetName()),2);
				BField *field = gResRoster.MakeFieldForType(resData->GetType(),
															resData->GetData(),
															resData->GetLength());
				if (field)
					row->SetField(field,3);
				row->SetField(new BSizeField(resData->GetLength()),4);
				fListView->AddRow(row);
				fDataList.AddItem(resData);
				resData = new ResourceData();
			}
		}
		delete resData;
	}
}


void
ResView::EmptyDataList(void)
{
	for (int32 i = 0; i < fDataList.CountItems(); i++) {
		ResourceData *data = (ResourceData*) fDataList.ItemAt(i);
		delete data;
	}
	fDataList.MakeEmpty();
}


void
ResView::UpdateRow(BRow *row)
{
	TypeCodeField *typeField = (TypeCodeField*) row->GetField(1);
	ResourceData *resData = typeField->GetResourceData();
	BStringField *strField = (BStringField *)row->GetField(0);
	
	if (strcmp("(attr)", strField->String()) != 0)
		strField->SetString(resData->GetIDString());
	
	strField = (BStringField *)row->GetField(2);
	strField->SetString(resData->GetName());
	
	PreviewField *preField = (PreviewField*)row->GetField(3);
	preField->SetData(resData->GetData(), resData->GetLength());
	
	BSizeField *sizeField = (BSizeField*)row->GetField(4);
	sizeField->SetSize(resData->GetLength());
}

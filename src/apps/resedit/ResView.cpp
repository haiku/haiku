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
#include "ResWindow.h"
#include "PreviewColumn.h"

static int32 sUntitled = 1;

ResourceRoster gResRoster;

enum {
	M_NEW_FILE = 'nwfl',
	M_OPEN_FILE,
	M_SAVE_FILE,
	M_SAVE_FILE_AS,
	M_QUIT
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
	
	width = be_plain_font->StringWidth("1000 bytes") + 20;
	fListView->AddColumn(new BSizeColumn("Size",width,10,100),4);
	
	if (ref)
		OpenFile(*ref);
}


ResView::~ResView(void)
{
	delete fRef;
}


void
ResView::AttachedToWindow(void)
{
	for(int32 i = 0; i < fBar->CountItems(); i++)
		fBar->SubmenuAt(i)->SetTargetForItems(this);
}


void
ResView::MessageReceived(BMessage *msg)
{
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
		default:
			BView::MessageReceived(msg);
	}
}


void
ResView::OpenFile(const entry_ref &ref)
{
	// Add all the 133t resources and attributes of the file
	
	BFile file(&ref, B_READ_ONLY);
	if (fResources.SetTo(&file) != B_OK)
		return;
	file.Unset();
	
	fResources.PreloadResourceType();
	
	int32 index = 0;
	ResourceData data;
	BRow *row;
	while (data.SetFromResource(index, fResources)) {
		row = new BRow();
		row->SetField(new BStringField(data.GetIDString()),0);
		row->SetField(new BStringField(data.GetTypeString()),1);
		row->SetField(new BStringField(data.GetName()),2);
		BField *field = gResRoster.MakeFieldForType(data.GetType(),
													data.GetData(),
													data.GetLength());
		if (field)
			row->SetField(field,3);
		row->SetField(new BSizeField(data.GetLength()),4);
		fListView->AddRow(row);
		index++;
	}

//debugger("");	
	BNode node;
	if (node.SetTo(&ref) == B_OK) {
		char attrName[B_ATTR_NAME_LENGTH];
		node.RewindAttrs();
		while (node.GetNextAttrName(attrName) == B_OK) {
			if (data.SetFromAttribute(attrName, node)) {
				row = new BRow();
				row->SetField(new BStringField(data.GetIDString()),0);
				row->SetField(new BStringField(data.GetTypeString()),1);
				row->SetField(new BStringField(data.GetName()),2);
				BField *field = gResRoster.MakeFieldForType(data.GetType(),
															data.GetData(),
															data.GetLength());
				if (field)
					row->SetField(field,3);
				row->SetField(new BSizeField(data.GetLength()),4);
				fListView->AddRow(row);
			}
		}
	}
}


const void *
ResView::LoadResource(const type_code &type, const int32 &id, size_t *out_size)
{
	return fResources.LoadResource(type, id, out_size);
}


const void *
ResView::LoadResource(const type_code &type, const char *name, size_t *out_size)
{
	return fResources.LoadResource(type, name, out_size);
}


status_t
ResView::AddResource(const type_code &type, const int32 &id, const void *data,
					const size_t &data_size, const char *name)
{
	return fResources.AddResource(type, id, data, data_size, name);
}


bool
ResView::HasResource(const type_code &type, const int32 &id)
{
	return fResources.HasResource(type, id);
}


bool
ResView::HasResource(const type_code &type, const char *name)
{
	return fResources.HasResource(type, name);
}


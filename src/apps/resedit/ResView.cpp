/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResView.h"

#include <Application.h>
#include <File.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <TranslatorRoster.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <stdlib.h>

#include "App.h"
#include "ColumnTypes.h"
#include "ResourceData.h"
#include "ResFields.h"
#include "ResListView.h"
#include "ResWindow.h"
#include "PreviewColumn.h"
#include "Editor.h"

static int32 sUntitled = 1;

ResourceRoster gResRoster;

enum {
	M_NEW_FILE = 'nwfl',
	M_OPEN_FILE,
	M_SAVE_FILE,
	M_QUIT,
	M_SELECT_FILE,
	M_DELETE_RESOURCE,
	M_EDIT_RESOURCE
};

ResView::ResView(const BRect &frame, const char *name, const int32 &resize,
				const int32 &flags, const entry_ref *ref)
  :	BView(frame, name, resize, flags),
  	fRef(NULL),
	fSaveStatus(FILE_INIT),
  	fOpenPanel(NULL),
  	fSavePanel(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	if (ref) {
		fRef = new entry_ref;
		*fRef = *ref;
		fFileName = fRef->name;
	} else {
		fFileName = "Untitled ";
		fFileName << sUntitled;
		sUntitled++;
	}
	
	BRect r(Bounds());
	r.bottom = 16;
	fBar = new BMenuBar(r, "bar");
	AddChild(fBar);
	
	BuildMenus(fBar);
	
	r = Bounds();
	r.top = fBar->Frame().bottom + 4;
	fListView = new ResListView(r, "gridview", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	AddChild(fListView);
	
	float width = be_plain_font->StringWidth("00000") + 20;
	fListView->AddColumn(new BStringColumn("ID", width, width, 100, B_TRUNCATE_END), 0);
	
	fListView->AddColumn(new BStringColumn("Type", width, width, 100, B_TRUNCATE_END), 1);
	fListView->AddColumn(new BStringColumn("Name", 150, 50, 300, B_TRUNCATE_END), 2);
	fListView->AddColumn(new PreviewColumn("Data", 150, 50, 300), 3);
	
	// Editing is disabled for now
	fListView->SetInvocationMessage(new BMessage(M_EDIT_RESOURCE));
	
	width = be_plain_font->StringWidth("1000 bytes") + 20;
	fListView->AddColumn(new BSizeColumn("Size", width, 10, 100), 4);
	
	fOpenPanel = new BFilePanel(B_OPEN_PANEL);
	if (ref)
		OpenFile(*ref);
	
	fSavePanel = new BFilePanel(B_SAVE_PANEL);
}


ResView::~ResView(void)
{
	EmptyDataList();
	delete fRef;
	delete fOpenPanel;
	delete fSavePanel;
}


void
ResView::AttachedToWindow(void)
{
	for (int32 i = 0; i < fBar->CountItems(); i++)
		fBar->SubmenuAt(i)->SetTargetForItems(this);
	fListView->SetTarget(this);
	
	BMessenger messenger(this);
	fOpenPanel->SetTarget(messenger);
	fSavePanel->SetTarget(messenger);
	
	Window()->Lock();
	BString title("ResEdit: ");
	title << fFileName;
	Window()->SetTitle(title.String());
	Window()->Unlock();
}


void
ResView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_NEW_FILE: {
			BRect r(100, 100, 400, 400);
			if (Window())
				r = Window()->Frame().OffsetByCopy(10, 10);
			ResWindow *win = new ResWindow(r);
			win->Show();
			break;
		}
		case M_OPEN_FILE: {
			be_app->PostMessage(M_SHOW_OPEN_PANEL);
			break;
		}
		case B_CANCEL: {
			if (fSaveStatus == FILE_QUIT_AFTER_SAVE)
				SetSaveStatus(FILE_DIRTY);
			break;
		}
		case B_SAVE_REQUESTED: {
			entry_ref saveDir;
			BString name;
			if (msg->FindRef("directory",&saveDir) == B_OK &&
				msg->FindString("name",&name) == B_OK) {
				SetTo(saveDir,name);
				SaveFile();
			}
			break;
		}
		case M_SAVE_FILE: {
			if (!fRef)
				fSavePanel->Show();
			else
				SaveFile();
			break;
		}
		case M_SHOW_SAVE_PANEL: {
			fSavePanel->Show();
			break;
		}
		case M_QUIT: {
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case B_REFS_RECEIVED: {
			int32 i = 0;
			entry_ref ref;
			while (msg->FindRef("refs", i++, &ref) == B_OK)
				AddResource(ref);
			break;
		}
		case M_SELECT_FILE: {
			fOpenPanel->Show();
			break;
		}
		case M_DELETE_RESOURCE: {
			DeleteSelectedResources();
			break;
		}
		case M_EDIT_RESOURCE: {
			BRow *row = fListView->CurrentSelection();
			TypeCodeField *field = (TypeCodeField*)row->GetField(1);
			gResRoster.SpawnEditor(field->GetResourceData(), this);
			break;
		}
		case M_UPDATE_RESOURCE: {
			ResourceData *item;
			if (msg->FindPointer("item", (void **)&item) != B_OK)
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


status_t
ResView::SetTo(const entry_ref &dir, const BString &name)
{
	entry_ref fileRef;
	
	BPath path(&dir);
	path.Append(name.String());
	BFile file(path.Path(), B_CREATE_FILE | B_READ_WRITE);
	if (file.InitCheck() != B_OK)
		return B_ERROR;
	
	if (!fRef)
		fRef = new entry_ref();
	
	BEntry entry(path.Path());
	entry.GetRef(fRef);
	fFileName = name;
	return B_OK;
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
	ResDataRow *row;
	ResourceData *resData = new ResourceData();
	while (resData->SetFromResource(index, resources)) {
		row = new ResDataRow(resData);
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
				row = new ResDataRow(resData);
				fListView->AddRow(row);
				fDataList.AddItem(resData);
				resData = new ResourceData();
			}
		}
		delete resData;
	}
}


void
ResView::SaveFile(void)
{
	if (fSaveStatus == FILE_CLEAN || !fRef)
		return;
	
	BFile file(fRef,B_READ_WRITE);
	BResources res(&file,true);
	file.Unset();
	
	for (int32 i = 0; i < fListView->CountRows(); i++) {
		ResDataRow *row = (ResDataRow*)fListView->RowAt(i);
		ResourceData *data = row->GetData();
		res.AddResource(data->GetType(), data->GetID(), data->GetData(),
						data->GetLength(), data->GetName());
	}
	
	res.Sync();
	
	if (fSaveStatus == FILE_QUIT_AFTER_SAVE && Window())
		Window()->PostMessage(B_QUIT_REQUESTED);
	SetSaveStatus(FILE_CLEAN);
}


void
ResView::SaveAndQuit(void)
{
	SetSaveStatus(FILE_QUIT_AFTER_SAVE);
	if (!fRef) {
		fSavePanel->Show();
		return;
	}
	
	SaveFile();
}


void
ResView::BuildMenus(BMenuBar *menuBar)
{
	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New" B_UTF8_ELLIPSIS, new BMessage(M_NEW_FILE), 'N'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS, new BMessage(M_OPEN_FILE), 'O'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Save", new BMessage(M_SAVE_FILE), 'S'));
	menu->AddItem(new BMenuItem("Save As" B_UTF8_ELLIPSIS, new BMessage(M_SHOW_SAVE_PANEL), 'S',
								B_COMMAND_KEY | B_SHIFT_KEY));
	menuBar->AddItem(menu);
	
	menu = new BMenu("Resource");
	menu->AddItem(new BMenuItem("Add" B_UTF8_ELLIPSIS, new BMessage(M_SELECT_FILE), 'F'));
	menu->AddItem(new BMenuItem("Delete", new BMessage(M_DELETE_RESOURCE), 'D'));
	menuBar->AddItem(menu);
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
	if (strField)
		strField->SetString(resData->GetName());
	
	PreviewField *preField = (PreviewField*)row->GetField(3);
	if (preField)
		preField->SetData(resData->GetData(), resData->GetLength());
	
	BSizeField *sizeField = (BSizeField*)row->GetField(4);
	if (sizeField)
		sizeField->SetSize(resData->GetLength());
}


void
ResView::AddResource(const entry_ref &ref)
{
	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;
	
	BString mime;
	file.ReadAttrString("BEOS:TYPE", &mime);
	
	if (mime == "application/x-be-resource") {
		BMessage msg(B_REFS_RECEIVED);
		msg.AddRef("refs", &ref);
		be_app->PostMessage(&msg);
		return;
	}
	
	type_code fileType = 0;
	
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	translator_info info;
	if (roster->Identify(&file, NULL, &info, 0, mime.String()) == B_OK)
		fileType = info.type;
	else
		fileType = B_RAW_TYPE;
	
	int32 lastID = -1;
	for (int32 i = 0; i < fDataList.CountItems(); i++) {
		ResourceData *resData = (ResourceData*)fDataList.ItemAt(i);
		if (resData->GetType() == fileType && resData->GetID() > lastID)
			lastID = resData->GetID();
	}
	
	off_t fileSize;
	file.GetSize(&fileSize);
	
	if (fileSize < 1)
		return;
	
	char *fileData = (char *)malloc(fileSize);
	file.Read(fileData, fileSize);
	
	ResourceData *resData = new ResourceData(fileType, lastID + 1, ref.name,
											fileData, fileSize);
	fDataList.AddItem(resData);
	
	ResDataRow *row = new ResDataRow(resData);
	fListView->AddRow(row);
	
	SetSaveStatus(FILE_DIRTY);
}


void
ResView::DeleteSelectedResources(void)
{
	ResDataRow *selection = (ResDataRow*)fListView->CurrentSelection();
	if (!selection)
		return;
	
	SetSaveStatus(FILE_DIRTY);
	
	while (selection) {
		ResourceData *data = selection->GetData();
		fListView->RemoveRow(selection);
		fDataList.RemoveItem(data);
		delete data;
		selection = (ResDataRow*)fListView->CurrentSelection();
	}
}


void
ResView::SetSaveStatus(uint8 value)
{
	if (value == fSaveStatus)
		return;
	
	fSaveStatus = value;
	
	BString title("ResEdit: ");
	title << fFileName;
	if (fSaveStatus == FILE_DIRTY)
		title << "*";
	
	if (Window()) {
		Window()->Lock();
		Window()->SetTitle(title.String());
		Window()->Unlock();
	}
}


ResDataRow::ResDataRow(ResourceData *data)
  :	fResData(data)
{
	if (data) {
		SetField(new BStringField(fResData->GetIDString()), 0);
		SetField(new TypeCodeField(fResData->GetType(), fResData), 1);
		SetField(new BStringField(fResData->GetName()), 2);
		BField *field = gResRoster.MakeFieldForType(fResData->GetType(),
													fResData->GetData(),
													fResData->GetLength());
		if (field)
			SetField(field, 3);
		SetField(new BSizeField(fResData->GetLength()), 4);
	}
}
	

ResourceData *
ResDataRow::GetData(void) const
{
	return fResData;
}

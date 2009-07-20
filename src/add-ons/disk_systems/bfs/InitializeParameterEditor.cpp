/*
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include "InitializeParameterEditor.h"

#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <GridLayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PartitionParameterEditor.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <View.h>
#include <Window.h>


static uint32 MSG_BLOCK_SIZE = 'blsz';


InitializeBFSEditor::InitializeBFSEditor()
	:
	BPartitionParameterEditor(),
	fView(NULL),
	fNameTC(NULL),
	fBlockSizeMF(NULL),
	fParameters(NULL)
{
	_CreateViewControls();
}


InitializeBFSEditor::~InitializeBFSEditor()
{
}


BView*
InitializeBFSEditor::View()
{
	return fView;
}


bool
InitializeBFSEditor::FinishedEditing()
{
	fParameters = "";
	if (BMenuItem* item = fBlockSizeMF->Menu()->FindMarked()) {
		const char* size;
		BMessage* message = item->Message();
		if (!message || message->FindString("size", &size) < B_OK)
			size = "2048";
		// TODO: use libroot driver settings API
		fParameters << "block_size " << size << ";\n";
	}
	fParameters << "name " << fNameTC->Text() << ";\n";

	return true;
}


status_t
InitializeBFSEditor::GetParameters(BString* parameters)
{
	if (parameters == NULL)
		return B_BAD_VALUE;

	*parameters = fParameters;
	return B_OK;
}


status_t
InitializeBFSEditor::PartitionNameChanged(const char* name)
{
	fNameTC->SetText(name);
	return B_OK;
}


void
InitializeBFSEditor::_CreateViewControls()
{
	fNameTC = new BTextControl("Name", "Haiku", NULL);
	// TODO find out what is the max length for this specific FS partition name
	fNameTC->TextView()->SetMaxBytes(31);

	BPopUpMenu* blocksizeMenu = new BPopUpMenu("Blocksize");
	BMessage* message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "1024");
	blocksizeMenu->AddItem(new BMenuItem("1024 (Mostly small files)",
		message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "2048");
	BMenuItem* defaultItem = new BMenuItem("2048 (Recommended)", message);
	blocksizeMenu->AddItem(defaultItem);
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "4096");
	blocksizeMenu->AddItem(new BMenuItem("4096", message));
	message = new BMessage(MSG_BLOCK_SIZE);
	message->AddString("size", "8192");
	blocksizeMenu->AddItem(new BMenuItem("8192 (Mostly large files)",
		message));

	fBlockSizeMF = new BMenuField("Blocksize", blocksizeMenu, NULL);
	defaultItem->SetMarked(true);

	fView = BGroupLayoutBuilder(B_VERTICAL, 5)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(10))

		// test views
		.Add(BGridLayoutBuilder(10, 10)
			// row 1
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5), 0, 0)

			.Add(fNameTC->CreateLabelLayoutItem(), 1, 0)
			.Add(fNameTC->CreateTextViewLayoutItem(), 2, 0)

			.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 3, 0)

			// row 2
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(10), 0, 1)

			.Add(fBlockSizeMF->CreateLabelLayoutItem(), 1, 1)
			.Add(fBlockSizeMF->CreateMenuBarLayoutItem(), 2, 1)

			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5), 3, 1)
		)
	;
}

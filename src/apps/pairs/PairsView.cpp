/*
 * Copyright 2008, Ralf Schülke, teammaui@web.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "PairsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <List.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>

#include "Pairs.h"
#include "PairsGlobal.h"
#include "PairsTopButton.h"

// TODO: support custom board sizes

PairsView::PairsView(BRect frame, const char* name, uint32 resizingMode)
	: BView(frame, name, resizingMode, B_WILL_DRAW)
{
	// init bitmap pointers
	for (int i = 0; i < 8; i++)
		fCard[i] = NULL;

	CreateGameBoard();
	_SetPairsBoard();
}


void
PairsView::CreateGameBoard()
{
	// Show hidden buttons
	for (int32 i = 0; i < CountChildren(); i++) {
		BView* child = ChildAt(i);
		if (child->IsHidden())
			child->Show();
	}
	_GenerateCardPos();
}


PairsView::~PairsView()
{
	for (int i = 0; i < 8; i++)
		delete fCard[i];

	for (int i = 0; i < 16; i++)
		delete fDeckCard[i];
}


void
PairsView::AttachedToWindow()
{
	MakeFocus(true);
}


void
PairsView::_ReadRandomIcons()
{
	// TODO: maybe read the icons only once at startup

	// clean out any previous icons
	for (int i = 0; i < 8; i++) {
		delete fCard[i];
		fCard[i] = NULL;
	}

	BDirectory appsDirectory;
	BDirectory prefsDirectory;

	BPath path;
	if (find_directory(B_BEOS_APPS_DIRECTORY, &path) == B_OK)
		appsDirectory.SetTo(path.Path());
	if (find_directory(B_BEOS_PREFERENCES_DIRECTORY, &path) == B_OK)
		prefsDirectory.SetTo(path.Path());

	// read vector icons from apps and prefs folder and put them
	// into a BList as BBitmaps
	BList bitmaps;

	BEntry entry;
	while (appsDirectory.GetNextEntry(&entry) == B_OK
		|| prefsDirectory.GetNextEntry(&entry) == B_OK) {

		BNode node(&entry);
		BNodeInfo nodeInfo(&node);

		if (nodeInfo.InitCheck() < B_OK)
			continue;

		uint8* data;
		size_t size;
		type_code type;

		if (nodeInfo.GetIcon(&data, &size, &type) < B_OK)
			continue;

		if (type != B_VECTOR_ICON_TYPE) {
			delete[] data;
			continue;
		}

		BBitmap* bitmap = new BBitmap(
			BRect(0, 0, kBitmapSize - 1, kBitmapSize - 1), 0, B_RGBA32);
		if (BIconUtils::GetVectorIcon(data, size, bitmap) < B_OK) {
			delete[] data;
			delete bitmap;
			continue;
		}

		delete[] data;

		if (!bitmaps.AddItem(bitmap))
			delete bitmap;

		if (bitmaps.CountItems() >= 128) {
			// this is enough to choose from, stop eating memory...
			break;
		}
	}

	// pick eight random bitmaps from the ones we got in the list
	srand((unsigned)time(0));

	for (int i = 0; i < 8; i++) {
		int32 index = rand() % bitmaps.CountItems();
		fCard[i] = (BBitmap*)bitmaps.RemoveItem(index);
		if (fCard[i] == NULL) {
			BAlert* alert = new BAlert("fatal", "Pairs did not find enough "
				"vector icons in the system, it needs at least eight.",
				"Oh!", NULL, NULL, B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
			alert->Go();
			exit(1);
		}
	}

	// delete the remaining bitmaps from the list
	while (BBitmap* bitmap = (BBitmap*)bitmaps.RemoveItem(0L))
		delete bitmap;
}


void
PairsView::_SetPairsBoard()
{
	for (int i = 0; i < 16; i++) {
		fButtonMessage = new BMessage(kMsgCardButton);
		fButtonMessage->AddInt32("ButtonNum", i);

		int x =  i % 4 * (kBitmapSize + 10) + 10;
		int y =  i / 4 * (kBitmapSize + 10) + 10;

		fDeckCard[i] = new TopButton(x, y, fButtonMessage);
		AddChild(fDeckCard[i]);
	}
}


void
PairsView::_GenerateCardPos()
{
	_ReadRandomIcons();

	srand((unsigned)time(0));

	int positions[16];
	for (int i = 0; i < 16; i++)
		positions[i] = i;

	for (int i = 16; i >= 1; i--) {
		int index = rand() % i;

		fRandPos[16-i] = positions[index];

		for (int j = index; j < i - 1; j++)
			positions[j] = positions[j + 1];
	}

	for (int i = 0; i < 16; i++) {
		fPosX[i] = (fRandPos[i]) % 4 * (kBitmapSize + 10) + 10;
		fPosY[i] = (fRandPos[i]) / 4 * (kBitmapSize + 10) + 10;
	}
}


void
PairsView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);

	// draw rand pair 1 & 2
	for (int i = 0; i < 16; i++)
		DrawBitmap(fCard[i % 8], BPoint(fPosX[i], fPosY[i]));
}


int
PairsView::GetIconFromPos(int pos)
{
	return fRandPos[pos];
}

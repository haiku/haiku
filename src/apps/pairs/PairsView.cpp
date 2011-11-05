/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PairsView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
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

PairsView::PairsView(BRect frame, const char* name, int width, int height,
		uint32 resizingMode)
	:
	BView(frame, name, resizingMode, B_WILL_DRAW),
	fWidth(width),
	fHeight(height),
	fNumOfCards(width * height),
	fRandPos(new int[fNumOfCards]),
	fPosX(new int[fNumOfCards]),
	fPosY(new int[fNumOfCards])
{
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
	for (int i = 0; i < fCardBitmaps.CountItems(); i++)
		delete ((BBitmap*)fCardBitmaps.ItemAt(i));

	for (int i = 0; i < fDeckCard.CountItems(); i++)
		delete ((TopButton*)fDeckCard.ItemAt(i));

	delete fRandPos;
	delete fPosX;
	delete fPosY;
}


void
PairsView::AttachedToWindow()
{
	MakeFocus(true);
}


bool
PairsView::_HasBitmap(BList& bitmaps, BBitmap* bitmap)
{
	// TODO: if this takes too long, we could build a hash value for each
	// bitmap in a separate list
	for (int32 i = bitmaps.CountItems(); i-- > 0;) {
		BBitmap* item = (BBitmap*)bitmaps.ItemAtFast(i);
		if (!memcmp(item->Bits(), bitmap->Bits(), item->BitsLength()))
			return true;
	}

	return false;
}

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PairsView"

void
PairsView::_ReadRandomIcons()
{
	// TODO: maybe read the icons only once at startup

	// clean out any previous icons
	for (int i = 0; i < fCardBitmaps.CountItems(); i++)
		delete ((BBitmap*)fCardBitmaps.ItemAt(i));

	fCardBitmaps.MakeEmpty();

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

		if (_HasBitmap(bitmaps, bitmap) || !bitmaps.AddItem(bitmap))
			delete bitmap;
		else if (bitmaps.CountItems() >= 128) {
			// this is enough to choose from, stop eating memory...
			break;
		}
	}

	// pick random bitmaps from the ones we got in the list
	srand((unsigned)time(0));

	for (int i = 0; i < fNumOfCards / 2; i++) {
		int32 index = rand() % bitmaps.CountItems();
		BBitmap* bitmap = ((BBitmap*)bitmaps.RemoveItem(index));
		if (bitmap == NULL) {
			char buffer[512];
			snprintf(buffer, sizeof(buffer), B_TRANSLATE("Pairs did not find "
				"enough vector icons in the system; it needs at least %d."),
				fNumOfCards / 2);
			BString msgStr(buffer);
			msgStr << "\n";
			BAlert* alert = new BAlert("Fatal", msgStr.String(),
				B_TRANSLATE("OK"), 	NULL, NULL, B_WIDTH_FROM_WIDEST,
				B_STOP_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			alert->Go();
			exit(1);
		}
		fCardBitmaps.AddItem(bitmap);
	}

	// delete the remaining bitmaps from the list
	while (BBitmap* bitmap = (BBitmap*)bitmaps.RemoveItem(0L))
		delete bitmap;
}


void
PairsView::_SetPairsBoard()
{
	for (int i = 0; i < fNumOfCards; i++) {
		fButtonMessage = new BMessage(kMsgCardButton);
		fButtonMessage->AddInt32("ButtonNum", i);

		int x =  i % fWidth * (kBitmapSize + kSpaceSize) + kSpaceSize;
		int y =  i / fHeight * (kBitmapSize + kSpaceSize) + kSpaceSize;

		TopButton* button = new TopButton(x, y, fButtonMessage);
		fDeckCard.AddItem(button);
		AddChild(button);
	}
}


void
PairsView::_GenerateCardPos()
{
	_ReadRandomIcons();

	srand((unsigned)time(0));

	int* positions = new int[fNumOfCards];
	for (int i = 0; i < fNumOfCards; i++)
		positions[i] = i;

	for (int i = fNumOfCards; i >= 1; i--) {
		int index = rand() % i;

		fRandPos[fNumOfCards - i] = positions[index];

		for (int j = index; j < i - 1; j++)
			positions[j] = positions[j + 1];
	}

	for (int i = 0; i < fNumOfCards; i++) {
		fPosX[i] = (fRandPos[i]) % fWidth * (kBitmapSize + kSpaceSize)
			+ kSpaceSize;
		fPosY[i] = (fRandPos[i]) / fHeight * (kBitmapSize + kSpaceSize)
			+ kSpaceSize;
	}

	delete [] positions;
}


void
PairsView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);

	// draw rand pair 1 & 2
	for (int i = 0; i < fNumOfCards; i++) {
		BBitmap* bitmap = ((BBitmap*)fCardBitmaps.ItemAt(i % fNumOfCards / 2));
		DrawBitmap(bitmap, BPoint(fPosX[i], fPosY[i]));
	}
}


int
PairsView::GetIconFromPos(int pos)
{
	return fRandPos[pos];
}

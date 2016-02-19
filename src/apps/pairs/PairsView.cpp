/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2010 Adam Smith <adamd.smith@utoronto.ca>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ralf Schülke, ralf.schuelke@googlemail.com
 *		John Scipione, jscipione@gmail.com
 *		Adam Smith, adamd.smith@utoronto.ca
 */


#include "PairsView.h"

#include <stdlib.h>
	// for srand() and rand()

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <Window.h>

#include "Pairs.h"
#include "PairsButton.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PairsView"


//	#pragma mark - PairsView


PairsView::PairsView(BRect frame, const char* name, uint8 rows, uint8 cols,
	uint8 iconSize)
	:
	BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
	fRows(rows),
	fCols(cols),
	fIconSize(iconSize),
	fButtonsCount(rows * cols),
	fCardsCount(fButtonsCount / 2),
	fPairsButtonList(new BObjectList<PairsButton>(fButtonsCount)),
	fSmallBitmapsList(new BObjectList<BBitmap>(fCardsCount)),
	fMediumBitmapsList(new BObjectList<BBitmap>(fCardsCount)),
	fLargeBitmapsList(new BObjectList<BBitmap>(fCardsCount)),
	fRandomPosition(new int32[fButtonsCount]),
	fPositionX(new int32[fButtonsCount]),
	fPositionY(new int32[fButtonsCount])
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	CreateGameBoard();
	_SetPairsBoard();
}


void
PairsView::CreateGameBoard()
{
	// Show hidden buttons
	int32 childrenCount = CountChildren();
	for (int32 i = 0; i < childrenCount; i++) {
		BView* child = ChildAt(i);
		if (child->IsHidden())
			child->Show();
	}
	_GenerateCardPositions();
}


PairsView::~PairsView()
{
	delete fSmallBitmapsList;
	delete fMediumBitmapsList;
	delete fLargeBitmapsList;
	delete fPairsButtonList;
	delete fRandomPosition;
	delete fPositionX;
	delete fPositionY;
}


void
PairsView::AttachedToWindow()
{
	for (int32 i = 0; i < fButtonsCount; i++) {
		PairsButton* button = fPairsButtonList->ItemAt(i);
		if (button != NULL)
			button->SetTarget(Window());
	}

	MakeFocus(true);
	BView::AttachedToWindow();
}


void
PairsView::Draw(BRect updateRect)
{
	BObjectList<BBitmap>* bitmapsList;
	switch (fIconSize) {
		case kSmallIconSize:
			bitmapsList = fSmallBitmapsList;
			break;

		case kLargeIconSize:
			bitmapsList = fLargeBitmapsList;
			break;

		case kMediumIconSize:
		default:
			bitmapsList = fMediumBitmapsList;
	}

	for (int32 i = 0; i < fButtonsCount; i++) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(bitmapsList->ItemAt(i % (fButtonsCount / 2)),
			BPoint(fPositionX[i], fPositionY[i]));
		SetDrawingMode(B_OP_COPY);
	}
}


void
PairsView::FrameResized(float newWidth, float newHeight)
{
	int32 spacing = Spacing();
	for (int32 i = 0; i < fButtonsCount; i++) {
		PairsButton* button = fPairsButtonList->ItemAt(i);
		if (button != NULL) {
			button->ResizeTo(fIconSize, fIconSize);
			int32 x = i % fRows * (fIconSize + spacing) + spacing;
			int32 y = i / fCols * (fIconSize + spacing) + spacing;
			button->MoveTo(x, y);
			button->SetFontSize(fIconSize - 15);
		}
	}

	_SetPositions();
	Invalidate(BRect(0, 0, newWidth, newHeight));
	BView::FrameResized(newWidth, newHeight);
}


int32
PairsView::GetIconPosition(int32 index)
{
	return fRandomPosition[index];
}


//	#pragma mark - PairsView private methods


void
PairsView::_GenerateCardPositions()
{
	// seed the random number generator based on the current timestamp
	srand((unsigned)time(0));

	_ReadRandomIcons();

	int32* positions = new int32[fButtonsCount];
	for (int32 i = 0; i < fButtonsCount; i++)
		positions[i] = i;

	for (int32 i = fButtonsCount; i > 0; i--) {
		int32 index = rand() % i;
		fRandomPosition[fButtonsCount - i] = positions[index];
		for (int32 j = index; j < i - 1; j++)
			positions[j] = positions[j + 1];
	}
	delete[] positions;

	_SetPositions();
}


void
PairsView::_ReadRandomIcons()
{
	Pairs* app = dynamic_cast<Pairs*>(be_app);
	if (app == NULL) // check if NULL to make Coverity happy
		return;

	// Create a copy of the icon map so we can erase elements from it as we
	// add them to the list eliminating repeated icons without altering the
	// orginal IconMap.
	IconMap tmpIconMap(app->GetIconMap());
	size_t mapSize = tmpIconMap.size();
	if (mapSize < (size_t)fCardsCount) {
		// not enough icons, we're screwed
		return;
	}

	// clean out any previous icons
	fSmallBitmapsList->MakeEmpty();
	fMediumBitmapsList->MakeEmpty();
	fLargeBitmapsList->MakeEmpty();

	// pick bitmaps at random from the icon map
	for (int32 i = 0; i < fCardsCount; i++) {
		IconMap::iterator iter = tmpIconMap.begin();
		if (mapSize < (size_t)fCardsCount) {
			// not enough valid icons, we're really screwed
			return;
		}
		std::advance(iter, rand() % mapSize);
		size_t key = iter->first;
		vector_icon* icon = iter->second;

		BBitmap* smallBitmap = new BBitmap(
			BRect(0, 0, kSmallIconSize - 1, kSmallIconSize - 1), B_RGBA32);
		status_t smallResult = BIconUtils::GetVectorIcon(icon->data,
			icon->size, smallBitmap);
		BBitmap* mediumBitmap = new BBitmap(
			BRect(0, 0, kMediumIconSize - 1, kMediumIconSize - 1), B_RGBA32);
		status_t mediumResult = BIconUtils::GetVectorIcon(icon->data,
			icon->size, mediumBitmap);
		BBitmap* largeBitmap = new BBitmap(
			BRect(0, 0, kLargeIconSize - 1, kLargeIconSize - 1), B_RGBA32);
		status_t largeResult = BIconUtils::GetVectorIcon(icon->data,
			icon->size, largeBitmap);

		if (smallResult + mediumResult + largeResult == B_OK) {
			fSmallBitmapsList->AddItem(smallBitmap);
			fMediumBitmapsList->AddItem(mediumBitmap);
			fLargeBitmapsList->AddItem(largeBitmap);
		} else {
			delete smallBitmap;
			delete mediumBitmap;
			delete largeBitmap;
			i--;
		}

		mapSize -= tmpIconMap.erase(key);
			// remove the element from the map so we don't read it again
	}
}


void
PairsView::_SetPairsBoard()
{
	int32 spacing = Spacing();
	for (int32 i = 0; i < fButtonsCount; i++) {
		BMessage* buttonMessage = new BMessage(kMsgCardButton);
		buttonMessage->AddInt32("button number", i);

		int32 x = i % fRows * (fIconSize + spacing) + spacing;
		int32 y = i / fCols * (fIconSize + spacing) + spacing;

		PairsButton* button = new PairsButton(x, y, fIconSize, buttonMessage);
		fPairsButtonList->AddItem(button);
		AddChild(button);
	}
}


void
PairsView::_SetPositions()
{
	int32 spacing = Spacing();
	for (int32 i = 0; i < fButtonsCount; i++) {
		fPositionX[i] = fRandomPosition[i] % fRows * (fIconSize + spacing) + spacing;
		fPositionY[i] = fRandomPosition[i] / fCols * (fIconSize + spacing) + spacing;
	}
}

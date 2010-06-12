//--------------------------------------------------------------------
//	
//	TestMenuBuilder.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Font.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "TestIcons.h"
#include "TestMenuBuilder.h"
#include "BitmapMenuItem.h"

//====================================================================
//	TestMenuBuilder Implementation

#define TEST_NAME_LENGTH		20



//--------------------------------------------------------------------
//	TestMenuBuilder entry point

BMenu* TestMenuBuilder::BuildTestMenu(icon_size size)
{
	BitmapMenuItem *pItemArray[NUM_TEST_ITEMS_DOWN][NUM_TEST_ITEMS_ACROSS];
	BitmapMenuItem *pTypicalItem = NULL;

	// fill the array of bitmap menu items
	for (int32 i=0; i<NUM_TEST_ITEMS_DOWN; i++) {
		for (int32 j=0; j<NUM_TEST_ITEMS_ACROSS; j++) {
			BBitmap *pBitmap = LoadTestBitmap(i, j, size);
			if (! pBitmap) {
				pItemArray[i][j] = NULL;
			} else {
				int32 itemIndex = NUM_TEST_ITEMS_ACROSS*i + j + 1;
				char name[TEST_NAME_LENGTH];
				sprintf(name, "%s %ld", STR_MNU_TEST_ITEM, itemIndex);
				BMessage* pTestMsg = new BMessage(MSG_TEST_ITEM);
				pTestMsg->AddInt32("Item Index", itemIndex);
				char shortcut = '0' + itemIndex; // the item's number
				BitmapMenuItem *pItem = new BitmapMenuItem(name, *pBitmap,
					pTestMsg, shortcut, 0);
				pItemArray[i][j] = pItem;
				pTypicalItem = pItem;
			}
			delete pBitmap;
		}
	}

	float menuHeight, menuWidth;

	// Simplifying assumption: All test items have same width and height.
	// Use pTypicalItem to calculate frames.
	if (! pTypicalItem) {
		// no items to put in a menu
		return NULL;
	}
	float itemHeight, itemWidth;
	pTypicalItem->GetBitmapSize(&itemWidth, &itemHeight);
	itemWidth++; // take space between items into account
	itemHeight++;
	
	menuWidth = NUM_TEST_ITEMS_ACROSS*itemWidth;
	menuHeight = NUM_TEST_ITEMS_DOWN*itemHeight;

	// create menu
	float left, top;
	BRect frame;
	BMenu* pMenu = new BMenu(STR_MNU_TEST, menuWidth, menuHeight);
	for (int32 i=0; i<NUM_TEST_ITEMS_DOWN; i++) {
		for (int32 j=0; j<NUM_TEST_ITEMS_ACROSS; j++) {
			BitmapMenuItem* pItem = pItemArray[i][j];
			if (pItem) {
				left = j*itemWidth;
				top = i*itemHeight;
				frame.Set(left, top, left + itemWidth - 1,
					top + itemHeight - 1);
				pMenu->AddItem(pItem, frame);
			}
		}
	}

	return pMenu;
}

//--------------------------------------------------------------------
//	TestMenuBuilder implementation member functions

BBitmap* TestMenuBuilder::LoadTestBitmap(int32 i, int32 j, icon_size size)
{
	BBitmap* pBitmap;
	const uint8* bits;
	uint32 byteLen;
	color_space iconColorSpace;
	BRect iconBounds(0,0,0,0);
	
	if ((i < 0) || (j < 0)) {
		return NULL;
	}
	
	if ((i >= NUM_TEST_ITEMS_DOWN) || (j >= NUM_TEST_ITEMS_ACROSS)) {
		return NULL;
	}
	
	if (size == B_LARGE_ICON) {
		bits = kLargeTestIcons[i][j];
		byteLen = LARGE_ICON_BYTES;
		iconBounds.right = kLargeIconWidth - 1; // 0 counts as a pixel!
		iconBounds.bottom = kLargeIconHeight - 1;
		iconColorSpace = kLargeIconColorSpace;
	} else {
		bits = kMiniTestIcons[i][j];
		byteLen = MINI_ICON_BYTES;
		iconBounds.right = kMiniIconWidth - 1;
		iconBounds.bottom = kMiniIconHeight - 1;
		iconColorSpace = kMiniIconColorSpace;
	}
	
	pBitmap = new BBitmap(iconBounds, iconColorSpace);
	uint8* destBits = (uint8*)pBitmap->Bits();
	memcpy(destBits, bits, byteLen);	
		 
	return pBitmap;		
}

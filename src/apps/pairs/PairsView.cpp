/*
 * Copyright 2008, Ralf Schülke, teammaui@web.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <cstdlib> 
#include <ctime>

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>

#include "PairsTopButton.h"
#include "PairsView.h"
#include "PairsGlobal.h"
#include "Pairs.h"

#include "bitmaps/appearance.h"
#include "bitmaps/cortex.h"
#include "bitmaps/joystick.h"
#include "bitmaps/kernel.h"
#include "bitmaps/launchbox.h"
#include "bitmaps/people.h"
#include "bitmaps/teapot.h"
#include "bitmaps/tracker.h"

// TODO: support custom board sizes

PairsView::PairsView(BRect frame, const char* name, uint32 resizingMode)
	: BView(frame, name, resizingMode, B_WILL_DRAW)
{
	_PairsCards();
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
	_GenarateCardPos();
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
PairsView::_PairsCards()
{
	for (int i = 0; i < 8; i++) {
		fCard[i] = new BBitmap(BRect(0, 0, kBitmapSize - 1, kBitmapSize - 1),
			B_RGBA32);
	}
		
	// TODO: read random icons from the files in /boot/beos/apps
	// and /boot/beos/preferences...
	memcpy(fCard[0]->Bits(), kappearanceBits, fCard[0]->BitsLength());
	memcpy(fCard[1]->Bits(), kcortexBits, fCard[1]->BitsLength());
	memcpy(fCard[2]->Bits(), kjoystickBits, fCard[2]->BitsLength());
	memcpy(fCard[3]->Bits(), kkernelBits, fCard[3]->BitsLength());
	memcpy(fCard[4]->Bits(), klaunchboxBits, fCard[4]->BitsLength());
	memcpy(fCard[5]->Bits(), kpeopleBits, fCard[5]->BitsLength());
	memcpy(fCard[6]->Bits(), kteapotBits, fCard[6]->BitsLength());
	memcpy(fCard[7]->Bits(), ktrackerBits, fCard[7]->BitsLength());
}


void
PairsView::_SetPairsBoard()
{	
	for (int i = 0; i < 16; i++) {
		fButtonMessage = new BMessage(kMsgCardButton);
		fButtonMessage->AddInt32("ButtonNum",i);
		
		int x =  i % 4 * (kBitmapSize + 10) + 10;
		int y =  i / 4 * (kBitmapSize + 10) + 10;
		
		fDeckCard[i] = new TopButton(x, y, fButtonMessage);
		AddChild(fDeckCard[i]);
	}	
}


void
PairsView::_GenarateCardPos()
{
	// TODO: when loading random icons, the would also have to be
	// loaded here (or at least somewhere in the code path that creates
	// a new game after one finished)

	srand((unsigned)time(0)); 
	
	int positions[16];	
	for (int i = 0; i < 16; i++)
		positions[i] = i;
	
	for (int i = 16; i >= 1; i--) {
		int index = rand() % i;

		fRandPos[16-i] = positions[index];
		
		for (int j = index; j < i - 1; j++) {
			positions[j] = positions[j + 1];
		}
	}
	
	for (int i = 0; i < 16; i++) {
		fPosX[i] =  (fRandPos[i]) % 4 * (kBitmapSize+10) + 10;
		fPosY[i] =  (fRandPos[i]) / 4 * (kBitmapSize+10) + 10;
	}
}


void
PairsView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_ALPHA);
	
	// draw rand pair 1 & 2
	for (int i = 0; i < 16; i++) {
		DrawBitmap(fCard[i % 8], BPoint(fPosX[i], fPosY[i]));	
	}
}


int
PairsView::GetIconFromPos(int pos)
{	
	return fRandPos[pos]; 
}



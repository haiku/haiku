/*
Author:	Michael Pfeiffer
Email:	michael.pfeiffer@utanet.at
*/
#include "Scope.h"

#include <MessageQueue.h>
#include <Bitmap.h>
#include <Synth.h>
#include <Window.h>

//----------------------------------------------------------

Scope::Scope(BRect rect) :
	BView(rect, NULL, 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_PULSE_NEEDED) {
	SetViewColor(B_TRANSPARENT_COLOR);	

	mWidth = (int) rect.Width(); mHeight = (int)rect.Height();
	mSampleCount = mWidth;
	mLeft = new int16[mSampleCount]; mRight = new int16[mSampleCount];
		
	mBitmap = new BBitmap(BRect(0, 0, mWidth, mHeight), B_RGB_16_BIT, true, true);
	if (mBitmap->Lock()) {
		mOffscreenView = new BView(mBitmap->Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW);
		mOffscreenView->SetHighColor(0, 0, 0, 0);
	
		mBitmap->AddChild(mOffscreenView);
		mOffscreenView->FillRect(mBitmap->Bounds());
		mOffscreenView->Sync();
		mBitmap->Unlock();
	}
}

//----------------------------------------------------------

void Scope::Draw(BRect updateRect) {
	DrawBitmap(mBitmap, BPoint(0, 0));
}

//----------------------------------------------------------

void Scope::DrawSample() {
	int32 size = be_synth->GetAudio(mLeft, mRight, mSampleCount);
	
	if (mBitmap->Lock()) {
		mOffscreenView->SetHighColor(0, 0, 0, 0);
		mOffscreenView->FillRect(mBitmap->Bounds());		
		if (size > 0) {
			mOffscreenView->SetHighColor(255, 0, 0, 0);
			mOffscreenView->SetLowColor(0, 255, 0, 0);
			#define N 16
			int32 x, y, sx = 0, f = (mHeight << N) / 65535, dy = mHeight / 2;
			for (int32 i = 0; i < mWidth; i++) {
				x = sx / mWidth;
				y = ((mLeft[x] * f) >> N)  + dy;
				mOffscreenView->FillRect(BRect(i, y, i, y));
				y = ((mRight[x] * f) >> N) + dy;
				mOffscreenView->FillRect(BRect(i, y, i, y), B_SOLID_LOW);
				sx += size;
			}
		}
		mOffscreenView->Flush();
		mBitmap->Unlock();
	}
}

//----------------------------------------------------------

#define PULSE_RATE 1 * 100000
void Scope::AttachedToWindow(void) {
	Window()->SetPulseRate(PULSE_RATE);
}

//----------------------------------------------------------

void Scope::DetachedFromWindow(void) {
	Window()->SetPulseRate(0);
	delete mLeft; delete mRight;
	
	if (mBitmap->Lock()) {
		mBitmap->RemoveChild(mOffscreenView);
		delete mOffscreenView;
		mBitmap->Unlock();
	}
	delete mBitmap;
}

//----------------------------------------------------------

void Scope::Pulse() {
	DrawSample();
	BMessage *m;
	BMessageQueue *mq = Window()->MessageQueue();
	int i = 0;	
	while ((m = mq->FindMessage(B_PULSE, 0)) != NULL) mq->RemoveMessage(m), i++;	
	Invalidate();
}
//----------------------------------------------------------

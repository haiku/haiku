// WorkView.cpp

#include <Application.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <stdio.h>
#include <Path.h>
#include "MultiTest.h"
#include "WorkView.h"

const char *kPath1 = "../data/images/image.jpg";
const char *kPath2 = "../data/images/image.gif";

WorkView::WorkView(BRect rect)
	: BView(rect, "Work View", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fbImage = true;
	fPath = kPath1;
}

void
WorkView::AttachedToWindow()
{
	BTranslatorRoster *pRoster = NULL;
	BBitmap *pBitmap;
	
	//pRoster = ((MultiTestApplication *) be_app)->GetTranslatorRoster();
	
	pBitmap = BTranslationUtils::GetBitmap(fPath, pRoster);
	if (pBitmap) {
		SetViewBitmap(pBitmap);
		delete pBitmap;
	}
}

void
WorkView::Pulse()
{
	if (fbImage) {
		ClearViewBitmap();
		fbImage = false;
		if (fPath == kPath1)
			fPath = kPath2;
		else
			fPath = kPath1;
	} else {
		//BTranslatorRoster *pRoster = NULL;
		BBitmap *pBitmap = BTranslationUtils::GetBitmapFile(fPath);
		if (pBitmap) {
			ClearViewBitmap();
			SetViewBitmap(pBitmap);
			delete pBitmap;
		} else {
			BPath Path(fPath);
			printf("-- failed to get bitmap (%s)!\n", Path.Path());
		}
		fbImage = true;
	}
	
	Invalidate();
}

/*
 * MakeScreenshot function
 */

#include <Application.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Message.h>
#include <Screen.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

// PRIVATE: in libzeta for now.
extern status_t ScaleBitmap(const BBitmap& inBitmap, BBitmap& outBitmap);

status_t MakeScreenshot(BBitmap **here)
{
	status_t err;
	BScreen bs;
	BWindow *win;
	BBitmap *shot;
	BBitmap *scaledBmp = NULL;
	be_app->Lock();
	win = be_app->WindowAt(0);
	if (win) {
		win->Lock();
		win->Hide();
		win->Unlock();
	}
	snooze(500000);
	err = bs.GetBitmap(&shot);
	if (!err) {
		BRect scaledBounds(0,0,640-1,480-1);
		scaledBmp = new BBitmap(scaledBounds, B_BITMAP_ACCEPTS_VIEWS, B_RGB32/*shot->ColorSpace()*/);
		err = scaledBmp->InitCheck();
		if (!err) {
			err = ENOSYS;
#ifdef B_ZETA_VERSION
			err = ScaleBitmap(*shot, *scaledBmp);
#endif
			if (err) {
				// filtered scaling didn't work, do it manually
				BView *v = new BView(scaledBounds, "scaleview", B_FOLLOW_NONE, 0);
				scaledBmp->AddChild(v);
				v->LockLooper();
				v->DrawBitmap(shot);
				v->Sync();
				v->UnlockLooper();
				scaledBmp->RemoveChild(v);
				delete v;
				err = B_OK;
			}
		}
		delete shot;
	}

	if (win) {
		win->Lock();
		win->Show();
		win->Unlock();
	}
	be_app->Unlock();
	
	if (err)
		return err;
	
	*here = scaledBmp;

	return B_OK;
}

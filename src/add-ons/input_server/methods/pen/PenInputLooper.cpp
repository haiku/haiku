/*
	Copyright 2007, Francois Revol.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#define DEBUG 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Debug.h>
#include <List.h>
#include <Message.h>
#include <OS.h>

#include <Application.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Region.h>
#include <Screen.h>

#if DEBUG
//#include <File.h>
#include <Alert.h>
#include <Button.h>
#include <TextView.h>
#include <StringIO.h>
#include "DumpMessage.h"
#endif

#include "PenInputLooper.h"
#include "PenInputBackend.h"
#include "PenInputInkWindow.h"
#include "PenInputServerMethod.h"
#include "PenInputStrings.h"

PenInputLooper::PenInputLooper(PenInputServerMethod *method)
	: BLooper("PenInputLooper", B_NORMAL_PRIORITY),
	  fOwner(method),
	  fInkWindow(NULL),
	  fBackend(NULL),
	  fDeskbarMenu(NULL),
	  fMouseDown(false),
	  fStroking(false),
	  fThresholdRunner(NULL),
	  fCachedMouseDown(NULL),
	  fShowInk(true),
	  fMouseDownThreshold(100000)
{
	PRINT(("%s\n", __FUNCTION__));

	//
	fDeskbarMenu = new BMenu(_T("Options"));
	BMenu *subMenu;

	
	subMenu = new BMenu(_T("Backend"));
	
	BMessage *msg;
	msg = new BMessage(MSG_SET_BACKEND);
	BMenuItem *item;
	// for (i = 0; i < fBackends.CountItems(); i++) ...
	item = new BMenuItem(_T("Default"), msg);
	item->SetMarked(true);
	subMenu->AddItem(item);
	
	item = new BMenuItem(subMenu);
	fDeskbarMenu->AddItem(item);
	
	msg = new BMessage(MSG_SHOW_INK);
	item = new BMenuItem(_T("Show Ink"), msg);
	item->SetMarked(true);
	fDeskbarMenu->AddItem(item);
	
	fDeskbarMenu->SetFont(be_plain_font);
	fOwner->SetMenu(fDeskbarMenu, BMessenger(NULL, this));

#if 0
	BRegion region;
	fOwner->GetScreenRegion(&region);
	fInkWindow = new PenInputInkWindow(region.Frame());
#endif
	BScreen screen;
	fInkWindow = new PenInputInkWindow(screen.Frame());
	fInkWindow->Show();
	fInkWindow->Lock();
	fInkWindow->Hide();
	fInkWindow->Unlock();
	fInkWindowMsgr = BMessenger(NULL, fInkWindow);

	BMessage m(MSG_CHECK_PEN_DOWN);
	fThresholdRunner = new BMessageRunner(BMessenger(NULL, this), &m, fMouseDownThreshold, 0);
	
	Run();
}

void PenInputLooper::Quit()
{
	PRINT(("%s\n", __FUNCTION__));
	delete fThresholdRunner;
	fInkWindow->Lock();
	fInkWindow->Quit();
	fOwner->SetMenu(NULL, BMessenger());
	delete fDeskbarMenu;
	BLooper::Quit();
}


void PenInputLooper::DispatchMessage(BMessage *message, BHandler *handler)
{
	switch (message->what) {
#if 0
	case B_MOUSE_MOVED:
	case B_MOUSE_DOWN:
	case B_MOUSE_UP:
		
	case B_INPUT_METHOD_EVENT:
		//case B_REPLY:
		if (fBackend) {
			fBackend->MessageReceived(message);
			return;
		}
#endif
	default:
		return BLooper::DispatchMessage(message, handler);
	}
}

void PenInputLooper::MessageReceived(BMessage *message)
{
	int32 v;
	int32 buttons;
	bool b;
#if 1
    {
		//message->Flatten(&fDebugFile);
		BStringIO sio;
		DumpMessageToStream(message, sio);
		fInkWindow->Lock();
		sio << BString("StrokesCount: ");
		BString tmp;
		tmp << fInkWindow->fStrokeCount;
		sio << tmp;
		sio << BString("\nStrokes.Bounds: ");
		sio << (fInkWindow->fStrokes.Bounds());
		sio << BString("\n");
		fInkWindow->Unlock();
		fOwner->fDebugAlert->Lock();
		fOwner->fDebugAlert->TextView()->Insert("Looper:\n");
		fOwner->fDebugAlert->TextView()->Insert(sio.String());
		fOwner->fDebugAlert->Unlock();
    }
#endif
	switch (message->what) {
	case MSG_CHECK_PEN_DOWN:
		if (fCachedMouseDown) {
			/* nothing happened, we want to move the mouse */
			EnqueueMessage(fCachedMouseDown);
			fCachedMouseDown = NULL;
		}
		break;
	case B_MOUSE_MOVED:
		if (!fMouseDown) { /* definitely not writing */
			/* pass it along */
			EnqueueMessage(DetachCurrentMessage());
			break;
		}
		if (fShowInk) {
			BMessage *msg = new BMessage(*message);
			fInkWindowMsgr.SendMessage(msg);
		}
		if (fBackend)
			fBackend->MessageReceived(message);
		else
			EnqueueMessage(DetachCurrentMessage());
		break;
	case B_MOUSE_DOWN:
		if (message->FindInt32("buttons", &buttons) == B_OK) {
			if (!(buttons & B_PRIMARY_MOUSE_BUTTON)) {
				/* pass it along */
				EnqueueMessage(DetachCurrentMessage());
				break;
			}
		}
		fMouseDown = true;
		if (fShowInk) {
			{
				BMessage *msg = new BMessage(MSG_BEGIN_INK);
				fInkWindowMsgr.SendMessage(msg);
			}
			BMessage *msg = new BMessage(*message);
			fInkWindowMsgr.SendMessage(msg);
		}
		if (fBackend)
			fBackend->MessageReceived(message);
		break;
	case B_MOUSE_UP:
		if (message->FindInt32("buttons", &buttons) == B_OK) {
			if (!fMouseDown || (buttons & B_PRIMARY_MOUSE_BUTTON)) {
				/* pass it along */
				EnqueueMessage(DetachCurrentMessage());
				break;
			}
		}
		fMouseDown = false;
		if (fShowInk) {
			BMessage *msg = new BMessage(*message);
			fInkWindowMsgr.SendMessage(msg);
			{
				BMessage *msg = new BMessage(MSG_END_INK);
				fInkWindowMsgr.SendMessage(msg);
			}
		}
		if (fBackend)
			fBackend->MessageReceived(message);
		break;
	case B_INPUT_METHOD_EVENT:
		if (fBackend)
			fBackend->MessageReceived(message);
		break;
	case MSG_METHOD_ACTIVATED:
		if (message->FindBool(MSGF_ACTIVE, &b) < B_OK)
			b = false;
		HandleMethodActivated(b);
		break;
	case B_REPLY:
		
	default:
		BLooper::MessageReceived(message);
		break;
	}
}

void PenInputLooper::EnqueueMessage(BMessage *message)
{
	fOwner->EnqueueMessage(message);
}

status_t PenInputLooper::InitCheck()
{
	return B_OK;
}

void PenInputLooper::HandleMethodActivated(bool active)
{
/*
  if (fShowInk && active)
  fInkWindowMsgr.SendMessage(MSG_SHOW_WIN);
  if (fShowInk && !active)
  fInkWindowMsgr.SendMessage(MSG_END_INK);
*/
}


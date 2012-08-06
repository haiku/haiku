/*
	Copyright 2007, Francois Revol.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

//#define DEBUG 1

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
#include <Region.h>

#if DEBUG
//#include <File.h>
#include <Alert.h>
#include <Button.h>
#include <TextView.h>
#include <StringIO.h>
#include "DumpMessage.h"
#endif

#include <add-ons/input_server/InputServerDevice.h>
#include <add-ons/input_server/InputServerMethod.h>

#include "PenIcon.h"

#include "PenInputServerMethod.h"
#include "PenInputLooper.h"
#include "PenInputStrings.h"

BInputServerMethod* instantiate_input_method()
{
	PRINT(("%s\n", __FUNCTION__));
	return (new PenInputServerMethod());
}


PenInputServerMethod::PenInputServerMethod()
  : BInputServerMethod("Pen", PenIconData),
    fEnabled(false)
{
	PRINT(("%s\n", __FUNCTION__));
#if DEBUG
	//fDebugFile.SetTo("/tmp/PenInputMethodMessages.txt", B_READ_WRITE|B_CREATE_FILE);
	fDebugAlert = new BAlert("PenInput Debug", "Plip                                   \n\n\n\n\n\n\n\n\n\n\n\n\n", "OK");
	fDebugAlert->SetLook(B_TITLED_WINDOW_LOOK);
	fDebugAlert->SetFlags(fDebugAlert->Flags() | B_CLOSE_ON_ESCAPE);
	fDebugAlert->TextView()->MakeSelectable();
	fDebugAlert->TextView()->SelectAll();
	fDebugAlert->TextView()->Delete();
	fDebugAlert->ButtonAt(0)->SetEnabled(false);
	BRegion r;
	GetScreenRegion(&r);
	BString s;
	s << r.CountRects() << " rects\n";
	fDebugAlert->TextView()->Insert(s.String());
	fDebugAlert->Go(NULL);
	
	
#endif
}

PenInputServerMethod::~PenInputServerMethod()
{
	PRINT(("%s\n", __FUNCTION__));
	SetMenu(NULL, BMessenger());
#if DEBUG
	fDebugAlert->Lock();
	fDebugAlert->Quit();
#endif
	BLooper *looper = NULL;
	fLooper.Target(&looper);
	if (looper != NULL)
	{
		if (looper->Lock())
			looper->Quit();
	}
}

status_t PenInputServerMethod::InitCheck()
{
	PenInputLooper *looper;
	status_t err;
	PRINT(("%s\n", __FUNCTION__));
	looper = new PenInputLooper(this);
	looper->Lock();
	err = looper->InitCheck();
	looper->Unlock();
	fLooper = BMessenger(NULL, looper);
	return err;
}


filter_result PenInputServerMethod::Filter(BMessage *message, BList *outList)
{
	status_t err;
	filter_result res = B_DISPATCH_MESSAGE;

	if (!IsEnabled())
		return B_DISPATCH_MESSAGE;
	
	
#if 0//DEBUG
	//message->Flatten(&fDebugFile);
	BStringIO sio;
	DumpMessageToStream(message, sio);
	fDebugAlert->Lock();
	fDebugAlert->TextView()->Insert(sio.String());
	fDebugAlert->Unlock();
#endif
	switch (message->what) {
	case B_KEY_UP:
	case B_KEY_DOWN:
	case B_UNMAPPED_KEY_UP:
	case B_UNMAPPED_KEY_DOWN:
	case B_MODIFIERS_CHANGED:
	case B_MOUSE_WHEEL_CHANGED:
		return B_DISPATCH_MESSAGE;
	default:
		//case B_MOUSE_MOVED:
		fLooper.SendMessage(message);
		return B_SKIP_MESSAGE;
	}


#if 0
	if (message->what == B_MOUSE_MOVED) {
	  BMessage *mDown = new BMessage(B_KEY_DOWN);
	  BMessage *mUp;
	  char states[16];
	  mDown->AddInt32("modifiers", 0x0);
	  mDown->AddInt32("key", 94);
	  mDown->AddInt32("raw_char", 32);
	  mDown->AddData("states", 'UBYT', states, sizeof(states));
	  mDown->AddString("bytes", " ");
	  mDown->AddData("byte", 'BYTE', " ", 1);
	  mUp = new BMessage(*mDown);
	  mUp->what = B_KEY_UP;
	  outList->AddItem(mDown);
	  outList->AddItem(mUp);
	}
#endif
	if (message->what == B_MOUSE_DOWN) {
	  int32 buttons;
	  int32 modifiers;
	  BPoint where;
	  if (message->FindInt32("buttons", &buttons) == B_OK) {
	    /* replace first with a button that likely won't exist,
	     * and so shouldn't cause any side effect (hmm err...) */
	    /* XXX: use get_mouse_map() ? */
	    if (buttons == B_PRIMARY_MOUSE_BUTTON)
	      //message->ReplaceInt32("buttons", 0x0000);
	      message->what = B_MOUSE_UP;
	  }
	  
#if 0
	  outList->AddItem(new BMessage(*message));
	  BMessage *m = new BMessage(B_MOUSE_MOVED);
	  if (message->FindInt32("buttons", &buttons) == B_OK)
	    m->AddInt32("buttons", buttons);
	  if (message->FindInt32("modifiers", &modifiers) == B_OK)
	    m->AddInt32("modifiers", modifiers);
	  if (message->FindPoint("where", &where) == B_OK)
	    m->AddPoint("where", where);
	  outList->AddItem(m);
#endif
#if DEBUG
	  fDebugAlert->Lock();
	  fDebugAlert->TextView()->Insert(">>>\n");
	  fDebugAlert->Unlock();
#endif
#if 0
	  m = new BMessage(B_INPUT_METHOD_EVENT);
	  m->AddInt32("be:opcode", B_INPUT_METHOD_STARTED);
	  m->AddMessenger("be:reply_to", this);
	  EnqueueMessage(m);
	  m = new BMessage(B_INPUT_METHOD_EVENT);
	  m->AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
	  m->AddString("be:string", " ");
	  EnqueueMessage(m);
	  m = new BMessage(B_INPUT_METHOD_EVENT);
	  m->AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
	  EnqueueMessage(m);
	  sleep(3);
#endif
#if DEBUG
	  fDebugAlert->Lock();
	  fDebugAlert->TextView()->Insert("<<<\n");
	  fDebugAlert->Unlock();
#endif
	}
	

	
	return (res);
}

status_t PenInputServerMethod::MethodActivated(bool active)
{
	fEnabled = active;

	BMessage msg(MSG_METHOD_ACTIVATED);
	if (active)
		msg.AddBool(MSGF_ACTIVE, true);
	fLooper.SendMessage( &msg );
	
	return B_OK;
}


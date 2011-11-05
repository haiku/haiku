/*
	Copyright 2005, Francois Revol.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Debug.h>
#include <List.h>
#include <Message.h>
#include <OS.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Handler.h>
#include <Locker.h>
#include <Menu.h>
#include <MenuItem.h>
#include <String.h>


#include <add-ons/input_server/InputServerMethod.h>

#ifndef _T
#define _T(s) s
#endif

#include "T9Icon.h"

extern "C" _EXPORT BInputServerMethod* instantiate_input_method();

enum T9Mode {
  WordMode,
  CharMode,
  NumMode
};


// modes: =Abc / Abc / 123
// flags:  Abc / ABC / abc

class T9InputServerMethod : public BInputServerMethod, public BHandler
{
public:
  T9InputServerMethod();
  virtual ~T9InputServerMethod();
  virtual filter_result Filter(BMessage *message, BList *outList);
  virtual status_t MethodActivated(bool active);
  virtual void MessageReceived(BMessage *message);
  
  bool IsEnabled() const { return fEnabled; };
  T9Mode Mode() const { return fMode; };
  void SetMode(T9Mode mode);
private:
  bool fEnabled;
  T9Mode fMode;
  BMenu *fDeskbarMenu;
  BLocker fLocker;
  BString fTyped; /* what has been typed so far for the current word */
};




BInputServerMethod* instantiate_input_method()
{
	PRINT(("%s\n", __FUNCTION__));
	return (new T9InputServerMethod());
}


T9InputServerMethod::T9InputServerMethod()
  : BInputServerMethod("T9", T9IconData),
    BHandler("T9IMHandler"),
    fEnabled(false),
    fMode(WordMode),
    fDeskbarMenu(NULL),
    fLocker("T9IM")
{
	PRINT(("%s\n", __FUNCTION__));

	if (be_app) {
	  be_app->Lock();
	  be_app->AddHandler(this);
	  be_app->Unlock();
	}
	
	//
	fDeskbarMenu = new BMenu(_T("Mode"));
	
	
	BMessage *msg = new BMessage('SetM');
	msg->AddInt32("t9mode", WordMode);
	BMenuItem *item;
	item = new BMenuItem(_T("Word mode"), msg);
	item->SetMarked(true);
	fDeskbarMenu->AddItem(item);
	msg = new BMessage('SetM');
	msg->AddInt32("t9mode", CharMode);
	item = new BMenuItem(_T("Character mode"), msg);
	fDeskbarMenu->AddItem(item);
	msg = new BMessage('SetM');
	msg->AddInt32("t9mode", NumMode);
	item = new BMenuItem(_T("Numeric mode"), msg);
	fDeskbarMenu->AddItem(item);
	fDeskbarMenu->SetFont(be_plain_font);
	// doesn't seem to work here
	//fDeskbarMenu->SetRadioMode(true);
	//fDeskbarMenu->SetLabelFromMarked(true);
	
	SetMenu(fDeskbarMenu, BMessenger(this));
}

T9InputServerMethod::~T9InputServerMethod()
{
	PRINT(("%s\n", __FUNCTION__));
	SetMenu(NULL, BMessenger());
	delete fDeskbarMenu;
	if (be_app) {
	  be_app->Lock();
	  be_app->RemoveHandler(this);
	  be_app->Unlock();
	}
}

filter_result T9InputServerMethod::Filter(BMessage *message, BList *outList)
{
	//status_t err;
	filter_result res = B_DISPATCH_MESSAGE;
	bool keyUp = false;
	BString bytes;

	if (!IsEnabled())
		return B_DISPATCH_MESSAGE;

	switch (message->what) {
	case B_KEY_UP:
	  keyUp = true;
	case B_KEY_DOWN:
	  if (message->FindString("bytes", &bytes) < B_OK)
	    break;
	  if (bytes.Length() == 1) {
	    BAutolock l(fLocker);
	    
	    if (fMode == NumMode)
	      break;
	  }
	  break;
	default:
	  break;
	}
	
	return (res);
}

status_t T9InputServerMethod::MethodActivated(bool active)
{
  fEnabled = active;
  return BInputServerMethod::MethodActivated(active);
}

void T9InputServerMethod::MessageReceived(BMessage *message)
{
  int32 v;
  switch (message->what) {
  case 'SetM':
    if (message->FindInt32("t9mode", &v) < B_OK)
      break;
    SetMode((T9Mode)v);
    
    /*{
      BString s;
      s << v;
      s << " - ";
      s << (long) fDeskbarMenu->FindMarked();
      s << " - ";
      s << (long) fDeskbarMenu->ItemAt(v);
      BAlert *a = new BAlert("Plop", s.String(), "OK");
      a->Go(NULL);
      }*/
    break;
  default:
    BHandler::MessageReceived(message);
    break;
  }
}

void T9InputServerMethod::SetMode(T9Mode mode)
{
  BAutolock l(fLocker);
  BMenuItem *item;
  // XXX: check
  fMode = mode;
  item = fDeskbarMenu->FindMarked();
  if (item)
    item->SetMarked(false);
  item = fDeskbarMenu->ItemAt((int32)mode);
  if (item)
    item->SetMarked(true);
  // necessary to update the copy used by the Deskbar icon.
  SetMenu(fDeskbarMenu, BMessenger(this));
}

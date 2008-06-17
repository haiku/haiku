/*
	Copyright 2007, Francois Revol.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _PEN_INPUT_LOOPER_H
#define _PEN_INPUT_LOOPER_H

#include <MessageRunner.h>
#include <Messenger.h>
#include <Looper.h>

/* internal messages */

#define MSG_METHOD_ACTIVATED 'IMAc'
#define MSGF_ACTIVE "active" /* bool */

#define MSG_BEGIN_INK 'InkB'
#define MSG_END_INK 'InkE'

#define MSG_SHOW_WIN 'ShoW'
#define MSG_HIDE_WIN 'HidW'

#define MSG_CHECK_PEN_DOWN 'ChkP'

/* menu messages */

#define MSG_SET_BACKEND 'SetB'
#define MSGF_BACKEND "backend" /* string */

#define MSG_SHOW_INK 'InkS' /* toggle */


class BMenu;
class BWindow;
class PenInputServerMethod;
class PenInputInkWindow;
class PenInputBackend;

class PenInputLooper : public BLooper
{
public:
	PenInputLooper(PenInputServerMethod *method);
	virtual void	Quit();
	void			DispatchMessage(BMessage *message, BHandler *handler);
	void			MessageReceived(BMessage *message);
	void			EnqueueMessage(BMessage *message);
	status_t		InitCheck();
	
//	virtual ~PenInputLooper();
	void MethodActivated(bool active);
	
private:
	friend class PenInputInkWindow;
	void			HandleMethodActivated(bool active);
	PenInputServerMethod *fOwner;
	PenInputInkWindow *fInkWindow;
	PenInputBackend *fBackend;
	BMenu *fDeskbarMenu;
	BMessenger fInkWindowMsgr;
	bool fMouseDown;
	bool fStroking;
	BMessageRunner *fThresholdRunner;
	BMessage *fCachedMouseDown;
	/* config */
	bool fShowInk;
	bigtime_t fMouseDownThreshold;
};

#endif /* _PEN_INPUT_LOOPER_H */

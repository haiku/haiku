//
//	CannaLooper.h
//	CannaIM Looper object

//	This is a part of...
//	CannaIM
//	version 1.0
//	(c) 1999 M.Kawamura
//

#ifndef _CANNA_LOOPER_H
#define _CANNA_LOOPER_H

#include <Messenger.h>
#include <Font.h>
#include <Looper.h>
#include "CannaInterface.h"

class CannaMethod;
class KouhoWindow;
class PaletteWindow;
class BMenu;
class BMessenger;

extern Preferences gSettings;

class CannaLooper : public BLooper
{
public:
					CannaLooper( CannaMethod *method );
	virtual void	Quit();
	void			MessageReceived( BMessage *msg );
	void			EnqueueMessage( BMessage *msg );
	status_t		InitCheck();
	status_t		ReadSettings( char *basepath );
//	bool			QuitRequested();
	void			SendInputStopped();
	void			SendInputStarted();
	
private:
	CannaMethod*	owner;
	CannaInterface*	canna;
	BFont			kouhoFont;
	void			HandleKeyDown( BMessage *msg );
	void			HandleLocationReply( BMessage *msg );
	void			HandleMethodActivated( bool active );
	void			ProcessResult( uint32 result );
	void			ForceKakutei();
	KouhoWindow*	theKouhoWindow;
	PaletteWindow*	thePalette;
	BMenu*			theMenu;
	BMessenger		self;
};

#endif

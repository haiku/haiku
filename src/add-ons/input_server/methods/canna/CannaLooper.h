/*
 * Copyright 2007-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 1999 M.Kawamura
 */


#ifndef CANNA_LOOPER_H
#define CANNA_LOOPER_H


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

class CannaLooper : public BLooper {
public:
							CannaLooper(CannaMethod* method);

	virtual void			Quit();
	virtual void			MessageReceived(BMessage* msg);

			void			EnqueueMessage(BMessage* msg);
			status_t		Init();
			status_t		ReadSettings(char* basePath);

			void			SendInputStopped();
			void			SendInputStarted();

private:
			void			_HandleKeyDown(BMessage* msg);
			void			_HandleLocationReply(BMessage* msg);
			void			_HandleMethodActivated(bool active);
			void			_ProcessResult(uint32 result);
			void			_ForceKakutei();

			CannaMethod*	fOwner;
			CannaInterface*	fCanna;
			BFont			fKouhoFont;
			KouhoWindow*	fKouhoWindow;
			PaletteWindow*	fPaletteWindow;
			BMenu*			fMenu;
};

#endif	// CANNA_LOOPER_H

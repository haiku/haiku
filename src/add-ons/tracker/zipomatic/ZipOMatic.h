#ifndef _ZIPOMATIC_H_
#define _ZIPOMATIC_H_


#include <Application.h>
#include <Invoker.h>
#include <Message.h>
#include <Rect.h>

#include "ZipOMaticSettings.h"


class ZipOMatic : public BApplication
{
public:
							ZipOMatic();
							~ZipOMatic();

	virtual	void			ReadyToRun();
	virtual	void			RefsReceived(BMessage* message);
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	
private:
			status_t		_ReadSettings();
			status_t		_WriteSettings();
			void			_CascadeOnFrameCollision(BRect* frame);
			void			_SilentRelaunch();
			void			_UseExistingOrCreateNewWindow(BMessage*
								message = NULL);
			void			_StopZipping();

			ZippoSettings	fSettings;
			bool			fGotRefs;
			BInvoker*		fInvoker;
			BRect			fWindowFrame;
};

#endif // _ZIPOMATIC_H_


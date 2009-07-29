#ifndef _ZIPOMATIC_WINDOW_H
#define _ZIPOMATIC_WINDOW_H


#include <Bitmap.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

#include "ZipOMaticSettings.h"
#include "ZipOMaticView.h"
#include "ZipperThread.h"


class ZippoWindow : public BWindow
{
public:
							ZippoWindow(BMessage* message = NULL);
							~ZippoWindow();
							
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	virtual	void			Zoom(BPoint origin, float width, float height);
	
			bool			IsZipping();
			
private:
			status_t		_ReadSettings();
			status_t		_WriteSettings();

			void			_StartZipping(BMessage* message);
			void			_StopZipping();
				
			void			_CloseWindowOrKeepOpen();

			ZippoView*		fView;
			ZippoSettings	fSettings;
			ZipperThread*	fThread;
	
			bool			fWindowGotRefs;
			bool			fZippingWasStopped;
			
			BInvoker*		fWindowInvoker;
};

#endif	// _ZIPOMATIC_WINDOW_H


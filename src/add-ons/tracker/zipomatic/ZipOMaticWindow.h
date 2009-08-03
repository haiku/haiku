#ifndef _ZIPOMATIC_WINDOW_H
#define _ZIPOMATIC_WINDOW_H


#include <Bitmap.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>

#include "ZipOMaticView.h"
#include "ZipperThread.h"


class ZippoWindow : public BWindow
{
public:
							ZippoWindow(BRect frame, BMessage* refs = NULL);
							~ZippoWindow();
							
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	virtual	void			Zoom(BPoint origin, float width, float height);
	
			bool			IsZipping();
			void			StopZipping();
			
private:

			void			_StartZipping(BMessage* message);
			void			_CloseWindowOrKeepOpen();

			ZippoView*		fView;
			ZipperThread*	fThread;
	
			bool			fWindowGotRefs;
			bool			fZippingWasStopped;
			
			BInvoker*		fWindowInvoker;
};

#endif	// _ZIPOMATIC_WINDOW_H


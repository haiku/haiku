#ifndef _ZIPOMATIC_WINDOW_H
#define _ZIPOMATIC_WINDOW_H


#include <Button.h>
#include <StringView.h>
#include <Window.h>

#include "ZipOMaticActivity.h"
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

			Activity*		fActivityView;
			BStringView*	fArchiveNameView;
			BStringView*	fZipOutputView;
			BButton*		fStopButton;

			ZipperThread*	fThread;
	
			bool			fWindowGotRefs;
			bool			fZippingWasStopped;
			int32			fFileCount;
			
			BInvoker*		fWindowInvoker;
};

#endif	// _ZIPOMATIC_WINDOW_H


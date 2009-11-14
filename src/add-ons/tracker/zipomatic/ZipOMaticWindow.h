#ifndef _ZIPOMATIC_WINDOW_H
#define _ZIPOMATIC_WINDOW_H


#include <Button.h>
#include <List.h>
#include <Rect.h>
#include <StringView.h>
#include <Window.h>

#include "ZipOMaticActivity.h"
#include "ZipperThread.h"


enum direction {
	up,
	down,
	left,
	right
};


class ZippoWindow : public BWindow
{
public:
							ZippoWindow(BList windowList,
								bool keepOpen = false);
							~ZippoWindow();
							
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	
			bool			IsZipping();
			void			StopZipping();
			
private:

			void			_StartZipping(BMessage* message);
			void			_CloseWindowOrKeepOpen();

			void			_FindBestPlacement();
			void			_OffsetRect(BRect* rect, direction whereTo);
			void			_OffscreenBounceBack(BRect* rect,
								direction* primaryDirection,
								direction secondaryDirection);
			BRect			_NearestRect(BRect goalRect, BRect a, BRect b);

			BList			fWindowList;

			Activity*		fActivityView;
			BStringView*	fArchiveNameView;
			BStringView*	fZipOutputView;
			BButton*		fStopButton;

			ZipperThread*	fThread;
			BString			fArchiveName;
	
			bool			fKeepOpen;
			bool			fZippingWasStopped;
			int32			fFileCount;
			
			BInvoker*		fWindowInvoker;
};

#endif	// _ZIPOMATIC_WINDOW_H


/*
	StatusWindow.h
  Copyright 2005 Dr.H.Reh. All Rights Reserved.
*/


#ifndef STATUS_WINDOW_H										
#define STATUS_WINDOW_H										

#include <Window.h>												
#include <StatusBar.h>
#include <String.h>

class StatusWindow : public BWindow 
{
public:
						StatusWindow(bool oddPages, bool evenPages,
								uint32 firstPage, uint32 numPages,
								uint32 docCopies, uint32 nup);
						~StatusWindow(void);

	virtual	void		MessageReceived(BMessage *message);
		
			void		ResetStatusBar(void);
			bool		UpdateStatusBar(uint32 page, uint32 copy);
			void		SetPageCopies(uint32 copies);
		
private:
			BStatusBar*	fStatusBar;
			BButton*	fCancelButton;
			BButton*	fHideButton;
			bool		fCancelled;
			bool		fDocumentCopy;
			uint32		fNops;
			uint32		fFirstPage;
			uint32		fCopies;
			uint32		fDocCopies;
			float		fStatusDelta;
			float		fDelta;
};

#endif


#ifndef REFRESHWINDOW_H
#define REFRESHWINDOW_H

#include <Window.h>

#include "RefreshView.h"
#include "RefreshSlider.h"

class RefreshWindow : public BWindow
{

public:
						RefreshWindow(BRect frame, int32 value);
	virtual	bool		QuitRequested();
	virtual void 		MessageReceived(BMessage *message);
	virtual void		WindowActivated(bool active);

private:
	RefreshView			*fRefreshView;
	RefreshSlider		*fRefreshSlider;
	BButton				*fDoneButton;
	BButton				*fCancelButton;
};

#endif
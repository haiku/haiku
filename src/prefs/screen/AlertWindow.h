#ifndef ALERTWINDOW_H
#define ALERTWINDOW_H

#include "AlertView.h"

class AlertWindow : public BWindow
{

public:
					AlertWindow(BRect frame);
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage *message);

private:
	AlertView		*fAlertView;
	BMessageRunner	*fRunner;
	BButton			*fRevertButton;
	BButton			*fKeepButton;
};

#endif
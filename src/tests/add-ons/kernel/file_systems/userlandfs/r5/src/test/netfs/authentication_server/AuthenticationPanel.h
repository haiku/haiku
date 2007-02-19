// AuthenticationPanel.h

#ifndef AUTHENTICATION_PANEL_H
#define AUTHENTICATION_PANEL_H

#include "Panel.h"

class BCheckBox;
class BTextControl;

class AuthenticationPanel : public Panel {
 public:
							AuthenticationPanel(BRect frame = BRect(-1000.0, -1000.0, -900.0, -900.0));
	virtual					~AuthenticationPanel();

	virtual	bool			QuitRequested();

	virtual void			MessageReceived(BMessage *message);

							// AuthenticationPanel
			bool			GetAuthentication(const char* server,
											  const char* share,
											  const char* previousUser,
											  const char* previousPass,
											  bool previousKeep,
											  bool badPassword,
											  char* user,
											  char* pass,
											  bool* askAgain);

	virtual	void	Cancel();

 private:
			BRect			_CalculateFrame(BRect frame);


	BTextControl*			fNameTC;
	BTextControl*			fPassTC;
	BCheckBox*				fKeepUsingCB;

	BButton*				fOkB;
	BButton*				fCancelB;

	bool					fCancelled;

	sem_id					fExitSem;
};

#endif // AUTHENTICATION_PANEL_H

/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef START_TEAM_WINDOW_H
#define START_TEAM_WINDOW_H


#include <Window.h>


class BButton;
class BFilePanel;
class BStringView;
class BTextControl;


class StartTeamWindow : public BWindow
{
public:
								StartTeamWindow();

								~StartTeamWindow();

	static	StartTeamWindow*	Create();
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

private:
			void	 			_Init();


private:
			BStringView*		fGuideText;
			BTextControl*		fTeamTextControl;
			BTextControl*		fArgumentsTextControl;
			BButton*			fBrowseTeamButton;
			BFilePanel*			fBrowseTeamPanel;
			BButton*			fStartButton;
			BButton*			fCancelButton;
};

#endif // START_TEAM_WINDOW_H

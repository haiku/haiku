/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef _JOY_WIN_H
#define _JOY_WIN_H


#include <Window.h>
#include <Joystick.h>

class BView;
class BCheckBox;
class BStringView;
class BListView;
class BButton;
class BBox;


class JoyWin : public BWindow
{
	public:
		JoyWin(BRect frame,const char *title,
			window_look look,
			window_feel feel,
			uint32 flags,
			uint32 workspace = B_CURRENT_WORKSPACE);

		virtual	void	MessageReceived(BMessage *message);
		virtual	bool	QuitRequested();

	protected:
		BJoystick		fJoystick;

		BBox*		    fBox;
		BCheckBox*		fCheckbox;

		BStringView*	fGamePortS;
		BStringView*	fConControllerS;

		BListView*		fGamePortL;
		BListView*		fConControllerL;

		BButton*		fCalibrateButton;
		BButton*		fProbeButton;

		status_t		AddDevices();
		status_t		AddJoysticks();
		status_t		Calibrate();
		status_t		PerformProbe();
		status_t		ApplyChanges();
		status_t		GetSettings();

};

#endif	/* _JOY_WIN_H */


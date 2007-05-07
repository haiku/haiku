/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef _CALIB_WIN_H
#define _CALIB_WIN_H


#include <Window.h>

class BView;
class BCheckBox;
class BStringView;
class BButton;
class BBox;


/*
	All this code is here is just to not have an empty view at
	Clicking the Calibrate function.
	
	All controls in this view needs to be created and placed dynamically according
	with the Joystick descriptors
*/


class CalibWin : public BWindow
{
	public:
		CalibWin(BRect frame, const char *title,
			window_look look,
			window_feel feel,
			uint32 flags,
			uint32 workspace = B_CURRENT_WORKSPACE);
							
		virtual	void	MessageReceived(BMessage *message);
		virtual	bool	QuitRequested();

	protected:
		BStringView*	fStringView3;
		BStringView*	fStringView4;
		BStringView*	fStringView5;
		BStringView*	fStringView6;
		BStringView*	fStringView7;
		BStringView*	fStringView8;
		BStringView*	fStringView9;

		BButton*		fButton3;
		BButton*		fButton4;

		BButton*		fButton5;
		BButton*		fButton6;
		BButton*		fButton7;
		BButton*		fButton8;
		BButton*		fButton9;
		BButton*		fButton10;
		BButton*		fButton11;
		BButton*		fButton12;

		BBox*			fBox;
		BView*			fView;
};


#endif	/* _CALIB_WIN_H */


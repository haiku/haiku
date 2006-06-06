/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#ifndef _CALC_WINDOW_H
#define _CALC_WINDOW_H

#include <Window.h>

class CalcView;

class CalcWindow : public BWindow {
 public:
								CalcWindow(BRect frame);
	virtual						~CalcWindow();

	virtual	void				Show();
	virtual	bool				QuitRequested();

			bool				LoadSettings(BMessage* archive);
			status_t			SaveSettings(BMessage* archive) const;

			void				SetFrame(BRect frame,
										 bool forceCenter = false);

 private:
			CalcView*			fCalcView;
};

#endif // _CALC_WINDOW_H

/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef _CALC_OPTIONS_WINDOW_H
#define _CALC_OPTIONS_WINDOW_H

#include <Window.h>

struct CalcOptions {
	bool mAutoNumLock;		// automatically activate numlock
	bool mAudioFeedback;	// provide audio feedback

	CalcOptions();
};

class BCheckBox;
class BButton;

class CalcOptionsWindow : public BWindow {
 public:		
								CalcOptionsWindow(BRect frame,
												  CalcOptions* options);

	virtual						~CalcOptionsWindow();
		
 private:
			CalcOptions*		fOptions;

			BCheckBox*			fAutoNumLockCheckBox;
			BCheckBox*			fAudioFeedbackCheckBox;
			BButton*			fOkButton;
			BButton*			fCancelButton;
};

#endif // _CALC_OPTIONS_WINDOW_H

/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CALC_WINDOW_H
#define _CALC_WINDOW_H


#include <Window.h>


class CalcView;

class CalcWindow : public BWindow {
 public:
								CalcWindow(BRect frame, BMessage* settings);
	virtual						~CalcWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Show();
	virtual	bool				QuitRequested();

			status_t			SaveSettings(BMessage* archive) const;

			void				SetFrame(BRect frame,
									bool forceCenter = false);

 private:
			CalcView*			fCalcView;
};

#endif // _CALC_WINDOW_H

/*
 * Copyright 2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */

#ifndef PROGRESS_STATUS_VIEW_H
#define PROGRESS_STATUS_VIEW_H


#include <View.h>


class BarberPole;
class BCardLayout;
class BStatusBar;
class BStringView;


class ProgressStatusView : public BView {
public:
					ProgressStatusView(const char* name = "load_progress");

	virtual	void	AttachedToWindow();

	virtual void	Pulse();
	virtual void	Draw(BRect updateRect);

	void			SetBusy();
	void			SetBusy(const BString& text);
	void			SetIdle();

	void			SetProgress(float progress);
	void			SetText(const BString& text);

private:
	void			_ValidatePreferredSize();

	BStatusBar*		fProgressBar;
	BarberPole*		fBarberPole;
	BCardLayout*	fProgressLayout;
	BView*			fProgressView;

	float			fMaxHeight;
	BSize			fBarSize;
	BSize			fPreferredSize;
	BString			fStatusText;
	bigtime_t		fLastPercentReceivedTime;
};


#endif // PROGRESS_STATUS_VIEW_H

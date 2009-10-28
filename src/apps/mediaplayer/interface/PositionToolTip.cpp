/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "PositionToolTip.h"

#include <stdio.h>

#include <StringView.h>


class PositionToolTip::PositionView : public BStringView {
public:
	PositionView()
		:
		BStringView("position", ""),
		fPosition(0),
		fDuration(0)
	{
	}

	virtual ~PositionView()
	{
	}

	virtual void AttachedToWindow()
	{
		BStringView::AttachedToWindow();
		Update(-1, -1);
	}

	void Update(bigtime_t position, bigtime_t duration)
	{
		if (!LockLooper())
			return;

		if (position != -1) {
			position /= 1000000L;
			duration /= 1000000L;
			if (position == fPosition && duration == fDuration) {
				UnlockLooper();
				return;
			}

			fPosition = position;
			fDuration = duration;
		}

		char text[64];
		snprintf(text, sizeof(text), "%02ld:%02ld / %02ld:%02ld",
			fPosition / 60, fPosition % 60, fDuration / 60, fDuration % 60);
		SetText(text);

		UnlockLooper();
	}

private:
	time_t		fPosition;
	time_t		fDuration;
};


// #pragma mark -


PositionToolTip::PositionToolTip()
{
	fView = new PositionView();
}


PositionToolTip::~PositionToolTip()
{
	delete fView;
}


BView*
PositionToolTip::View() const
{
	return fView;
}


void
PositionToolTip::Update(bigtime_t position, bigtime_t duration)
{
	fView->Update(position, duration);
}

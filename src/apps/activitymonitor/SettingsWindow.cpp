/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsWindow.h"

#include <stdio.h>

#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Slider.h>
#include <String.h>


static const uint32 kMsgUpdateTimeInterval = 'upti';

static const bigtime_t kUpdateIntervals[] = { 50, 100, 250, 500, 1000, 2000 };


class IntervalSlider : public BSlider {
public:
	IntervalSlider(const char* label, BMessage* message, uint32 levels)
		: BSlider("intervalSlider", label, message, 0, levels - 1, B_HORIZONTAL)
	{
		BString min(_TextFor(0));
		BString max(_TextFor(levels - 1));
		SetLimitLabels(min.String(), max.String());
		SetHashMarks(B_HASH_MARKS_BOTTOM);
		SetHashMarkCount(levels);

		if (message != NULL)
			SetModificationMessage(new BMessage(*message));
	}

	void SetInterval(bigtime_t interval)
	{
		interval /= 1000;

		// Find closest index
		int32 bestDiff = LONG_MAX;
		uint32 bestIndex = 0;
		for (uint32 i = 0;
				i < sizeof(kUpdateIntervals) / sizeof(kUpdateIntervals[0]);
				i++) {
			int32 diff = abs(kUpdateIntervals[i] - interval);
			if (diff < bestDiff) {
				bestDiff = diff;
				bestIndex = i;
			}
		}

		SetValue(bestIndex);
	}

	virtual const char* UpdateText() const
	{
		return _TextFor(Value());
	}

private:
	const char* _TextFor(uint32 level) const
	{
		if (level >= sizeof(kUpdateIntervals) / sizeof(kUpdateIntervals[0]))
			return NULL;

		bigtime_t interval = kUpdateIntervals[level];
		if ((interval % 1000) == 0)
			snprintf(fText, sizeof(fText), "%lld secs", interval / 1000);
		else
			snprintf(fText, sizeof(fText), "%lld msecs", interval);

		return fText;
	}

	mutable char	fText[64];
};


//	#pragma mark -


SettingsWindow::SettingsWindow(ActivityWindow* target)
	: BWindow(_RelativeTo(target), "Settings", B_FLOATING_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fIntervalSlider = new IntervalSlider("Update time interval:",
		new BMessage(kMsgUpdateTimeInterval), 6);
	fIntervalSlider->SetInterval(target->RefreshInterval());

	// controls pane
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fIntervalSlider)
		.SetInsets(10, 10, 10, 10)
	);
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateTimeInterval:
		{
			int32 level = 0;
			if (message->FindInt32("be:value", &level) != B_OK)
				break;

			BMessage update(kMsgTimeIntervalUpdated);
			update.AddInt64("interval", kUpdateIntervals[level] * 1000LL);

			fTarget.SendMessage(&update);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
SettingsWindow::QuitRequested()
{
	return true;
}


BRect
SettingsWindow::_RelativeTo(BWindow* window)
{
	BRect frame = window->Frame();
	return BRect(frame.right - 150, frame.top + frame.Height() / 4,
		frame.right + 200, frame.top + frame.Height() / 4 + 50);
}


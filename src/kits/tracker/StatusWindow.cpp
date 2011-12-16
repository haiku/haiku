/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

/*!	A subclass of BWindow that is used to display the status of the Tracker
	operations (copying, deleting, etc.).
*/


#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <DurationFormat.h>
#include <Locale.h>
#include <MessageFilter.h>
#include <StringView.h>
#include <String.h>

#include <string.h>

#include "AutoLock.h"
#include "Bitmaps.h"
#include "Commands.h"
#include "StatusWindow.h"
#include "StringForSize.h"
#include "DeskWindow.h"


const float	kDefaultStatusViewHeight = 50;
const bigtime_t kMaxUpdateInterval = 100000LL;
const bigtime_t kSpeedReferenceInterval = 2000000LL;
const bigtime_t kShowSpeedInterval = 8000000LL;
const bigtime_t kShowEstimatedFinishInterval = 4000000LL;
const BRect kStatusRect(200, 200, 550, 200);

static bigtime_t sLastEstimatedFinishSpeedToggleTime = -1;
static bool sShowSpeed = true;
static const time_t kSecondsPerDay = 24 * 60 * 60;

class TCustomButton : public BButton {
public:
								TCustomButton(BRect frame, uint32 command);
	virtual	void				Draw(BRect updateRect);
private:
			typedef BButton _inherited;
};


class BStatusMouseFilter : public BMessageFilter {
public:
								BStatusMouseFilter();
	virtual	filter_result		Filter(BMessage* message, BHandler** target);
};


namespace BPrivate {
BStatusWindow *gStatusWindow = NULL;
}


BStatusMouseFilter::BStatusMouseFilter()
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_MOUSE_DOWN)
{
}


filter_result
BStatusMouseFilter::Filter(BMessage* message, BHandler** target)
{
	// If the target is the status bar, make sure the message goes to the
	// parent view instead.
	if ((*target)->Name() != NULL
		&& strcmp((*target)->Name(), "StatusBar") == 0) {
		BView* view = dynamic_cast<BView*>(*target);
		if (view != NULL)
			view = view->Parent();
		if (view != NULL)
			*target = view;
	}

	return B_DISPATCH_MESSAGE;
}


TCustomButton::TCustomButton(BRect frame, uint32 what)
	:
	BButton(frame, "", "", new BMessage(what), B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW)
{
}


void
TCustomButton::Draw(BRect updateRect)
{
	_inherited::Draw(updateRect);

	if (Message()->what == kStopButton) {
		updateRect = Bounds();
		updateRect.InsetBy(9, 8);
		SetHighColor(0, 0, 0);
		if (Value() == B_CONTROL_ON)
			updateRect.OffsetBy(1, 1);
		FillRect(updateRect);
	} else {
		updateRect = Bounds();
		updateRect.InsetBy(9, 7);
		BRect rect(updateRect);
		rect.right -= 3;

		updateRect.left += 3;
		updateRect.OffsetBy(1, 0);
		SetHighColor(0, 0, 0);
		if (Value() == B_CONTROL_ON) {
			updateRect.OffsetBy(1, 1);
			rect.OffsetBy(1, 1);
		}
		FillRect(updateRect);
		FillRect(rect);
	}
}


// #pragma mark -


class StatusBackgroundView : public BView {
public:
	StatusBackgroundView(BRect frame)
		: BView(frame, "BackView", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	virtual void Pulse()
	{
		bigtime_t now = system_time();
		if (sShowSpeed
			&& sLastEstimatedFinishSpeedToggleTime + kShowSpeedInterval
				<= now) {
			sShowSpeed = false;
			sLastEstimatedFinishSpeedToggleTime = now;
		} else if (!sShowSpeed
			&& sLastEstimatedFinishSpeedToggleTime
				+ kShowEstimatedFinishInterval <= now) {
			sShowSpeed = true;
			sLastEstimatedFinishSpeedToggleTime = now;
		}
	}
};


// #pragma mark - BStatusWindow


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "StatusWindow"


BStatusWindow::BStatusWindow()
	:
	BWindow(kStatusRect, B_TRANSLATE("Tracker status"),	B_TITLED_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
	fRetainDesktopFocus(false)
{
	SetSizeLimits(0, 100000, 0, 100000);
	fMouseDownFilter = new BStatusMouseFilter();
	AddCommonFilter(fMouseDownFilter);

	BView* view = new StatusBackgroundView(Bounds());
	AddChild(view);

	SetPulseRate(1000000);

	Hide();
	Show();
}


BStatusWindow::~BStatusWindow()
{
}


void
BStatusWindow::CreateStatusItem(thread_id thread, StatusWindowState type)
{
	AutoLock<BWindow> lock(this);

	BRect rect(Bounds());
	if (BStatusView* lastView = fViewList.LastItem())
		rect.top = lastView->Frame().bottom + 1;
	else {
		// This is the first status item, reset speed/estimated finish toggle.
		sShowSpeed = true;
		sLastEstimatedFinishSpeedToggleTime = system_time();
	}
	rect.bottom = rect.top + kDefaultStatusViewHeight - 1;

	BStatusView* view = new BStatusView(rect, thread, type);
	// the BStatusView will resize itself if needed in its constructor
	ChildAt(0)->AddChild(view);
	fViewList.AddItem(view);

	ResizeTo(Bounds().Width(), view->Frame().bottom);

	// find out if the desktop is the active window
	// if the status window is the only thing to take over active state and
	// desktop was active to begin with, return focus back to desktop
	// when we are done
	bool desktopActive = false;
	{
		AutoLock<BLooper> lock(be_app);
		int32 count = be_app->CountWindows();
		for (int32 index = 0; index < count; index++) {
			if (dynamic_cast<BDeskWindow *>(be_app->WindowAt(index))
				&& be_app->WindowAt(index)->IsActive()) {
				desktopActive = true;
				break;
			}
		}
	}

	if (IsHidden()) {
		fRetainDesktopFocus = desktopActive;
		Minimize(false);
		Show();
	} else
		fRetainDesktopFocus &= desktopActive;
}


void
BStatusWindow::InitStatusItem(thread_id thread, int32 totalItems,
	off_t totalSize, const entry_ref* destDir, bool showCount)
{
	AutoLock<BWindow> lock(this);

	int32 numItems = fViewList.CountItems();
	for (int32 index = 0; index < numItems; index++) {
		BStatusView* view = fViewList.ItemAt(index);
		if (view->Thread() == thread) {
			view->InitStatus(totalItems, totalSize, destDir, showCount);
			break;
		}
	}

}


void
BStatusWindow::UpdateStatus(thread_id thread, const char* curItem,
	off_t itemSize, bool optional)
{
	AutoLock<BWindow> lock(this);

	int32 numItems = fViewList.CountItems();
	for (int32 index = 0; index < numItems; index++) {
		BStatusView* view = fViewList.ItemAt(index);
		if (view->Thread() == thread) {
			view->UpdateStatus(curItem, itemSize, optional);
			break;
		}
	}
}


void
BStatusWindow::RemoveStatusItem(thread_id thread)
{
	AutoLock<BWindow> lock(this);
	BStatusView* winner = NULL;

	int32 numItems = fViewList.CountItems();
	int32 index;
	for (index = 0; index < numItems; index++) {
		BStatusView* view = fViewList.ItemAt(index);
		if (view->Thread() == thread) {
			winner = view;
			break;
		}
	}

	if (winner != NULL) {
		// The height by which the other views will have to be moved (in pixel
		// count).
		float height = winner->Bounds().Height() + 1;
		fViewList.RemoveItem(winner);
		winner->RemoveSelf();
		delete winner;

		if (--numItems == 0 && !IsHidden()) {
			BDeskWindow* desktop = NULL;
			if (fRetainDesktopFocus) {
				AutoLock<BLooper> lock(be_app);
				int32 count = be_app->CountWindows();
				for (int32 index = 0; index < count; index++) {
					desktop = dynamic_cast<BDeskWindow*>(
						be_app->WindowAt(index));
					if (desktop != NULL)
						break;
				}
			}
			Hide();
			if (desktop != NULL) {
				// desktop was active when we first started,
				// make it active again
				desktop->Activate();
			}
		}

		for (; index < numItems; index++)
			fViewList.ItemAt(index)->MoveBy(0, -height);

		ResizeTo(Bounds().Width(), Bounds().Height() - height);
	}
}


bool
BStatusWindow::CheckCanceledOrPaused(thread_id thread)
{
	bool wasCanceled = false;
	bool isPaused = false;

	BStatusView* view = NULL;

	AutoLock<BWindow> lock(this);
	// check if cancel or pause hit
	for (int32 index = fViewList.CountItems() - 1; index >= 0; index--) {
		view = fViewList.ItemAt(index);
		if (view && view->Thread() == thread) {
			isPaused = view->IsPaused();
			wasCanceled = view->WasCanceled();
			break;
		}
	}

	if (wasCanceled || !isPaused)
		return wasCanceled;

	if (isPaused && view != NULL) {
		// say we are paused
		view->Invalidate();
		thread_id thread = view->Thread();

		lock.Unlock();

		// and suspend ourselves
		// we will get resumed from BStatusView::MessageReceived
		ASSERT(find_thread(NULL) == thread);
		suspend_thread(thread);
	}

	return wasCanceled;
}


bool
BStatusWindow::AttemptToQuit()
{
	// called when tracker is quitting
	// try to cancel all the move/copy/empty trash threads in a nice way
	// by issuing cancels
	int32 count = fViewList.CountItems();

	if (count == 0)
		return true;

	for (int32 index = 0; index < count; index++)
		fViewList.ItemAt(index)->SetWasCanceled();

	// maybe next time everything will have been canceled
	return false;
}


void
BStatusWindow::WindowActivated(bool state)
{
	if (!state)
		fRetainDesktopFocus = false;

	return _inherited::WindowActivated(state);
}


// #pragma mark - BStatusView


BStatusView::BStatusView(BRect bounds, thread_id thread,
	StatusWindowState type)
	:
	BView(bounds, "StatusView", B_FOLLOW_NONE, B_WILL_DRAW),
	fType(type),
	fBitmap(NULL),
	fThread(thread)
{
	Init();

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());
	SetHighColor(20, 20, 20);
	SetDrawingMode(B_OP_OVER);

	const float buttonWidth = 22;
	const float buttonHeight = 20;

	BRect rect(bounds);
	rect.OffsetTo(B_ORIGIN);
	rect.left += 40;
	rect.right -= buttonWidth * 2 + 12;
	rect.top += 6;
	rect.bottom = rect.top + 15;

	BString caption;
	int32 id = 0;

	switch (type) {
		case kCopyState:
			caption = B_TRANSLATE("Preparing to copy items" B_UTF8_ELLIPSIS);
			id = R_CopyStatusBitmap;
			break;

		case kMoveState:
			caption = B_TRANSLATE("Preparing to move items" B_UTF8_ELLIPSIS);
			id = R_MoveStatusBitmap;
			break;

		case kCreateLinkState:
			caption = B_TRANSLATE("Preparing to create links" B_UTF8_ELLIPSIS);
			id = R_MoveStatusBitmap;
			break;

		case kTrashState:
			caption = B_TRANSLATE("Preparing to empty Trash" B_UTF8_ELLIPSIS);
			id = R_TrashStatusBitmap;
			break;

		case kVolumeState:
			caption = B_TRANSLATE("Searching for disks to mount" B_UTF8_ELLIPSIS);
			break;

		case kDeleteState:
			caption = B_TRANSLATE("Preparing to delete items" B_UTF8_ELLIPSIS);
			id = R_TrashStatusBitmap;
			break;

		case kRestoreFromTrashState:
			caption = B_TRANSLATE("Preparing to restore items" B_UTF8_ELLIPSIS);
			break;

		default:
			TRESPASS();
			break;
	}

	if (caption.Length() != 0) {
		fStatusBar = new BStatusBar(rect, "StatusBar", caption.String());
		fStatusBar->SetBarHeight(12);
		float width, height;
		fStatusBar->GetPreferredSize(&width, &height);
		fStatusBar->ResizeTo(fStatusBar->Frame().Width(), height);
		AddChild(fStatusBar);

		// Figure out how much room we need to display the additional status
		// message below the bar
		font_height fh;
		GetFontHeight(&fh);
		BRect f = fStatusBar->Frame();
		// Height is 3 x the "room from the top" + bar height + room for
		// string.
		ResizeTo(Bounds().Width(), f.top + f.Height() + fh.leading + fh.ascent
			+ fh.descent + f.top);
	}

	if (id != 0)
	 	GetTrackerResources()->GetBitmapResource(B_MESSAGE_TYPE, id, &fBitmap);

	rect = Bounds();
	rect.left = rect.right - buttonWidth * 2 - 7;
	rect.right = rect.left + buttonWidth;
	rect.top = floorf((rect.top + rect.bottom) / 2 + 0.5) - buttonHeight / 2;
	rect.bottom = rect.top + buttonHeight;

	fPauseButton = new TCustomButton(rect, kPauseButton);
	fPauseButton->ResizeTo(buttonWidth, buttonHeight);
	AddChild(fPauseButton);

	rect.OffsetBy(buttonWidth + 2, 0);
	fStopButton = new TCustomButton(rect, kStopButton);
	fStopButton->ResizeTo(buttonWidth, buttonHeight);
	AddChild(fStopButton);
}


BStatusView::~BStatusView()
{
	delete fBitmap;
}


void
BStatusView::Init()
{
	fDestDir = "";
	fCurItem = 0;
	fPendingStatusString[0] = '\0';
	fWasCanceled = false;
	fIsPaused = false;
	fLastUpdateTime = 0;
	fBytesPerSecond = 0.0;
	for (size_t i = 0; i < kBytesPerSecondSlots; i++)
		fBytesPerSecondSlot[i] = 0.0;
	fCurrentBytesPerSecondSlot = 0;
	fItemSize = 0;
	fSizeProcessed = 0;
	fLastSpeedReferenceSize = 0;
	fEstimatedFinishReferenceSize = 0;

	fProcessStartTime = fLastSpeedReferenceTime = fEstimatedFinishReferenceTime
		= system_time();
}


void
BStatusView::InitStatus(int32 totalItems, off_t totalSize,
	const entry_ref* destDir, bool showCount)
{
	Init();
	fTotalSize = totalSize;
	fShowCount = showCount;

	BEntry entry;
	char name[B_FILE_NAME_LENGTH];
	if (destDir && (entry.SetTo(destDir) == B_OK)) {
		entry.GetName(name);
		fDestDir = name;
	}

	BString buffer;
	if (totalItems > 0) {
		char totalStr[32];
		buffer.SetTo(B_TRANSLATE("of %items"));
		snprintf(totalStr, sizeof(totalStr), "%ld", totalItems);
		buffer.ReplaceFirst("%items", totalStr);
	}

	switch (fType) {
		case kCopyState:
			fStatusBar->Reset(B_TRANSLATE("Copying: "),	buffer.String());
			break;

		case kCreateLinkState:
			fStatusBar->Reset(B_TRANSLATE("Creating links: "), buffer.String());
			break;

		case kMoveState:
			fStatusBar->Reset(B_TRANSLATE("Moving: "), buffer.String());
			break;

		case kTrashState:
			fStatusBar->Reset(B_TRANSLATE("Emptying Trash" B_UTF8_ELLIPSIS " "),
				buffer.String());
			break;

		case kDeleteState:
			fStatusBar->Reset(B_TRANSLATE("Deleting: "), buffer.String());
			break;

		case kRestoreFromTrashState:
			fStatusBar->Reset(B_TRANSLATE("Restoring: "), buffer.String());
			break;

		default:
			break;
	}

	fStatusBar->SetMaxValue(1);
		// SetMaxValue has to be here because Reset changes it to 100
	Invalidate();
}


void
BStatusView::Draw(BRect updateRect)
{
	if (fBitmap) {
		BPoint location;
		location.x = (fStatusBar->Frame().left - fBitmap->Bounds().Width()) / 2;
		location.y = (Bounds().Height()- fBitmap->Bounds().Height()) / 2;
		DrawBitmap(fBitmap, location);
	}

	BRect bounds(Bounds());
	be_control_look->DrawRaisedBorder(this, bounds, updateRect, ViewColor());

	SetHighColor(0, 0, 0);

	BPoint tp = fStatusBar->Frame().LeftBottom();
	font_height fh;
	GetFontHeight(&fh);
	tp.y += ceilf(fh.leading) + ceilf(fh.ascent);
	if (IsPaused()) {
		DrawString(B_TRANSLATE("Paused: click to resume or stop"), tp);
		return;
	}

	BFont font;
	GetFont(&font);
	float normalFontSize = font.Size();
	float smallFontSize = max_c(normalFontSize * 0.8f, 8.0f);
	float availableSpace = fStatusBar->Frame().Width();
	availableSpace -= be_control_look->DefaultLabelSpacing();
		// subtract to provide some room between our two strings

	float destinationStringWidth = 0.f;
	BString destinationString(_DestinationString(&destinationStringWidth));
	availableSpace -= destinationStringWidth;

	float statusStringWidth = 0.f;	
	BString statusString(_StatusString(availableSpace, smallFontSize,
		&statusStringWidth));

	if (statusStringWidth > availableSpace) {
		TruncateString(&destinationString, B_TRUNCATE_MIDDLE,
			availableSpace + destinationStringWidth - statusStringWidth);
	}

	BPoint textPoint = fStatusBar->Frame().LeftBottom();
	textPoint.y += ceilf(fh.leading) + ceilf(fh.ascent);

	if (destinationStringWidth > 0) {
		DrawString(destinationString.String(), textPoint);
	}

	SetHighColor(tint_color(LowColor(), B_DARKEN_4_TINT));
	font.SetSize(smallFontSize);
	SetFont(&font, B_FONT_SIZE);

	textPoint.x = fStatusBar->Frame().right - statusStringWidth;
	DrawString(statusString.String(), textPoint);

	font.SetSize(normalFontSize);
	SetFont(&font, B_FONT_SIZE);
}


BString
BStatusView::_DestinationString(float* _width)
{
	if (fDestDir.Length() > 0) {
		BString buffer(B_TRANSLATE("To: %dir"));
		buffer.ReplaceFirst("%dir", fDestDir);

		*_width = ceilf(StringWidth(buffer.String()));
		return buffer;
	} else {
		*_width = 0;
		return BString();	
	}
}


BString
BStatusView::_StatusString(float availableSpace, float fontSize, float* _width)
{
	BFont font;
	GetFont(&font);
	float oldSize = font.Size();
	font.SetSize(fontSize);
	SetFont(&font, B_FONT_SIZE);

	BString status;
	if (sShowSpeed) {
		status = _SpeedStatusString(availableSpace, _width);
	} else
		status = _TimeStatusString(availableSpace, _width);

	font.SetSize(oldSize);
	SetFont(&font, B_FONT_SIZE);
	return status;
}


BString
BStatusView::_SpeedStatusString(float availableSpace, float* _width)
{
	BString string(_FullSpeedString());
	*_width = StringWidth(string.String());
	if (*_width > availableSpace) {
		string.SetTo(_ShortSpeedString());
		*_width = StringWidth(string.String());
	}
	*_width = ceilf(*_width);
	return string;
}


BString
BStatusView::_FullSpeedString()
{
	BString buffer;
	if (fBytesPerSecond != 0.0) {
		char sizeBuffer[128];
		buffer.SetTo(B_TRANSLATE(
			"(%SizeProcessed of %TotalSize, %BytesPerSecond/s)"));
		buffer.ReplaceFirst("%SizeProcessed",
			string_for_size((double)fSizeProcessed, sizeBuffer,
			sizeof(sizeBuffer)));
		buffer.ReplaceFirst("%TotalSize",
			string_for_size((double)fTotalSize, sizeBuffer,
			sizeof(sizeBuffer)));
		buffer.ReplaceFirst("%BytesPerSecond",
			string_for_size(fBytesPerSecond, sizeBuffer, sizeof(sizeBuffer)));
	}
	return buffer;
}


BString
BStatusView::_ShortSpeedString()
{
	BString buffer;
	if (fBytesPerSecond != 0.0) {
		char sizeBuffer[128];
		buffer << B_TRANSLATE("%BytesPerSecond/s");
		buffer.ReplaceFirst("%BytesPerSecond",
			string_for_size(fBytesPerSecond, sizeBuffer, sizeof(sizeBuffer)));
	}
	return buffer;
}


BString
BStatusView::_TimeStatusString(float availableSpace, float* _width)
{
	double totalBytesPerSecond = (double)(fSizeProcessed
			- fEstimatedFinishReferenceSize)
		* 1000000LL / (system_time() - fEstimatedFinishReferenceTime);
	double secondsRemaining = (fTotalSize - fSizeProcessed)
		/ totalBytesPerSecond;
	time_t now = (time_t)real_time_clock();
	time_t finishTime = (time_t)(now + secondsRemaining);

	char timeText[32];
	const BLocale* locale = BLocale::Default();
	if (finishTime - now > kSecondsPerDay) {
		locale->FormatDateTime(timeText, sizeof(timeText), finishTime,
			B_MEDIUM_DATE_FORMAT, B_MEDIUM_TIME_FORMAT);
	} else {
		locale->FormatTime(timeText, sizeof(timeText), finishTime,
			B_MEDIUM_TIME_FORMAT);
	}

	BString string(_FullTimeRemainingString(now, finishTime, timeText));
	*_width = StringWidth(string.String());
	if (*_width > availableSpace) {
		string.SetTo(_ShortTimeRemainingString(timeText));
		*_width = StringWidth(string.String());
	}

	return string;
}


BString
BStatusView::_ShortTimeRemainingString(const char* timeText)
{
	BString buffer;

	// complete string too wide, try with shorter version
	buffer.SetTo(B_TRANSLATE("(Finish: %time)"));
	buffer.ReplaceFirst("%time", timeText);

	return buffer;
}


BString
BStatusView::_FullTimeRemainingString(time_t now, time_t finishTime,
	const char* timeText)
{
	BDurationFormat formatter;
	BString buffer;
	BString finishStr;
	if (finishTime - now > 60 * 60) {
		buffer.SetTo(B_TRANSLATE("(Finish: %time - Over %finishtime left)"));
		formatter.Format(now * 1000000LL, finishTime * 1000000LL, &finishStr);
	} else {
		buffer.SetTo(B_TRANSLATE("(Finish: %time - %finishtime left)"));
		formatter.Format(now * 1000000LL, finishTime * 1000000LL, &finishStr);
	}

	buffer.ReplaceFirst("%time", timeText);
	buffer.ReplaceFirst("%finishtime", finishStr);

	return buffer;
}


void
BStatusView::AttachedToWindow()
{
	fPauseButton->SetTarget(this);
	fStopButton->SetTarget(this);
}


void
BStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kPauseButton:
			fIsPaused = !fIsPaused;
			fPauseButton->SetValue(fIsPaused ? B_CONTROL_ON : B_CONTROL_OFF);
			if (fBytesPerSecond != 0.0) {
				fBytesPerSecond = 0.0;
				for (size_t i = 0; i < kBytesPerSecondSlots; i++)
					fBytesPerSecondSlot[i] = 0.0;
				Invalidate();
			}
			if (!fIsPaused) {
				fEstimatedFinishReferenceTime = system_time();
				fEstimatedFinishReferenceSize = fSizeProcessed;

				// force window update
				Invalidate();

				// let 'er rip
				resume_thread(Thread());
			}
			break;

		case kStopButton:
			fWasCanceled = true;
			if (fIsPaused) {
				// resume so that the copy loop gets a chance to finish up
				fIsPaused = false;

				// force window update
				Invalidate();

				// let 'er rip
				resume_thread(Thread());
			}
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
BStatusView::UpdateStatus(const char *curItem, off_t itemSize, bool optional)
{
	if (!fShowCount) {
		fStatusBar->Update((float)fItemSize / fTotalSize);
		fItemSize = 0;
		return;
	}

	if (curItem != NULL)
		fCurItem++;

	fItemSize += itemSize;
	fSizeProcessed += itemSize;

	bigtime_t currentTime = system_time();
	if (!optional || ((currentTime - fLastUpdateTime) > kMaxUpdateInterval)) {
		if (curItem != NULL || fPendingStatusString[0]) {
			// forced update or past update time

			BString buffer;
			buffer <<  fCurItem << " ";

			// if we don't have curItem, take the one from the stash
			const char *statusItem = curItem != NULL
				? curItem : fPendingStatusString;

			fStatusBar->Update((float)fItemSize / fTotalSize, statusItem,
				buffer.String());

			// we already displayed this item, clear the stash
			fPendingStatusString[0] =  '\0';

			fLastUpdateTime = currentTime;
		} else {
			// don't have a file to show, just update the bar
			fStatusBar->Update((float)fItemSize / fTotalSize);
		}

		if (currentTime
				>= fLastSpeedReferenceTime + kSpeedReferenceInterval) {
			// update current speed every kSpeedReferenceInterval
			fCurrentBytesPerSecondSlot
				= (fCurrentBytesPerSecondSlot + 1) % kBytesPerSecondSlots;
			fBytesPerSecondSlot[fCurrentBytesPerSecondSlot]
				= (double)(fSizeProcessed - fLastSpeedReferenceSize)
					* 1000000LL / (currentTime - fLastSpeedReferenceTime);
			fLastSpeedReferenceSize = fSizeProcessed;
			fLastSpeedReferenceTime = currentTime;
			fBytesPerSecond = 0.0;
			size_t count = 0;
			for (size_t i = 0; i < kBytesPerSecondSlots; i++) {
				if (fBytesPerSecondSlot[i] != 0.0) {
					fBytesPerSecond += fBytesPerSecondSlot[i];
					count++;
				}
			}
			if (count > 0)
				fBytesPerSecond /= count;
			Invalidate();
		}

		fItemSize = 0;
	} else if (curItem != NULL) {
		// stash away the name of the item we are currently processing
		// so we can show it when the time comes
		strncpy(fPendingStatusString, curItem, 127);
		fPendingStatusString[127] = '0';
	}
}


void
BStatusView::SetWasCanceled()
{
	fWasCanceled = true;
}


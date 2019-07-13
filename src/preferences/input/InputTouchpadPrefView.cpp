/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "InputTouchpadPrefView.h"

#include <stdio.h>

#include <Alert.h>
#include <Box.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <File.h>
#include <FindDirectory.h>
#include <Input.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include <keyboard_mouse_driver.h>


const uint32 SCROLL_X_DRAG = 'sxdr';
const uint32 SCROLL_Y_DRAG = 'sydr';

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TouchpadPrefView"


TouchpadView::TouchpadView(BRect frame)
	:
	BView(frame, "TouchpadView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fXTracking = false;
	fYTracking = false;
	fOffScreenView  = NULL;
	fOffScreenBitmap = NULL;

	fPrefRect = frame;
	fPadRect = fPrefRect;
	fPadRect.InsetBy(10, 10);
	fXScrollRange = fPadRect.Width();
	fYScrollRange = fPadRect.Height();

}


TouchpadView::~TouchpadView()
{
	delete fOffScreenBitmap;
}


void
TouchpadView::Draw(BRect updateRect)
{
	DrawSliders();
}


void
TouchpadView::MouseDown(BPoint point)
{
	if (fXScrollDragZone.Contains(point)) {
		fXTracking = true;
		fOldXScrollRange = fXScrollRange;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}

	if (fYScrollDragZone.Contains(point)) {
		fYTracking = true;
		fOldYScrollRange = fYScrollRange;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}


void
TouchpadView::MouseUp(BPoint point)
{
	if (!fXTracking && !fYTracking)
		return;

	fXTracking = false;
	fYTracking = false;

	const float kSoftScrollLimit = 0.7;

	int32 result = 0;
	if (GetRightScrollRatio() > kSoftScrollLimit
		|| GetBottomScrollRatio() > kSoftScrollLimit) {
		BAlert* alert = new BAlert(B_TRANSLATE("Please confirm"),
			B_TRANSLATE("The new scroll area is very large and may impede "
				"normal mouse operation. Do you really want to change it?"),
			B_TRANSLATE("OK"), B_TRANSLATE("Cancel"),
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(1, B_ESCAPE);
		result = alert->Go();
	}

	if (result == 0) {
		BMessage msg(SCROLL_AREA_CHANGED);
		Invoke(&msg);
	} else {
		if (GetRightScrollRatio() > kSoftScrollLimit)
			fXScrollRange = fOldXScrollRange;
		if (GetBottomScrollRatio() > kSoftScrollLimit)
			fYScrollRange = fOldYScrollRange;
		DrawSliders();
	}
}


void
TouchpadView::AttachedToWindow()
{
	if (!fOffScreenView)
		fOffScreenView = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);

	if (!fOffScreenBitmap) {
		fOffScreenBitmap = new BBitmap(Bounds(), B_CMAP8, true, false);

		if (fOffScreenBitmap && fOffScreenView)
			fOffScreenBitmap->AddChild(fOffScreenView);
	}
}


void
TouchpadView::SetValues(float rightRange, float bottomRange)
{
	fXScrollRange = fPadRect.Width() * (1 - rightRange);
	fYScrollRange = fPadRect.Height() * (1 - bottomRange);
	Invalidate();
}


void
TouchpadView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = fPrefRect.Width();
	if (height != NULL)
		*height = fPrefRect.Height();
}


void
TouchpadView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (fXTracking) {
		if (point.x > fPadRect.right)
			fXScrollRange = fPadRect.Width();
		else if (point.x < fPadRect.left)
			fXScrollRange = 0;
		else
			fXScrollRange = point.x - fPadRect.left;

		DrawSliders();
	}

	if (fYTracking) {
		if (point.y > fPadRect.bottom)
			fYScrollRange = fPadRect.Height();
		else if (point.y < fPadRect.top)
			fYScrollRange = 0;
		else
			fYScrollRange = point.y - fPadRect.top;

		DrawSliders();
	}
}



void
TouchpadView::DrawSliders()
{
	BView* view = fOffScreenView != NULL ? fOffScreenView : this;

	if (!LockLooper())
		return;

	if (fOffScreenBitmap->Lock()) {
		view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->FillRect(Bounds());
		view->SetHighColor(100, 100, 100);
		view->FillRoundRect(fPadRect, 4, 4);

		int32 dragSize = 3; // half drag size

		// scroll areas
		view->SetHighColor(145, 100, 100);
		BRect rightRect(fPadRect.left + fXScrollRange,  fPadRect.top,
			fPadRect.right, fPadRect.bottom);
		view->FillRoundRect(rightRect, 4, 4);

		BRect bottomRect(fPadRect.left, fPadRect.top + fYScrollRange,
			fPadRect.right, fPadRect.bottom);
		view->FillRoundRect(bottomRect, 4, 4);

		// Stroke Rect
		view->SetHighColor(100, 100, 100);
		view->SetPenSize(2);
		view->StrokeRoundRect(fPadRect, 4, 4);

		// x scroll range line
		view->SetHighColor(200, 0, 0);
		view->StrokeLine(BPoint(fPadRect.left + fXScrollRange, fPadRect.top),
			BPoint(fPadRect.left + fXScrollRange, fPadRect.bottom));

		fXScrollDragZone = BRect(fPadRect.left + fXScrollRange - dragSize,
			fPadRect.top - dragSize, fPadRect.left + fXScrollRange + dragSize,
			fPadRect.bottom + dragSize);
		BRect xscrollDragZone1 = BRect(fPadRect.left + fXScrollRange - dragSize,
			fPadRect.top - dragSize, fPadRect.left + fXScrollRange + dragSize,
			fPadRect.top + dragSize);
		view->FillRect(xscrollDragZone1);
		BRect xscrollDragZone2 = BRect(fPadRect.left + fXScrollRange - dragSize,
			fPadRect.bottom - dragSize,
			fPadRect.left + fXScrollRange + dragSize,
			fPadRect.bottom + dragSize);
		view->FillRect(xscrollDragZone2);

		// y scroll range line
		view->StrokeLine(BPoint(fPadRect.left, fPadRect.top + fYScrollRange),
			BPoint(fPadRect.right, fPadRect.top  + fYScrollRange));

		fYScrollDragZone = BRect(fPadRect.left - dragSize,
			fPadRect.top + fYScrollRange - dragSize,
			fPadRect.right  + dragSize,
			fPadRect.top + fYScrollRange + dragSize);
		BRect yscrollDragZone1 = BRect(fPadRect.left - dragSize,
			fPadRect.top + fYScrollRange - dragSize, fPadRect.left  + dragSize,
			fPadRect.top + fYScrollRange + dragSize);
		view->FillRect(yscrollDragZone1);
		BRect yscrollDragZone2 = BRect(fPadRect.right - dragSize,
			fPadRect.top + fYScrollRange - dragSize, fPadRect.right  + dragSize,
			fPadRect.top + fYScrollRange + dragSize);
		view->FillRect(yscrollDragZone2);

		view->Sync();
		fOffScreenBitmap->Unlock();
		DrawBitmap(fOffScreenBitmap, B_ORIGIN);
	}

	UnlockLooper();
}


//	#pragma mark - TouchpadPrefView


TouchpadPrefView::TouchpadPrefView(BInputDevice* dev)
	:
	BGroupView(),
	fTouchpadPref(dev)
{
	SetupView();
	// set view values
	SetValues(&fTouchpadPref.Settings());
}


TouchpadPrefView::~TouchpadPrefView()
{
}


void
TouchpadPrefView::MessageReceived(BMessage* message)
{
	touchpad_settings& settings = fTouchpadPref.Settings();

	switch (message->what) {
		case SCROLL_AREA_CHANGED:
			settings.scroll_rightrange = fTouchpadView->GetRightScrollRatio();
			settings.scroll_bottomrange = fTouchpadView->GetBottomScrollRatio();
			fTouchpadPref.UpdateSettings();
			break;

		case SCROLL_CONTROL_CHANGED:
			settings.scroll_twofinger = fTwoFingerBox->Value() == B_CONTROL_ON;
			settings.scroll_twofinger_horizontal
				= fTwoFingerHorizontalBox->Value() == B_CONTROL_ON;
			settings.scroll_acceleration = fScrollAccelSlider->Value();
			settings.scroll_xstepsize = (20 - fScrollStepXSlider->Value()) * 3;
			settings.scroll_ystepsize = (20 - fScrollStepYSlider->Value()) * 3;
			fTwoFingerHorizontalBox->SetEnabled(settings.scroll_twofinger);
			fTouchpadPref.UpdateSettings();
			break;

		case TAP_CONTROL_CHANGED:
			settings.tapgesture_sensibility = fTapSlider->Value();
			fTouchpadPref.UpdateSettings();
			break;

		case PADBLOCK_TIME_CHANGED:
			settings.padblocker_threshold = fPadBlockerSlider->Value();
			// The maximum value means "disabled", but in the settings file that
			// must be stored as 0
			if (settings.padblocker_threshold == 1000)
				settings.padblocker_threshold = 0;
			fRevertButton->SetEnabled(true);
			fTouchpadPref.UpdateSettings();
			break;

		case DEFAULT_SETTINGS:
			fTouchpadPref.Defaults();
			fRevertButton->SetEnabled(true);
			fTouchpadPref.UpdateSettings();
			SetValues(&settings);
			break;

		case REVERT_SETTINGS:
			fTouchpadPref.Revert();
			fTouchpadPref.UpdateSettings();
			fRevertButton->SetEnabled(false);
			SetValues(&settings);
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
TouchpadPrefView::AttachedToWindow()
{
	fTouchpadView->SetTarget(this);
	fTwoFingerBox->SetTarget(this);
	fTwoFingerHorizontalBox->SetTarget(this);
	fScrollStepXSlider->SetTarget(this);
	fScrollStepYSlider->SetTarget(this);
	fScrollAccelSlider->SetTarget(this);
	fPadBlockerSlider->SetTarget(this);
	fTapSlider->SetTarget(this);
	fDefaultButton->SetTarget(this);
	fRevertButton->SetTarget(this);
	BSize size = PreferredSize();
	Window()->ResizeTo(size.width, size.height);

	BPoint position = fTouchpadPref.WindowPosition();
	// center window on screen if it had a bad position
	if (position.x < 0 && position.y < 0)
		Window()->CenterOnScreen();
	else
		Window()->MoveTo(position);
}


void
TouchpadPrefView::DetachedFromWindow()
{
	fTouchpadPref.SetWindowPosition(Window()->Frame().LeftTop());
}


void
TouchpadPrefView::SetupView()
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	BBox* scrollBox = new BBox("Touchpad");
	scrollBox->SetLabel(B_TRANSLATE("Scrolling"));


	fTouchpadView = new TouchpadView(BRect(0, 0, 130, 120));
	fTouchpadView->SetExplicitMaxSize(BSize(130, 120));

	// Create the "Mouse Speed" slider...
	fScrollAccelSlider = new BSlider("scroll_accel",
		B_TRANSLATE("Acceleration"),
		new BMessage(SCROLL_CONTROL_CHANGED), 0, 20, B_HORIZONTAL);
	fScrollAccelSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fScrollAccelSlider->SetHashMarkCount(7);
	fScrollAccelSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Fast"));
	fScrollAccelSlider->SetExplicitMinSize(BSize(150, B_SIZE_UNSET));

	fScrollStepXSlider = new BSlider("scroll_stepX",
		B_TRANSLATE("Horizontal"),
		new BMessage(SCROLL_CONTROL_CHANGED),
		0, 20, B_HORIZONTAL);
	fScrollStepXSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fScrollStepXSlider->SetHashMarkCount(7);
	fScrollStepXSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Fast"));

	fScrollStepYSlider = new BSlider("scroll_stepY",
		B_TRANSLATE("Vertical"),
		new BMessage(SCROLL_CONTROL_CHANGED), 0, 20, B_HORIZONTAL);
	fScrollStepYSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fScrollStepYSlider->SetHashMarkCount(7);
	fScrollStepYSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Fast"));

	fPadBlockerSlider = new BSlider("padblocker",
		B_TRANSLATE("Keyboard Lock Delay"),
		new BMessage(PADBLOCK_TIME_CHANGED), 5, 1000, B_HORIZONTAL);
	fPadBlockerSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPadBlockerSlider->SetHashMarkCount(10);
	fPadBlockerSlider->SetLimitLabels(B_TRANSLATE("Quick"),
		B_TRANSLATE("Never"));

	fTwoFingerBox = new BCheckBox(B_TRANSLATE("Two finger scrolling"),
		new BMessage(SCROLL_CONTROL_CHANGED));
	fTwoFingerHorizontalBox = new BCheckBox(
		B_TRANSLATE("Horizontal scrolling"),
		new BMessage(SCROLL_CONTROL_CHANGED));

	float spacing = be_control_look->DefaultItemSpacing();

	BView* scrollPrefLeftLayout
		= BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.Add(fTouchpadView)
		.AddStrut(spacing)
		.Add(fTwoFingerBox)
		.AddGroup(B_HORIZONTAL, 0)
			.AddStrut(spacing * 2)
			.Add(fTwoFingerHorizontalBox)
			.End()
		.AddGlue()
		.View();

	BGroupView* scrollPrefRightLayout = new BGroupView(B_VERTICAL);
	scrollPrefRightLayout->AddChild(fScrollAccelSlider);
	scrollPrefRightLayout->AddChild(fScrollStepXSlider);
	scrollPrefRightLayout->AddChild(fScrollStepYSlider);

	BGroupLayout* scrollPrefLayout = new BGroupLayout(B_HORIZONTAL);
	scrollPrefLayout->SetSpacing(spacing);
	scrollPrefLayout->SetInsets(spacing,
		scrollBox->TopBorderOffset() * 2 + spacing, spacing, spacing);
	scrollBox->SetLayout(scrollPrefLayout);

	scrollPrefLayout->AddView(scrollPrefLeftLayout);
	scrollPrefLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(spacing
		* 1.5));
	scrollPrefLayout->AddView(scrollPrefRightLayout);

	fTapSlider = new BSlider("tap_sens", B_TRANSLATE("Tapping sensitivity"),
		new BMessage(TAP_CONTROL_CHANGED), 0, spacing * 2, B_HORIZONTAL);
	fTapSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fTapSlider->SetHashMarkCount(7);
	fTapSlider->SetLimitLabels(B_TRANSLATE("Off"), B_TRANSLATE("High"));

	fDefaultButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(DEFAULT_SETTINGS));

	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(REVERT_SETTINGS));
	fRevertButton->SetEnabled(false);


	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(scrollBox)
		.Add(fTapSlider)
		.Add(fPadBlockerSlider)
		.Add(new BSeparatorView(B_HORIZONTAL))
			.AddGroup(B_HORIZONTAL)
			.Add(fDefaultButton)
			.Add(fRevertButton)
			.AddGlue()
		.End()
	.End();
}


void
TouchpadPrefView::SetValues(touchpad_settings* settings)
{
	fTouchpadView->SetValues(settings->scroll_rightrange,
		settings->scroll_bottomrange);
	fTwoFingerBox->SetValue(settings->scroll_twofinger
		? B_CONTROL_ON : B_CONTROL_OFF);
	fTwoFingerHorizontalBox->SetValue(settings->scroll_twofinger_horizontal
		? B_CONTROL_ON : B_CONTROL_OFF);
	fTwoFingerHorizontalBox->SetEnabled(settings->scroll_twofinger);
	fScrollStepXSlider->SetValue(20 - settings->scroll_xstepsize / 2);
	fScrollStepYSlider->SetValue(20 - settings->scroll_ystepsize / 2);
	fScrollAccelSlider->SetValue(settings->scroll_acceleration);
	fTapSlider->SetValue(settings->tapgesture_sensibility);
	fPadBlockerSlider->SetValue(settings->padblocker_threshold);
}

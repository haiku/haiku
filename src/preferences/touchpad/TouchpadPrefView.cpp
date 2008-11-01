#include "TouchpadPrefView.h"
#include "kb_mouse_driver.h"

#include <CheckBox.h>
#include <Alert.h>
#include <Box.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Input.h>
#include <SpaceLayoutItem.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Screen.h>
#include <Window.h>

#include <stdio.h>


const uint32 SCROLL_X_DRAG = 'sxdr';
const uint32 SCROLL_Y_DRAG = 'sydr';


TouchpadView::TouchpadView(BRect frame)
	:BView(frame, "TouchpadView", B_FOLLOW_NONE, B_WILL_DRAW)
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
	if(fOffScreenBitmap){
		delete fOffScreenBitmap;
	}
}


void
TouchpadView::Draw(BRect updateRect)
{
	DrawSliders();

}


void
TouchpadView::MouseDown(BPoint point)
{
	if(fXScrollDragZone.Contains(point))
	{
		fXTracking = true;
		fOldXScrollRange = fXScrollRange;
		SetMouseEventMask( B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS );
	}

	if(fYScrollDragZone.Contains(point))
	{
		fYTracking = true;
		fOldYScrollRange = fYScrollRange;
		SetMouseEventMask( B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS );
	}
}


void
TouchpadView::MouseUp(BPoint point)
{
	if(fXTracking || fYTracking){
		fXTracking = false;
		fYTracking = false;
		int32 result = 0;
		if(GetRightScrollRatio() > 0.7 || GetBottomScrollRatio() > 0.7){
			BAlert *alert = new BAlert("ReallyChangeScrollArea",
				"The new scroll area is very small. Do you really want to change the scroll area?", "Ok", "Cancel",
				NULL,
				B_WIDTH_AS_USUAL,
				B_WARNING_ALERT);
			result = alert->Go();
		}
		if(result == 0){
			BMessage msg(SCROLL_AREA_CHANGED);
			Invoke(&msg);
		}
		else{
			fXScrollRange = fOldXScrollRange;
			fYScrollRange = fOldYScrollRange;
			DrawSliders();
		}
	}
}


void
TouchpadView::AttachedToWindow()
{
	if (!fOffScreenView) {
		fOffScreenView = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
	}
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
TouchpadView::GetPreferredSize(float *width, float *height)
{
	*width = fPrefRect.Width();
	*height =  fPrefRect.Height();
}


void
TouchpadView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if(fXTracking){
		if(point.x > fPadRect.right){
			fXScrollRange = fPadRect.Width();
		}
		else if(point.x < fPadRect.left){
			fXScrollRange = 0;
		}
		else{
			fXScrollRange = point.x - fPadRect.left;
		}
		DrawSliders();
	}

	if(fYTracking){
		if(point.y > fPadRect.bottom){
			fYScrollRange = fPadRect.Height();
		}
		else if(point.y < fPadRect.top){
			fYScrollRange = 0;
		}
		else{
			fYScrollRange = point.y - fPadRect.top;
		}
		DrawSliders();
	}
}



void
TouchpadView::DrawSliders()
{
	BView *view = NULL;
	if(fOffScreenView){
		view = fOffScreenView;
	}
	else{
		view = this;
	}

	if (LockLooper()) {
	if (fOffScreenBitmap->Lock()) {
	view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->FillRect(Bounds());
	view->SetHighColor(100, 100, 100);
	view->FillRoundRect(fPadRect, 4, 4);

	int32 dragSize = 3; //half drag size

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
					fPadRect.top - dragSize,
					fPadRect.left + fXScrollRange + dragSize,
					fPadRect.bottom + dragSize);
	fXScrollDragZone1 = BRect(fPadRect.left + fXScrollRange - dragSize,
					fPadRect.top - dragSize,
					fPadRect.left + fXScrollRange + dragSize,
					fPadRect.top + dragSize);
	view->FillRect(fXScrollDragZone1);
	fXScrollDragZone2 = BRect(fPadRect.left + fXScrollRange - dragSize,
					fPadRect.bottom - dragSize,
					fPadRect.left + fXScrollRange + dragSize,
					fPadRect.bottom + dragSize);
	view->FillRect(fXScrollDragZone2);

	// y scroll range line
	view->StrokeLine(BPoint(fPadRect.left, fPadRect.top + fYScrollRange),
				BPoint(fPadRect.right, fPadRect.top  + fYScrollRange));

	fYScrollDragZone = BRect(fPadRect.left - dragSize,
					fPadRect.top + fYScrollRange - dragSize,
					fPadRect.right  + dragSize,
					fPadRect.top + fYScrollRange + dragSize);
	fYScrollDragZone1 = BRect(fPadRect.left - dragSize,
					fPadRect.top + fYScrollRange - dragSize,
					fPadRect.left  + dragSize,
					fPadRect.top + fYScrollRange + dragSize);
	view->FillRect(fYScrollDragZone1);
	fYScrollDragZone2 = BRect(fPadRect.right - dragSize,
					fPadRect.top + fYScrollRange - dragSize,
					fPadRect.right  + dragSize,
					fPadRect.top + fYScrollRange + dragSize);
	view->FillRect(fYScrollDragZone2);

		fOffScreenView->Sync();
		fOffScreenBitmap->Unlock();
		DrawBitmap(fOffScreenBitmap, B_ORIGIN);


	}
		UnlockLooper();
	}
}

// -------TouchpadPrefView----------
TouchpadPrefView::TouchpadPrefView(BRect frame, const char *name)
							:BView(frame, name, B_FOLLOW_ALL_SIDES, 0)
{
	SetupView();
	// set view values
	SetValues(fTouchpadPref.GetSettings());
}



TouchpadPrefView::~TouchpadPrefView()
{
	delete fTouchpadView;
}


void
TouchpadPrefView::MessageReceived(BMessage *msg)
{
	touchpad_settings *settings = fTouchpadPref.GetSettings();
	switch(msg->what) {
		case SCROLL_AREA_CHANGED:
			settings->scroll_rightrange = fTouchpadView->GetRightScrollRatio();
			settings->scroll_bottomrange = fTouchpadView->GetBottomScrollRatio();
			fRevertButton->SetEnabled(true);
			fTouchpadPref.UpdateSettings();
			break;
		case SCROLL_CONTROL_CHANGED:
			settings->scroll_twofinger = (fTwoFingerBox->Value() == B_CONTROL_ON) ? true : false;
			settings->scroll_multifinger = (fMultiFingerBox->Value() == B_CONTROL_ON) ? true : false;
			settings->scroll_acceleration = fScrollAccelSlider->Value();
			settings->scroll_xstepsize = (20 - fScrollStepXSlider->Value()) * 3;
			settings->scroll_ystepsize = (20 - fScrollStepYSlider->Value()) * 3;
			fRevertButton->SetEnabled(true);
			fTouchpadPref.UpdateSettings();
			break;
		case TAP_CONTROL_CHANGED:
			settings->tapgesture_sensibility = fTapSlider->Value();
			fRevertButton->SetEnabled(true);
			fTouchpadPref.UpdateSettings();
			break;
		case DEFAULT_SETTINGS:
			fTouchpadPref.Defaults();
			fRevertButton->SetEnabled(true);
			fTouchpadPref.UpdateSettings();
			SetValues(settings);
			break;
		case REVERT_SETTINGS:
			fTouchpadPref.Revert();
			fTouchpadPref.UpdateSettings();
			fRevertButton->SetEnabled(false);
			SetValues(settings);
			break;
		default:
			BView::MessageReceived(msg);
	}

}


void
TouchpadPrefView::AttachedToWindow()
{
	fTouchpadView->SetTarget(this);
	fTwoFingerBox->SetTarget(this);
	fMultiFingerBox->SetTarget(this);
	fScrollStepXSlider->SetTarget(this);
	fScrollStepYSlider->SetTarget(this);
	fScrollAccelSlider->SetTarget(this);
	fTapSlider->SetTarget(this);
	fDefaultButton->SetTarget(this);
	fRevertButton->SetTarget(this);
	BSize size = PreferredSize();
	Window()->ResizeTo(size.width, size.height);

	BRect rect = BScreen().Frame();
	BRect windowFrame = Window()->Frame();
	BPoint position = fTouchpadPref.WindowPosition();
	// center window on screen if it had a bad position
	if(position.x < 0 && position.y < 0){
		position.x = (rect.Width() - windowFrame.Width()) / 2;
		position.y = (rect.Height() - windowFrame.Height()) / 2;
	}
	Window()->MoveTo(position);
}


void
TouchpadPrefView::DetachedFromWindow(void)
{
	fTouchpadPref.SetWindowPosition(Window()->Frame().LeftTop());
}


void
TouchpadPrefView::SetupView()
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	BBox *scrollBox = new BBox(rect, "Touchpad");
	scrollBox->SetLabel("Scrolling");
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTouchpadView = new TouchpadView(BRect(0,0,130,120));
	fTouchpadView->SetExplicitMaxSize(BSize(130, 120));

	// Create the "Mouse Speed" slider...
	fScrollAccelSlider = new BSlider(rect, "scroll_accel", "Scroll Acceleration",
		new BMessage(SCROLL_CONTROL_CHANGED), 0, 20);
	fScrollAccelSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fScrollAccelSlider->SetHashMarkCount(7);
	fScrollAccelSlider->SetLimitLabels("Slow", "Fast");

	fScrollStepXSlider = new BSlider(rect, "scroll_stepX", "Horizontal Scroll Stepsize",
		new BMessage(SCROLL_CONTROL_CHANGED), 0, 20);
	fScrollStepXSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fScrollStepXSlider->SetHashMarkCount(7);
	fScrollStepXSlider->SetLimitLabels("Wide", "Small");

	fScrollStepYSlider = new BSlider(rect, "scroll_stepY", "Vertical Scroll Stepsize",
		new BMessage(SCROLL_CONTROL_CHANGED), 0, 20);
	fScrollStepYSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fScrollStepYSlider->SetHashMarkCount(7);
	fScrollStepYSlider->SetLimitLabels("Wide", "Small");

	fTwoFingerBox = new BCheckBox("Two Finger Scrolling", new BMessage(SCROLL_CONTROL_CHANGED));
	fMultiFingerBox = new BCheckBox("Multi Finger Scrolling", new BMessage(SCROLL_CONTROL_CHANGED));

	BGroupView* scrollPrefLeftLayout = new BGroupView(B_VERTICAL);
	scrollPrefLeftLayout->AddChild(fTouchpadView);
	scrollPrefLeftLayout->AddChild(fTwoFingerBox);
	scrollPrefLeftLayout->AddChild(fMultiFingerBox);

	BGroupView* scrollPrefRightLayout = new BGroupView(B_VERTICAL);
	scrollPrefRightLayout->AddChild(fScrollAccelSlider);
	scrollPrefRightLayout->AddChild(fScrollStepXSlider);
	scrollPrefRightLayout->AddChild(fScrollStepYSlider);

	BGroupLayout* scrollPrefLayout = new BGroupLayout(B_HORIZONTAL);
	scrollPrefLayout->SetSpacing(10);
	scrollPrefLayout->SetInsets(10, scrollBox->TopBorderOffset() * 2 + 10, 10, 10);
	scrollBox->SetLayout(scrollPrefLayout);

	scrollPrefLayout->AddView(scrollPrefLeftLayout);
	scrollPrefLayout->AddItem(BSpaceLayoutItem::CreateVerticalStrut(15));
	scrollPrefLayout->AddView(scrollPrefRightLayout);

	BBox *tapBox = new BBox(rect, "tapbox");
	tapBox->SetLabel("Tap Gesture");

	BGroupLayout* tapPrefLayout = new BGroupLayout(B_HORIZONTAL);
	tapPrefLayout->SetInsets(10, tapBox->TopBorderOffset() * 2 + 10, 10, 10);
	tapBox->SetLayout(tapPrefLayout);

	fTapSlider = new BSlider(rect, "tap_sens", "Tap Click Sensitivity",
		new BMessage(TAP_CONTROL_CHANGED), 0, 20);
	fTapSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fTapSlider->SetHashMarkCount(7);
	fTapSlider->SetLimitLabels("Off", "High");

	tapPrefLayout->AddView(fTapSlider);

	BGroupView* buttonView = new BGroupView(B_HORIZONTAL);
	fDefaultButton = new BButton("Defaults",
										new BMessage(DEFAULT_SETTINGS));

	buttonView->AddChild(fDefaultButton);
	buttonView->GetLayout()->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(7));
	fRevertButton = new BButton("Revert",
										new BMessage(REVERT_SETTINGS));
	fRevertButton->SetEnabled(false);
	buttonView->AddChild(fRevertButton);
	buttonView->GetLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	layout->SetInsets(10, 10, 10, 10);
	layout->SetSpacing(10);
	BView* rootView = new BView("root view", 0, layout);
	AddChild(rootView);
	rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	layout->AddView(scrollBox);
	layout->AddView(tapBox);
	layout->AddView(buttonView);

}


void
TouchpadPrefView::SetValues(touchpad_settings *settings)
{
	fTouchpadView->SetValues(settings->scroll_rightrange, settings->scroll_bottomrange);
	fTwoFingerBox->SetValue(settings->scroll_twofinger ? B_CONTROL_ON : B_CONTROL_OFF);
	fMultiFingerBox->SetValue(settings->scroll_multifinger ? B_CONTROL_ON : B_CONTROL_OFF);
	fScrollStepXSlider->SetValue(20 - settings->scroll_xstepsize / 2);
	fScrollStepYSlider->SetValue(20 - settings->scroll_ystepsize / 2);
	fScrollAccelSlider->SetValue(settings->scroll_acceleration);
	fTapSlider->SetValue(settings->tapgesture_sensibility);
}


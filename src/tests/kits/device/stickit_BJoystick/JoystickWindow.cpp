//
// StickIt
// File: JoystickWindow.cpp
// Joystick window.
// Sampel code used in "Getting a Grip on BJoystick" by Eric Shepherd
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>

#include <Application.h>
#include <Joystick.h>
#include <String.h>
#include <StringView.h>

#include "JoystickWindow.h"

rgb_color rgb_black = {0, 0, 0, 255};
rgb_color rgb_red = {255, 0, 0, 255};
rgb_color rgb_grey = {216, 216, 216};

int32 hatX[9] = {10, 10, 20, 20, 20, 10, 0, 0, 0};
int32 hatY[9] = {10, 0, 0, 10, 20, 20, 20, 10, 0};

JoystickWindow::JoystickWindow(BJoystick *stick, BRect rect)
	: BWindow(rect, "StickIt", B_TITLED_WINDOW,
	B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	fView = new JoystickView(Bounds(), stick);
	fView->SetViewColor(rgb_grey);
	fView->SetLowColor(rgb_grey);
	ResizeTo(fView->Bounds().Width(), fView->Bounds().Height());
	AddChild(fView);
	SetPulseRate(100000.0);
}


bool
JoystickWindow::QuitRequested(void) {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


JoystickView::JoystickView(BRect frame, BJoystick *stick)
	:BView(frame, "jview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_PULSE_NEEDED),
	fStick(stick)
{
	// Set up the controls
	BString name;
	fStick->GetControllerName(&name);
	BRect rect(5, 5, 350, 25);
	BStringView *stickName = new BStringView(rect, "stickname", name.String());
	stickName->SetFontSize(18);
	AddChild(stickName);

	rect = _BuildButtons(stick);
	rect.top += 15;
	rect.bottom += 15;
	rect = _BuildHats(stick, rect);
	_BuildAxes(stick, rect);
}


void
JoystickView::Pulse(void) {
	Window()->Lock();
	Draw(Bounds());
	Window()->Unlock();
}


void
JoystickView::Draw(BRect updateRect)
{
	int32 numButtons = fStick->CountButtons();
	int32 numHats = fStick->CountHats();
	int32 numAxes = fStick->CountAxes();
	bool *buttons = (bool *) malloc(sizeof(bool) * numButtons);
	int16 *axes = (int16 *) malloc(sizeof(int16) * numAxes);
	uint8 *hats = (uint8 *) malloc(numHats);
	fStick->Update();

	// Buttons first
	BRect r(105, 50, 115, 60);
	fStick->GetButtonValues(buttons);
	for (int32 i = 0; i < numButtons; i++) {
		if (buttons[i]) {
			FillRect(r, B_SOLID_HIGH);
		} else {
			r.InsetBy(1, 1);
			FillRect(r, B_SOLID_LOW);
			r.InsetBy(-1, -1);
			StrokeRect(r, B_SOLID_HIGH);
		}
		r.top += 18;
		r.bottom += 18;
	}

	// Now hats
	fStick->GetHatValues(hats);
	r.top += 15;
	r.bottom = r.top + 30;
	r.left = 105;
	r.right = r.left + 30;
	BRect curHatRect;
	for (int32 i = 0; i < numHats; i++) {
		BeginLineArray(8);
		AddLine(r.LeftTop(), r.RightTop(), rgb_black);
		AddLine(r.LeftTop(), r.LeftBottom(), rgb_black);
		AddLine(r.RightTop(), r.RightBottom(), rgb_black);
		AddLine(r.LeftBottom(), r.RightBottom(), rgb_black);
		AddLine(BPoint(r.left+10, r.top), BPoint(r.left+10, r.bottom),
			rgb_black);
		AddLine(BPoint(r.left+20, r.top), BPoint(r.left+20, r.bottom),
			rgb_black);
		AddLine(BPoint(r.left, r.top+10), BPoint(r.right, r.top+10),
			rgb_black);
		AddLine(BPoint(r.left, r.top+20), BPoint(r.right, r.top+20),
			rgb_black);
		EndLineArray();
		curHatRect.Set(r.left, r.top, r.left+10, r.top+10);
		curHatRect.OffsetBy(hatX[hats[i]], hatY[hats[i]]);
		fLastHatRect.InsetBy(1, 1);
		FillRect(fLastHatRect, B_SOLID_LOW);
		fLastHatRect = curHatRect;
		fLastHatRect.InsetBy(1, 1);
		FillRect(fLastHatRect, B_SOLID_HIGH);
		fLastHatRect.InsetBy(-1, -1);
		r.top += 20;
		r.bottom += 20;
	}

	// Now the joystick
	r.Set(200, 50, 350, 200);
	FillRect(r, B_SOLID_HIGH);
	fStick->GetAxisValues(axes);
	float x = axes[0];
	float y = axes[1];
	SetHighColor(rgb_red);
	x += 32768;
	y -= 32768;
	x *= 0.0021362304;
	y *= 0.0021362304;
	x += 205;
	y += 195;
	FillEllipse(BPoint(x,y), 5, 5);
	SetHighColor(rgb_black);

	// Finally, other axes
	r.Set(200, 220, 350, 234);
	for (int32 i = 2; i < numAxes; i++) {
		FillRect(r, B_SOLID_HIGH);
		x = axes[i];
		x += 32768;
		x *= 0.0021362304;
		x += 205;
		BRect thumbRect(x - 3, r.top, x + 3, r.bottom);
		SetHighColor(rgb_red);
		FillRoundRect(thumbRect, 3, 3);
		SetHighColor(rgb_black);
		r.top += 20;
		r.bottom += 20;
	}
	free(buttons);
	free(axes);
	free(hats);
}

//----------------------- Pragma mark ------------------------------------//

BRect
JoystickView::_BuildButtons(BJoystick *stick)
{
	BString name;
	int32 numButtons = stick->CountButtons();
	BRect rect(5, 50, 100, 64);
	for (int32 i = 0; i < numButtons; i++) {
		stick->GetButtonNameAt(i, &name);
		rect = _BuildString(name, "buttonlabel", 18, rect);
	}
	return rect;
}


BRect
JoystickView::_BuildHats(BJoystick *stick, BRect rect)
{
	BString name;
	int32 numHats = stick->CountHats();
	for (int32 i = 0; i < numHats; i++) {
		stick->GetHatNameAt(i, &name);
		rect = _BuildString(name, "hatlabel", 40, rect);
	}
	return rect;
}


BRect
JoystickView::_BuildString(BString name, const char* strName, int number,
	BRect rect)
{
	name.Append(":");
	BStringView *sview = new BStringView(rect, strName, name.String());
	sview->SetAlignment(B_ALIGN_RIGHT);
	sview->SetFont(be_bold_font);
	AddChild(sview);
	rect.top += number;
	rect.bottom += number;
	return rect;
}


void
JoystickView::_BuildAxes(BJoystick *stick, BRect rect)
{
	float col1bot = rect.bottom - 30;
	int32 numAxes = stick->CountAxes();
	// We assume that the first two axes are the x and y axes.
	rect.Set(130, 50, 195, 64);
	BStringView *sview = new BStringView(rect, "sticklabel", "Stick:");
	sview->SetFont(be_bold_font);
	sview->SetAlignment(B_ALIGN_RIGHT);
	AddChild(sview);

	BString name;
	int32 i;
	// Now make labels for all the solitary axes
	rect.Set(130, 200, 195, 234);
	for (i = 2; i < numAxes; i++) {
		stick->GetAxisNameAt(i, &name);
		rect = _BuildString(name, "hatlabel", 20, rect);
	}

	fLastHatRect.Set(0, 0, 0, 0);
	if (rect.bottom-10 > col1bot) {
		if (i == 2) {
			col1bot = 205;
		} else {
			col1bot = rect.bottom - 10;
		}
	}
	ResizeTo(355, col1bot);
}

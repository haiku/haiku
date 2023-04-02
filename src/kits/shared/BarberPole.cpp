/*
 * Copyright 2017 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "BarberPole.h"

#include <AutoLocker.h>
#include <ControlLook.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Messenger.h>

#include <pthread.h>
#include <stdio.h>


// #pragma mark - MachineRoom


/*! The machine room spins all the barber poles.
    Keeps a list of all barber poles of this team and runs its own
    thread to invalidate them in regular intervals.
*/
class MachineRoom
{
private:
	enum {
		kSpinInterval = 20000 // us
	};

private:
	MachineRoom()
		:
		fMessengers(20, true)
	{
		fSpinLoopLock = create_sem(0, "BarberPole lock");
		fSpinLoopThread = spawn_thread(&MachineRoom::_StartSpinLoop,
			"The Barber Machine", B_DISPLAY_PRIORITY, this);
		resume_thread(fSpinLoopThread);
	}

public:
	static void AttachBarberPole(BarberPole* pole)
	{
		_InitializeIfNeeded();
		sInstance->_Attach(pole);
	}

	static void DetachBarberPole(BarberPole* pole)
	{
		sInstance->_Detach(pole);
	}

private:
	static void _Initialize()
	{
		sInstance = new MachineRoom();
	}

	static status_t _StartSpinLoop(void* instance)
	{
		static_cast<MachineRoom*>(instance)->_SpinLoop();
		return B_OK;
	}

	static void _InitializeIfNeeded()
	{
		pthread_once(&sOnceControl, &MachineRoom::_Initialize);
	}

	void _Attach(BarberPole* pole)
	{
		AutoLocker<BLocker> locker(fLock);

		bool wasEmpty = fMessengers.IsEmpty();

		BMessenger* messenger = new BMessenger(pole);
		fMessengers.AddItem(messenger);

		if (wasEmpty)
			release_sem(fSpinLoopLock);
	}

	void _Detach(BarberPole* pole)
	{
		AutoLocker<BLocker> locker(fLock);

		for (int32 i = 0; i < fMessengers.CountItems(); i++) {
			BMessenger* messenger = fMessengers.ItemAt(i);
			if (messenger->Target(NULL) == pole) {
				fMessengers.RemoveItem(messenger, true);
				break;
			}
		}

		if (fMessengers.IsEmpty())
			acquire_sem(fSpinLoopLock);
	}

	void _SpinLoop()
	{
		for (;;) {
			AutoLocker<BLocker> locker(fLock);

			for (int32 i = 0; i < fMessengers.CountItems(); i++) {
				BMessenger* messenger = fMessengers.ItemAt(i);
				messenger->SendMessage(BarberPole::kRefreshMessage);
			}

			locker.Unset();

			acquire_sem(fSpinLoopLock);
			release_sem(fSpinLoopLock);

			snooze(kSpinInterval);
		}
	}

private:
	static MachineRoom*		sInstance;
	static pthread_once_t	sOnceControl;

	thread_id				fSpinLoopThread;
	sem_id					fSpinLoopLock;

	BLocker					fLock;
	BObjectList<BMessenger>	fMessengers;
};


MachineRoom* MachineRoom::sInstance = NULL;
pthread_once_t MachineRoom::sOnceControl = PTHREAD_ONCE_INIT;


// #pragma mark - BarberPole


BarberPole::BarberPole(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FRAME_EVENTS),
	fIsSpinning(false),
	fSpinSpeed(0.05),
	fColors(NULL),
	fNumColors(0),
	fScrollOffset(0.0),
	fStripeWidth(0.0),
	fNumStripes(0)
{
	// Default colors, chosen from system color scheme
	rgb_color defaultColors[2];
	rgb_color otherColor = tint_color(ui_color(B_STATUS_BAR_COLOR), 1.3);
	otherColor.alpha = 50;
	defaultColors[0] = otherColor;
	defaultColors[1] = B_TRANSPARENT_COLOR;
	SetColors(defaultColors, 2);
}


BarberPole::~BarberPole()
{
	Stop();
	delete[] fColors;
}


void
BarberPole::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kRefreshMessage:
			_Spin();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
BarberPole::Draw(BRect updateRect)
{
	if (fIsSpinning)
		_DrawSpin(updateRect);
	else
		_DrawNonSpin(updateRect);
}


void
BarberPole::_DrawSpin(BRect updateRect)
{
	// Draw color stripes
	float position = -fStripeWidth * (fNumColors + 0.5) + fScrollOffset;
		// Starting position: beginning of the second color cycle
		// The + 0.5 is so we start out without a partially visible stripe
		// on the left side (makes it simpler to loop)
	BRect bounds = Bounds();
	bounds.InsetBy(-2, -2);
	be_control_look->DrawStatusBar(this, bounds, updateRect,
		ui_color(B_PANEL_BACKGROUND_COLOR), ui_color(B_STATUS_BAR_COLOR),
		bounds.Width());
	SetDrawingMode(B_OP_ALPHA);
	uint32 colorIndex = 0;
	for (uint32 i = 0; i < fNumStripes; i++) {
		SetHighColor(fColors[colorIndex]);
		colorIndex++;
		if (colorIndex >= fNumColors)
			colorIndex = 0;

		BRect stripeFrame = fStripe.Frame();
		fStripe.MapTo(stripeFrame,
			stripeFrame.OffsetToCopy(position, 0.0));
		FillPolygon(&fStripe);

		position += fStripeWidth;
	}

	SetDrawingMode(B_OP_COPY);
	// Draw box around it
	bounds = Bounds();
	be_control_look->DrawBorder(this, bounds, updateRect,
		ui_color(B_PANEL_BACKGROUND_COLOR), B_PLAIN_BORDER);
}


/*! This will show something in the place of the spinner when there is no
    spinning going on.  The logic to render the striped background comes
    from the 'drivesetup' application.
*/

void
BarberPole::_DrawNonSpin(BRect updateRect)
{
	// approach copied from the DiskSetup application.
	static const pattern kStripes = { { 0xc7, 0x8f, 0x1f, 0x3e, 0x7c,
		0xf8, 0xf1, 0xe3 } };
	BRect bounds = Bounds();
	SetHighUIColor(B_PANEL_BACKGROUND_COLOR, B_DARKEN_1_TINT);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
	FillRect(bounds, kStripes);
	be_control_look->DrawBorder(this, bounds, updateRect,
		ui_color(B_PANEL_BACKGROUND_COLOR), B_PLAIN_BORDER);
}


void
BarberPole::FrameResized(float width, float height)
{
	// Choose stripe width so that at least 2 full stripes fit into the view,
	// but with a minimum of 5px. Larger views get wider stripes, but they
	// grow slower than the view and are capped to a maximum of 200px.
	fStripeWidth = (width / 4) + 5;
	if (fStripeWidth > 200)
		fStripeWidth = 200;

	BPoint stripePoints[4];
	stripePoints[0].Set(fStripeWidth * 0.5, 0.0); // top left
	stripePoints[1].Set(fStripeWidth * 1.5, 0.0); // top right
	stripePoints[2].Set(fStripeWidth, height);    // bottom right
	stripePoints[3].Set(0.0, height);             // bottom left

	fStripe = BPolygon(stripePoints, 4);

	fNumStripes = (int32)ceilf((width) / fStripeWidth) + 1 + fNumColors;
		// Number of color stripes drawn in total for the barber pole, the
		// user-visible part is a "window" onto the complete pole. We need
		// as many stripes as are visible, an extra one on the right side
		// (will be partially visible, that's the + 1); and then a whole color
		// cycle of strips extra which we scroll into until we loop.
		//
		// Example with 3 colors and a visible area of 2*fStripeWidth (which means
		// that 2 will be fully visible, and a third one partially):
		//               ........
		//   X___________v______v___
		//  / 1 / 2 / 3 / 1 / 2 / 3 /
		//  `````````````````````````
		// Pole is scrolled to the right into the visible region, which is marked
		// between the two 'v'. Once the left edge of the visible area reaches
		// point X, we can jump back to the initial region position.
}


BSize
BarberPole::MinSize()
{
	BSize result = BView::MinSize();

	if (result.width < 50)
		result.SetWidth(50);

	if (result.height < 5)
		result.SetHeight(5);

	return result;
}


void
BarberPole::Start()
{
	if (fIsSpinning)
		return;
	MachineRoom::AttachBarberPole(this);
	fIsSpinning = true;
}


void
BarberPole::Stop()
{
	if (!fIsSpinning)
		return;
	MachineRoom::DetachBarberPole(this);
	fIsSpinning = false;
	Invalidate();
}


void
BarberPole::SetSpinSpeed(float speed)
{
	if (speed > 1.0f)
		speed = 1.0f;
	if (speed < -1.0f)
		speed = -1.0f;
	fSpinSpeed = speed;
}


void
BarberPole::SetColors(const rgb_color* colors, uint32 numColors)
{
	delete[] fColors;
	rgb_color* colorsCopy = new rgb_color[numColors];
	for (uint32 i = 0; i < numColors; i++)
		colorsCopy[i] = colors[i];

	fColors = colorsCopy;
	fNumColors = numColors;
}


void
BarberPole::_Spin()
{
	fScrollOffset += fStripeWidth / (1.0f / fSpinSpeed);
	if (fScrollOffset >= fStripeWidth * fNumColors) {
		// Cycle completed, jump back to where we started
		fScrollOffset = 0;
	}
	Invalidate();
	//Parent()->Invalidate();
}

/*
 * Copyright 2008-2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2022-2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Augustin Cavalier <waddlesplash@gmail.com>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Fredrik Holmqvist <fredrik.holmqvist@gmail.com>
 *		Michael Kanis <mks@skweez.net>
 *		Sylvain Kerjean <sylvain_kerjean@hotmail.com>
 *		Murai Takashi <tmurai01@gmail.com>
 *		PawanYr <pawan.yerramilli@gmail.com>
 *		Samuel Rodríguez Pérez <samuelrp84@gmail.com>
 *		Clemens Zeidler <clemens.zeidler@googlemail.com>
 */

#include "movement_maker.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <private/utils/BitUtils.h>

//#define TRACE_MOVEMENT_MAKER

#ifdef TRACE_MOVEMENT_MAKER
#	include <String.h>
#	include <headers/private/shared/FunctionTracer.h>

	static int32 sFunctionDepth = -1;
#	define CALLED(x...)	FunctionTracer _ft(debug_printf, this, __PRETTY_FUNCTION__, sFunctionDepth)
#	define TRACE(x...)	do { BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							char _extra[1024]; \
							sprintf(_extra, x); \
							debug_printf("%p -> %s%s", this, _to.String(), _extra); \
						} while (0)
#	define LOG_EVENT(text...) do {} while (0)
#	define LOG_ERROR(text...) TRACE(text)
#else
#	define TRACE(x...)
#	define CALLED(x...)
#	define LOG_ERROR(x...)  do { debug_printf(x); } while(0)
#	define LOG_EVENT(x...)
#endif


// magic constants
#define SYN_WIDTH				(4100)
#define SYN_HEIGHT				(3140)


static int32
make_small(float value)
{
	return (int32)truncf(value);
}


void
MovementMaker::SetSettings(const touchpad_settings& settings)
{
	CALLED();

	fSettings = settings;
}


void
MovementMaker::SetSpecs(const touchpad_specs& specs)
{
	CALLED();

	fSpecs = specs;

	fAreaWidth = fSpecs.areaEndX - fSpecs.areaStartX;
	fAreaHeight = fSpecs.areaEndY - fSpecs.areaStartY;

	// calibrated on the synaptics touchpad
	fSpeed = SYN_WIDTH / fAreaWidth;
	fSmallMovement = 3 / fSpeed;
}


void
MovementMaker::StartNewMovment()
{
	CALLED();

	if (fSettings.scroll_xstepsize <= 0)
		fSettings.scroll_xstepsize = 1;
	if (fSettings.scroll_ystepsize <= 0)
		fSettings.scroll_ystepsize = 1;

	fMovementMakerStarted = true;
	scrolling_x = 0;
	scrolling_y = 0;
}


void
MovementMaker::GetMovement(uint32 posX, uint32 posY)
{
	CALLED();

	_GetRawMovement(posX, posY);
}


void
MovementMaker::GetScrolling(uint32 posX, uint32 posY)
{
	CALLED();

	int32 stepsX = 0, stepsY = 0;
	int32 directionMultiplier = fSettings.scroll_reverse ? -1 : 1;

	_GetRawMovement(posX, posY);
	_ComputeAcceleration(fSettings.scroll_acceleration);

	if (fSettings.scroll_xstepsize > 0) {
		scrolling_x += directionMultiplier * xDelta;

		stepsX = make_small(scrolling_x / fSettings.scroll_xstepsize);

		scrolling_x -= stepsX * fSettings.scroll_xstepsize;
		xDelta = stepsX;
	} else {
		scrolling_x = 0;
		xDelta = 0;
	}

	if (fSettings.scroll_ystepsize > 0) {
		scrolling_y += directionMultiplier * yDelta;

		stepsY = make_small(scrolling_y / fSettings.scroll_ystepsize);

		scrolling_y -= stepsY * fSettings.scroll_ystepsize;
		yDelta = -1 * stepsY;
	} else {
		scrolling_y = 0;
		yDelta = 0;
	}
}


void
MovementMaker::_GetRawMovement(uint32 posX, uint32 posY)
{
	CALLED();

	// calibrated on the synaptics touchpad
	posX = posX * SYN_WIDTH / fAreaWidth;
	posY = posY * SYN_HEIGHT / fAreaHeight;

	const float acceleration = 0.8;
	const float translation = 12.0;

	int diff;

	if (fMovementMakerStarted) {
		fMovementMakerStarted = false;
		// init delta tracking
		fPreviousX = posX;
		fPreviousY = posY;
		// deltas are automatically reset
	}

	// accumulate delta and store current pos, reset if pos did not change
	diff = posX - fPreviousX;
	// lessen the effect of small diffs
	if ((diff > -fSmallMovement && diff < -1)
		|| (diff > 1 && diff < fSmallMovement)) {
		diff /= 2;
	}
	if (diff == 0)
		fDeltaSumX = 0;
	else
		fDeltaSumX += diff;

	diff = posY - fPreviousY;
	// lessen the effect of small diffs
	if ((diff > -fSmallMovement && diff < -1)
		|| (diff > 1 && diff < fSmallMovement)) {
		diff /= 2;
	}
	if (diff == 0)
		fDeltaSumY = 0;
	else
		fDeltaSumY += diff;

	fPreviousX = posX;
	fPreviousY = posY;

	// compute current delta and reset accumulated delta if
	// abs() is greater than 1
	xDelta = fDeltaSumX / translation;
	yDelta = fDeltaSumY / translation;
	if (xDelta > 1.0) {
		fDeltaSumX = 0.0;
		xDelta = 1.0 + (xDelta - 1.0) * acceleration;
	} else if (xDelta < -1.0) {
		fDeltaSumX = 0.0;
		xDelta = -1.0 + (xDelta + 1.0) * acceleration;
	}

	if (yDelta > 1.0) {
		fDeltaSumY = 0.0;
		yDelta = 1.0 + (yDelta - 1.0) * acceleration;
	} else if (yDelta < -1.0) {
		fDeltaSumY = 0.0;
		yDelta = -1.0 + (yDelta + 1.0) * acceleration;
	}

	xDelta = make_small(xDelta);
	yDelta = make_small(yDelta);
}


void
MovementMaker::_ComputeAcceleration(int8 accel_factor)
{
	CALLED();

	// acceleration
	float acceleration = 1;
	if (accel_factor != 0) {
		acceleration = 1 + sqrtf(xDelta * xDelta
			+ yDelta * yDelta) * accel_factor / 50.0;
	}

	xDelta = make_small(xDelta * acceleration);
	yDelta = make_small(yDelta * acceleration);
}


// #pragma mark -


#define fTapTimeOUT			200000


TouchpadMovement::TouchpadMovement()
{
	CALLED();

	fMovementStarted = false;
	fScrollingStarted = false;
	fTapStarted = false;
	fValidEdgeMotion = false;
	fDoubleClick = false;
}

TouchpadMovement::~TouchpadMovement() {
	CALLED();
}


status_t
TouchpadMovement::EventToMovement(const touchpad_movement* event, mouse_movement* movement,
	bigtime_t& repeatTimeout)
{
	CALLED();

	if (!movement)
		return B_ERROR;

	TRACE("TM_EVENT: b:0x%" B_PRIx8 " nf:%" B_PRId8 " f:0x%" B_PRIx8
		" x:%" B_PRIu32 " y:%" B_PRIu32 " p:%" B_PRIu8 " w:%" B_PRIu8 "\n",
		event->buttons,
		count_set_bits(event->fingers),
		event->fingers,
		event->xPosition,
		event->yPosition,
		event->zPressure,
		event->fingerWidth
	);
	TRACE("TM_STATUS: b:0x%" B_PRIx8 " %c%c%c%c%c%c"
		" dx:%" B_PRId32 " dy:%" B_PRId32
		" tcks:%" B_PRId32 " cks:%" B_PRId32 "\n",
		fButtonsState,

		fMovementStarted	? 'M' : 'm',
		fScrollingStarted	? 'S' : 's',
		fTapStarted			? 'T' : 't',
		fTapdragStarted		? 'D' : 'd',
		fValidEdgeMotion	? 'E' : 'e',
		fDoubleClick		? 'C' : 'c',

		fTapDeltaX,
		fTapDeltaY,
		fTapClicks,
		fClickCount
	);

	movement->xdelta = 0;
	movement->ydelta = 0;
	movement->buttons = 0;
	movement->wheel_ydelta = 0;
	movement->wheel_xdelta = 0;
	movement->modifiers = 0;
	movement->clicks = 0;
	movement->timestamp = system_time();

	if ((movement->timestamp - fTapTime) > fTapTimeOUT) {
		if (fTapStarted)
			TRACE("TouchpadMovement: tap gesture timed out\n");
		fTapStarted = false;
		if (!fDoubleClick
			|| (movement->timestamp - fTapTime) > 2 * fTapTimeOUT) {
			fTapClicks = 0;
		}
	}

	if (event->buttons & kLeftButton) {
		fTapClicks = 0;
		fTapdragStarted = false;
		fTapStarted = false;
		fValidEdgeMotion = false;
	}

	if (event->zPressure >= fSpecs.minPressure
		&& event->zPressure < fSpecs.maxPressure
		&& ((event->fingerWidth >= 4 && event->fingerWidth <= 7)
			|| event->fingerWidth == 0 || event->fingerWidth == 1)
		&& (event->xPosition != 0 || event->yPosition != 0)) {
		// The touch pad is in touch with at least one finger
		if (!_CheckScrollingToMovement(event, movement))
			_MoveToMovement(event, movement);
	} else
		_NoTouchToMovement(event, movement);


	if (fTapdragStarted || fValidEdgeMotion) {
		// We want the current event to be repeated in 50ms if no other
		// events occur in the interim.
		repeatTimeout = 1000 * 50;
	} else
		repeatTimeout = B_INFINITE_TIMEOUT;

	return B_OK;
}


// in pixel per second
const int32 kEdgeMotionSpeed = 200;


bool
TouchpadMovement::_EdgeMotion(const touchpad_movement *event, mouse_movement *movement,
	bool validStart)
{
	CALLED();

	float xdelta = 0;
	float ydelta = 0;

	bigtime_t time = system_time();
	if (fLastEdgeMotion != 0) {
		xdelta = fRestEdgeMotion + kEdgeMotionSpeed *
			float(time - fLastEdgeMotion) / (1000 * 1000);
		fRestEdgeMotion = xdelta - int32(xdelta);
		ydelta = xdelta;
	} else {
		fRestEdgeMotion = 0;
	}

	bool inXEdge = false;
	bool inYEdge = false;

	if (int32(event->xPosition) < fSpecs.areaStartX + fSpecs.edgeMotionWidth) {
		inXEdge = true;
		xdelta *= -1;
	} else if (event->xPosition > uint16(
		fSpecs.areaEndX - fSpecs.edgeMotionWidth)) {
		inXEdge = true;
	}

	if (int32(event->yPosition) < fSpecs.areaStartY + fSpecs.edgeMotionWidth) {
		inYEdge = true;
		ydelta *= -1;
	} else if (event->yPosition > uint16(
		fSpecs.areaEndY - fSpecs.edgeMotionWidth)) {
		inYEdge = true;
	}

	// for a edge motion the drag has to be started in the middle of the pad
	// TODO: this is difficult to understand simplify the code
	if (inXEdge && validStart)
		movement->xdelta = make_small(xdelta);
	if (inYEdge && validStart)
		movement->ydelta = make_small(ydelta);

	if (!inXEdge && !inYEdge)
		fLastEdgeMotion = 0;
	else
		fLastEdgeMotion = time;

	if ((inXEdge || inYEdge) && !validStart)
		return false;

	return true;
}


/*!	If a button has been clicked (movement->buttons must be set accordingly),
	this function updates the fClickCount, as well as the
	\a movement's clicks field.
	Also, it sets the button state from movement->buttons.
*/
void
TouchpadMovement::_UpdateButtons(mouse_movement *movement)
{
	CALLED();

	// set click count correctly according to double click timeout
	if (movement->buttons != 0 && fButtonsState == 0) {
		if (fClickLastTime + click_speed > movement->timestamp)
			fClickCount++;
		else
			fClickCount = 1;

		fClickLastTime = movement->timestamp;
	}

	if (movement->buttons != 0)
		movement->clicks = fClickCount;

	fButtonsState = movement->buttons;
}


void
TouchpadMovement::_NoTouchToMovement(const touchpad_movement *event,
	mouse_movement *movement)
{
	CALLED();

	uint32 buttons = event->buttons;

	if (fMovementStarted)
		TRACE("TouchpadMovement: no touch event\n");

	fScrollingStarted = false;
	fMovementStarted = false;
	fLastEdgeMotion = 0;

	if (fTapdragStarted
		&& (movement->timestamp - fTapTime) < fTapTimeOUT) {
		buttons = kLeftButton;
	}

	// if the movement stopped switch off the tap drag when timeout is expired
	if ((movement->timestamp - fTapTime) > fTapTimeOUT) {
		if (fTapdragStarted)
			TRACE("TouchpadMovement: tap drag gesture timed out\n");
		fTapdragStarted = false;
		fValidEdgeMotion = false;
	}

	if (abs(fTapDeltaX) > 15 || abs(fTapDeltaY) > 15) {
		fTapStarted = false;
		fTapClicks = 0;
	}

	if (fTapStarted || fDoubleClick) {
		TRACE("TouchpadMovement: tap gesture\n");
		fTapClicks++;

		if (fTapClicks > 1) {
			TRACE("TouchpadMovement: empty click\n");
			buttons = kNoButton;
			fTapClicks = 0;
			fDoubleClick = true;
		} else {
			buttons = kLeftButton;
			fTapStarted = false;
			fTapdragStarted = true;
			fDoubleClick = false;
		}
	}

	movement->buttons = buttons;
	_UpdateButtons(movement);
}


void
TouchpadMovement::_MoveToMovement(const touchpad_movement *event, mouse_movement *movement)
{
	CALLED();

	bool isStartOfMovement = false;
	float pressure = 0;

	TRACE("TouchpadMovement: movement event\n");
	if (!fMovementStarted) {
		isStartOfMovement = true;
		fMovementStarted = true;
		StartNewMovment();
	}

	GetMovement(event->xPosition, event->yPosition);

	movement->xdelta = make_small(xDelta);
	movement->ydelta = make_small(yDelta);

	// tap gesture
	fTapDeltaX += make_small(xDelta);
	fTapDeltaY += make_small(yDelta);

	if (fTapdragStarted) {
		movement->buttons = kLeftButton;
		movement->clicks = 0;

		fValidEdgeMotion = _EdgeMotion(event, movement, fValidEdgeMotion);
		TRACE("TouchpadMovement: tap drag\n");
	} else {
		TRACE("TouchpadMovement: movement set buttons\n");
		movement->buttons = event->buttons;
	}

	// use only a fraction of pressure range, the max pressure seems to be
	// to high
	pressure = 20 * (event->zPressure - fSpecs.minPressure)
		/ (fSpecs.realMaxPressure - fSpecs.minPressure);
	if (!fTapStarted
		&& isStartOfMovement
		&& fSettings.tapgesture_sensibility > 0.
		&& fSettings.tapgesture_sensibility > (20 - pressure)) {
		TRACE("TouchpadMovement: tap started\n");
		fTapStarted = true;
		fTapTime = system_time();
		fTapDeltaX = 0;
		fTapDeltaY = 0;
	}

	_UpdateButtons(movement);
}


/*!	Checks if this is a scrolling event or not, and also actually does the
	scrolling work if it is.

	\return \c true if this was a scrolling event, \c false if not.
*/
bool
TouchpadMovement::_CheckScrollingToMovement(const touchpad_movement *event,
	mouse_movement *movement)
{
	CALLED();

	bool isSideScrollingV = false;
	bool isSideScrollingH = false;

	// if a button is pressed don't allow to scroll, we likely be in a drag
	// action
	if (fButtonsState != 0)
		return false;

	if ((fSpecs.areaEndX - fAreaWidth * fSettings.scroll_rightrange
			< event->xPosition && !fMovementStarted
		&& fSettings.scroll_rightrange > 0.000001)
			|| fSettings.scroll_rightrange > 0.999999) {
		isSideScrollingV = true;
	}
	if ((fSpecs.areaStartY + fAreaHeight * fSettings.scroll_bottomrange
				> event->yPosition && !fMovementStarted
			&& fSettings.scroll_bottomrange > 0.000001)
				|| fSettings.scroll_bottomrange > 0.999999) {
		isSideScrollingH = true;
	}
	if ((event->fingerWidth == 0 || event->fingerWidth == 1)
		&& fSettings.scroll_twofinger) {
		// two finger scrolling is enabled
		isSideScrollingV = true;
		isSideScrollingH = fSettings.scroll_twofinger_horizontal;
	}

	if (!isSideScrollingV && !isSideScrollingH) {
		fScrollingStarted = false;
		return false;
	}

	TRACE("TouchpadMovement: scroll event\n");

	fTapStarted = false;
	fTapClicks = 0;
	fTapdragStarted = false;
	fValidEdgeMotion = false;
	if (!fScrollingStarted) {
		fScrollingStarted = true;
		StartNewMovment();
	}
	GetScrolling(event->xPosition, event->yPosition);
	movement->wheel_ydelta = make_small(yDelta);
	movement->wheel_xdelta = make_small(xDelta);

	if (isSideScrollingV && !isSideScrollingH)
		movement->wheel_xdelta = 0;
	else if (isSideScrollingH && !isSideScrollingV)
		movement->wheel_ydelta = 0;

	fButtonsState = movement->buttons;

	return true;
}

#include "movement_maker.h"

#include <stdlib.h>

#include <KernelExport.h>


#if 1
#	define INFO(x...) dprintf(x)
#else
#	define INFO(x...)
#endif

// #define TRACE_MOVEMENT_MAKER
#ifdef TRACE_MOVEMENT_MAKER
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


typedef union {
  float value;
  /* FIXME: Assumes 32 bit int.  */
  unsigned int word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)					\
do {								\
  ieee_float_shape_type gf_u;					\
  gf_u.value = (d);						\
  (i) = gf_u.word;						\
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)					\
do {								\
  ieee_float_shape_type sf_u;					\
  sf_u.word = (i);						\
  (d) = sf_u.value;						\
} while (0)

static const float huge = 1.0e30;

float
floorf(float x)
{
	int32 i0,j0;
	uint32 i;
	GET_FLOAT_WORD(i0,x);
	j0 = ((i0>>23)&0xff)-0x7f;
	if (j0<23) {
	    if (j0<0) { 	/* raise inexact if x != 0 */
		if (huge+x>(float)0.0) {/* return 0*sign(x) if |x|<1 */
		    if (i0>=0) {i0=0;}
		    else if ((i0&0x7fffffff)!=0)
			{ i0=0xbf800000;}
		}
	    } else {
		i = (0x007fffff)>>j0;
		if ((i0&i)==0) return x; /* x is integral */
		if (huge+x>(float)0.0) {	/* raise inexact flag */
		    if (i0<0) i0 += (0x00800000)>>j0;
		    i0 &= (~i);
		}
	    }
	} else {
	    if (j0==0x80) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	SET_FLOAT_WORD(x,i0);
	return x;
}


float
ceilf(float x)
{
	int32 i0,j0;
	uint32 i;

	GET_FLOAT_WORD(i0,x);
	j0 = ((i0>>23)&0xff)-0x7f;
	if (j0<23) {
	    if (j0<0) { 	/* raise inexact if x != 0 */
		if (huge+x>(float)0.0) {/* return 0*sign(x) if |x|<1 */
		    if (i0<0) {i0=0x80000000;}
		    else if (i0!=0) { i0=0x3f800000;}
		}
	    } else {
		i = (0x007fffff)>>j0;
		if ((i0&i)==0) return x; /* x is integral */
		if (huge+x>(float)0.0) {	/* raise inexact flag */
		    if (i0>0) i0 += (0x00800000)>>j0;
		    i0 &= (~i);
		}
	    }
	} else {
	    if (j0==0x80) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	SET_FLOAT_WORD(x,i0);
	return x;
}

static  const float        one        = 1.0, tiny=1.0e-30;

float
sqrtf(float x)
{
        float z;
        int32 sign = (int)0x80000000;
        int32 ix,s,q,m,t,i;
        uint32 r;

        GET_FLOAT_WORD(ix,x);

    /* take care of Inf and NaN */
        if ((ix&0x7f800000)==0x7f800000) {
            return x*x+x;                /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                                           sqrt(-inf)=sNaN */
        }
    /* take care of zero */
        if (ix<=0) {
            if ((ix&(~sign))==0) return x;/* sqrt(+-0) = +-0 */
            else if (ix<0)
                return (x-x)/(x-x);                /* sqrt(-ve) = sNaN */
        }
    /* normalize x */
        m = (ix>>23);
        if (m==0) {                                /* subnormal x */
            for(i=0;(ix&0x00800000)==0;i++) ix<<=1;
            m -= i-1;
        }
        m -= 127;        /* unbias exponent */
        ix = (ix&0x007fffff)|0x00800000;
        if (m&1)        /* odd m, double x to make it even */
            ix += ix;
        m >>= 1;        /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
        ix += ix;
        q = s = 0;                /* q = sqrt(x) */
        r = 0x01000000;                /* r = moving bit from right to left */

        while(r!=0) {
            t = s+r;
            if (t<=ix) {
                s    = t+r;
                ix  -= t;
                q   += r;
            }
            ix += ix;
            r>>=1;
        }

    /* use floating add to find out rounding direction */
        if (ix!=0) {
            z = one-tiny; /* trigger inexact flag */
            if (z>=one) {
                z = one+tiny;
                if (z>one)
                    q += 2;
                else
                    q += (q&1);
            }
        }
        ix = (q>>1)+0x3f000000;
        ix += (m <<23);
        SET_FLOAT_WORD(z,ix);
        return z;
}


int32
make_small(float value)
{
	if (value > 0)
		return floorf(value);
	else
		return ceilf(value);
}



void
MovementMaker::SetSettings(touchpad_settings* settings)
{
	fSettings = settings;
}


void
MovementMaker::SetSpecs(hardware_specs* specs)
{
	fSpecs = specs;

	fAreaWidth = fSpecs->areaEndX - fSpecs->areaStartX;
	fAreaHeight = fSpecs->areaEndY - fSpecs->areaStartY;

	// calibrated on the synaptics touchpad
	fSpeed = 4100 / fAreaWidth;
	fSmallMovement = 3 / fSpeed;
}


void
MovementMaker::StartNewMovment()
{
	if (fSettings->scroll_xstepsize <= 0)
		fSettings->scroll_xstepsize = 1;
	if (fSettings->scroll_ystepsize <= 0)
		fSettings->scroll_ystepsize = 1;

	movementStarted = true;
	scrolling_x = 0;
	scrolling_y = 0;
}


void
MovementMaker::GetMovement(uint32 posX, uint32 posY)
{
	_GetRawMovement(posX, posY);

//	INFO("SYN: pos: %lu x %lu, delta: %ld x %ld, sums: %ld x %ld\n",
//		posX, posY, xDelta, yDelta,
//		deltaSumX, deltaSumY);

	xDelta = xDelta;
	yDelta = yDelta;
}


void
MovementMaker::GetScrolling(uint32 posX, uint32 posY)
{
	int32 stepsX = 0, stepsY = 0;

	_GetRawMovement(posX, posY);
	_ComputeAcceleration(fSettings->scroll_acceleration);

	if (fSettings->scroll_xstepsize > 0) {
		scrolling_x += xDelta;

		stepsX = make_small(scrolling_x / fSettings->scroll_xstepsize);

		scrolling_x -= stepsX * fSettings->scroll_xstepsize;
		xDelta = stepsX;
	} else {
		scrolling_x = 0;
		xDelta = 0;
	}
	if (fSettings->scroll_ystepsize > 0) {
		scrolling_y += yDelta;

		stepsY = make_small(scrolling_y / fSettings->scroll_ystepsize);

		scrolling_y -= stepsY * fSettings->scroll_ystepsize;
		yDelta = -1 * stepsY;
	} else {
		scrolling_y = 0;
		yDelta = 0;
	}
}


void
MovementMaker::_GetRawMovement(uint32 posX, uint32 posY)
{
	// calibrated on the synaptics touchpad
	posX = posX * float(SYN_WIDTH) / fAreaWidth;
	posY = posY * float(SYN_HEIGHT) / fAreaHeight;

	const float acceleration = 0.8;
	const float translation = 12.0;

	int diff;

	if (movementStarted) {
		movementStarted = false;
		// init delta tracking
		previousX = posX;
		previousY = posY;
		// deltas are automatically reset
	}

	// accumulate delta and store current pos, reset if pos did not change
	diff = posX - previousX;
	// lessen the effect of small diffs
	if ((diff > -fSmallMovement && diff < -1)
		|| (diff > 1 && diff < fSmallMovement)) {
		diff /= 2;
	}
	if (diff == 0)
		deltaSumX = 0.0;
	else
		deltaSumX += diff;

	diff = posY - previousY;
	// lessen the effect of small diffs
	if ((diff > -fSmallMovement && diff < -1)
		|| (diff > 1 && diff < fSmallMovement)) {
		diff /= 2;
	}
	if (diff == 0)
		deltaSumY = 0.0;
	else
		deltaSumY += diff;

	previousX = posX;
	previousY = posY;

	// compute current delta and reset accumulated delta if
	// abs() is greater than 1
	xDelta = deltaSumX / translation;
	yDelta = deltaSumY / translation;
	if (xDelta > 1.0) {
		deltaSumX = 0.0;
		xDelta = 1.0 + (xDelta - 1.0) * acceleration;
	} else if (xDelta < -1.0) {
		deltaSumX = 0.0;
		xDelta = -1.0 + (xDelta + 1.0) * acceleration;
	}

	if (yDelta > 1.0) {
		deltaSumY = 0.0;
		yDelta = 1.0 + (yDelta - 1.0) * acceleration;
	} else if (yDelta < -1.0) {
		deltaSumY = 0.0;
		yDelta = -1.0 + (yDelta + 1.0) * acceleration;
	}

	xDelta = make_small(xDelta);
	yDelta = make_small(yDelta);
}



void
MovementMaker::_ComputeAcceleration(int8 accel_factor)
{
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


#define TAP_TIMEOUT			200000


void
TouchpadMovement::Init()
{
	movement_started = false;
	scrolling_started = false;
	tap_started = false;
	valid_edge_motion = false;
	double_click = false;
}


status_t
TouchpadMovement::EventToMovement(touch_event *event, mouse_movement *movement)
{
	if (!movement)
		return B_ERROR;

	movement->xdelta = 0;
	movement->ydelta = 0;
	movement->buttons = 0;
	movement->wheel_ydelta = 0;
	movement->wheel_xdelta = 0;
	movement->modifiers = 0;
	movement->clicks = 0;
	movement->timestamp = system_time();

	if ((movement->timestamp - tap_time) > TAP_TIMEOUT) {
		TRACE("ALPS: tap gesture timed out\n");
		tap_started = false;
		if (!double_click
			|| (movement->timestamp - tap_time) > 2 * TAP_TIMEOUT) {
			tap_clicks = 0;
		}
	}

	if (event->buttons & kLeftButton) {
		tap_clicks = 0;
		tapdrag_started = false;
		tap_started = false;
		valid_edge_motion = false;
	}

	if (event->zPressure >= fSpecs->minPressure
		&& event->zPressure < fSpecs->maxPressure
		&& ((event->wValue >= 4 && event->wValue <= 7)
			|| event->wValue == 0 || event->wValue == 1)
		&& (event->xPosition != 0 || event->yPosition != 0)) {
		// The touch pad is in touch with at least one finger
		if (!_CheckScrollingToMovement(event, movement))
			_MoveToMovement(event, movement);
	} else
		_NoTouchToMovement(event, movement);

	return B_OK;
}


bool
TouchpadMovement::_EdgeMotion(mouse_movement *movement, touch_event *event,
	bool validStart)
{
	int32 xdelta = 0;
	int32 ydelta = 0;

	if (event->xPosition < fSpecs->areaStartX + fSpecs->edgeMotionWidth)
		xdelta = -fSpecs->edgeMotionSpeedFactor * fSpeed;
	else if (event->xPosition > uint16(
		fSpecs->areaEndX - fSpecs->edgeMotionWidth)) {
		xdelta = fSpecs->edgeMotionSpeedFactor * fSpeed;
	}

	if (event->yPosition < fSpecs->areaStartY + fSpecs->edgeMotionWidth)
		ydelta = -fSpecs->edgeMotionSpeedFactor * fSpeed;
	else if (event->yPosition > uint16(
		fSpecs->areaEndY - fSpecs->edgeMotionWidth)) {
		ydelta = fSpecs->edgeMotionSpeedFactor * fSpeed;
	}

	if (xdelta && validStart)
		movement->xdelta = xdelta;
	if (ydelta && validStart)
		movement->ydelta = ydelta;

	if ((xdelta || ydelta) && !validStart)
		return false;

	return true;
}


/*!	If a button has been clicked (movement->buttons must be set accordingly),
	this function updates the click_count, as well as the
	\a movement's clicks field.
	Also, it sets the button state from movement->buttons.
*/
void
TouchpadMovement::_UpdateButtons(mouse_movement *movement)
{
	// set click count correctly according to double click timeout
	if (movement->buttons != 0 && buttons_state == 0) {
		if (click_last_time + click_speed > movement->timestamp)
			click_count++;
		else
			click_count = 1;

		click_last_time = movement->timestamp;
	}

	if (movement->buttons != 0)
		movement->clicks = click_count;

	buttons_state = movement->buttons;
}


void
TouchpadMovement::_NoTouchToMovement(touch_event *event,
	mouse_movement *movement)
{
	uint32 buttons = event->buttons;

	TRACE("ALPS: no touch event\n");

	scrolling_started = false;
	movement_started = false;

	if (tapdrag_started
		&& (movement->timestamp - tap_time) < TAP_TIMEOUT) {
		buttons = 0x01;
	}

	// if the movement stopped switch off the tap drag when timeout is expired
	if ((movement->timestamp - tap_time) > TAP_TIMEOUT) {
		tapdrag_started = false;
		valid_edge_motion = false;
		TRACE("ALPS: tap drag gesture timed out\n");
	}

	if (abs(tap_delta_x) > 15 || abs(tap_delta_y) > 15) {
		tap_started = false;
		tap_clicks = 0;
	}

	if (tap_started || double_click) {
		TRACE("ALPS: tap gesture\n");
		tap_clicks++;

		if (tap_clicks > 1) {
			TRACE("ALPS: empty click\n");
			buttons = 0x00;
			tap_clicks = 0;
			double_click = true;
		} else {
			buttons = 0x01;
			tap_started = false;
			tapdrag_started = true;
			double_click = false;
		}
	}

	movement->buttons = buttons;
	_UpdateButtons(movement);
}


void
TouchpadMovement::_MoveToMovement(touch_event *event, mouse_movement *movement)
{
	bool isStartOfMovement = false;
	float pressure = 0;

	TRACE("ALPS: movement event\n");
	if (!movement_started) {
		isStartOfMovement = true;
		movement_started = true;
		StartNewMovment();
	}

	GetMovement(event->xPosition, event->yPosition);

	movement->xdelta = xDelta;
	movement->ydelta = yDelta;

	// tap gesture
	tap_delta_x += xDelta;
	tap_delta_y += yDelta;

	if (tapdrag_started) {
		movement->buttons = kLeftButton;
		movement->clicks = 0;

		valid_edge_motion = _EdgeMotion(movement, event, valid_edge_motion);
		TRACE("ALPS: tap drag\n");
	} else {
		TRACE("ALPS: movement set buttons\n");
		movement->buttons = event->buttons;
	}

	// use only a fraction of pressure range, the max pressure seems to be
	// to high
	pressure = 20 * (event->zPressure - fSpecs->minPressure)
		/ (fSpecs->realMaxPressure - fSpecs->minPressure);
	if (!tap_started
		&& isStartOfMovement
		&& fSettings->tapgesture_sensibility > 0.
		&& fSettings->tapgesture_sensibility > (20 - pressure)) {
		TRACE("ALPS: tap started\n");
		tap_started = true;
		tap_time = system_time();
		tap_delta_x = 0;
		tap_delta_y = 0;
	}

	_UpdateButtons(movement);
}


/*!	Checks if this is a scrolling event or not, and also actually does the
	scrolling work if it is.

	\return \c true if this was a scrolling event, \c false if not.
*/
bool
TouchpadMovement::_CheckScrollingToMovement(touch_event *event,
	mouse_movement *movement)
{
	bool isSideScrollingV = false;
	bool isSideScrollingH = false;

	// if a button is pressed don't allow to scroll, we likely be in a drag
	// action
	if (buttons_state != 0)
		return false;

	if ((fSpecs->areaEndX - fAreaWidth * fSettings->scroll_rightrange
			< event->xPosition && !movement_started
		&& fSettings->scroll_rightrange > 0.000001)
			|| fSettings->scroll_rightrange > 0.999999) {
		isSideScrollingV = true;
	}
	if ((fSpecs->areaStartY + fAreaHeight * fSettings->scroll_bottomrange
				> event->yPosition && !movement_started
			&& fSettings->scroll_bottomrange > 0.000001)
				|| fSettings->scroll_bottomrange > 0.999999) {
		isSideScrollingH = true;
	}
	if ((event->wValue == 0 || event->wValue == 1)
		&& fSettings->scroll_twofinger) {
		// two finger scrolling is enabled
		isSideScrollingV = true;
		isSideScrollingH = fSettings->scroll_twofinger_horizontal;
	}

	if (!isSideScrollingV && !isSideScrollingH) {
		scrolling_started = false;
		return false;
	}

	TRACE("ALPS: scroll event\n");

	tap_started = false;
	tap_clicks = 0;
	tapdrag_started = false;
	valid_edge_motion = false;
	if (!scrolling_started) {
		scrolling_started = true;
		StartNewMovment();
	}
	GetScrolling(event->xPosition, event->yPosition);
	movement->wheel_ydelta = yDelta;
	movement->wheel_xdelta = xDelta;

	if (isSideScrollingV && !isSideScrollingH)
		movement->wheel_xdelta = 0;
	else if (isSideScrollingH && !isSideScrollingV)
		movement->wheel_ydelta = 0;

	buttons_state = movement->buttons;

	return true;
}

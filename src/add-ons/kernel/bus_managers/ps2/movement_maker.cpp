#include "movement_maker.h"

#include <stdlib.h>

#include <KernelExport.h>


//#define TRACE_MOVEMENT_MAKER
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


static int32
make_small(float value)
{
	if (value > 0)
		return (int32)floorf(value);
	else
		return (int32)ceilf(value);
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
	fSpeed = SYN_WIDTH / fAreaWidth;
	fSmallMovement = 3 / fSpeed;
}


void
MovementMaker::StartNewMovment()
{
	if (fSettings->scroll_xstepsize <= 0)
		fSettings->scroll_xstepsize = 1;
	if (fSettings->scroll_ystepsize <= 0)
		fSettings->scroll_ystepsize = 1;

	fMovementMakerStarted = true;
	scrolling_x = 0;
	scrolling_y = 0;
}


void
MovementMaker::GetMovement(uint32 posX, uint32 posY)
{
	_GetRawMovement(posX, posY);
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


void
TouchpadMovement::Init()
{
	fMovementStarted = false;
	fScrollingStarted = false;
	fTapStarted = false;
	fValidEdgeMotion = false;
	fDoubleClick = false;
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

	if ((movement->timestamp - fTapTime) > fTapTimeOUT) {
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


// in pixel per second
const int32 kEdgeMotionSpeed = 200;


bool
TouchpadMovement::_EdgeMotion(mouse_movement *movement, touch_event *event,
	bool validStart)
{
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

	if (int32(event->xPosition) < fSpecs->areaStartX + fSpecs->edgeMotionWidth) {
		inXEdge = true;
		xdelta *= -1;
	} else if (event->xPosition > uint16(
		fSpecs->areaEndX - fSpecs->edgeMotionWidth)) {
		inXEdge = true;
	}

	if (int32(event->yPosition) < fSpecs->areaStartY + fSpecs->edgeMotionWidth) {
		inYEdge = true;
		ydelta *= -1;
	} else if (event->yPosition > uint16(
		fSpecs->areaEndY - fSpecs->edgeMotionWidth)) {
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
TouchpadMovement::UpdateButtons(mouse_movement *movement)
{
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
TouchpadMovement::_NoTouchToMovement(touch_event *event,
	mouse_movement *movement)
{
	uint32 buttons = event->buttons;

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
		fTapdragStarted = false;
		fValidEdgeMotion = false;
		TRACE("TouchpadMovement: tap drag gesture timed out\n");
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
	UpdateButtons(movement);
}


void
TouchpadMovement::_MoveToMovement(touch_event *event, mouse_movement *movement)
{
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

		fValidEdgeMotion = _EdgeMotion(movement, event, fValidEdgeMotion);
		TRACE("TouchpadMovement: tap drag\n");
	} else {
		TRACE("TouchpadMovement: movement set buttons\n");
		movement->buttons = event->buttons;
	}

	// use only a fraction of pressure range, the max pressure seems to be
	// to high
	pressure = 20 * (event->zPressure - fSpecs->minPressure)
		/ (fSpecs->realMaxPressure - fSpecs->minPressure);
	if (!fTapStarted
		&& isStartOfMovement
		&& fSettings->tapgesture_sensibility > 0.
		&& fSettings->tapgesture_sensibility > (20 - pressure)) {
		TRACE("TouchpadMovement: tap started\n");
		fTapStarted = true;
		fTapTime = system_time();
		fTapDeltaX = 0;
		fTapDeltaY = 0;
	}

	UpdateButtons(movement);
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
	if (fButtonsState != 0)
		return false;

	if ((fSpecs->areaEndX - fAreaWidth * fSettings->scroll_rightrange
			< event->xPosition && !fMovementStarted
		&& fSettings->scroll_rightrange > 0.000001)
			|| fSettings->scroll_rightrange > 0.999999) {
		isSideScrollingV = true;
	}
	if ((fSpecs->areaStartY + fAreaHeight * fSettings->scroll_bottomrange
				> event->yPosition && !fMovementStarted
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

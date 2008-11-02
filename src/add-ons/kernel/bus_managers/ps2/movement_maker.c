#include "movement_maker.h"

#include <Debug.h>
#if 1
#	define INFO(x...) dprintf(x)
#else
#	define INFO(x...)
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
get_raw_movement(movement_maker *move, uint32 posX, uint32 posY)
{
	int diff;
	float xDelta, yDelta;
	const float acceleration = 0.7;

	if (move->movementStarted) {
		move->movementStarted = false;
		// init delta tracking
		move->previousX = posX;
		move->previousY = posY;
		// deltas are automatically reset
	}

	// accumulate delta and store current pos, reset if pos did not change
	diff = posX - move->previousX;
	// lessen the effect of small diffs
	if ((diff > -3 && diff < -1) || (diff > 1 && diff < 3))
		diff /= 2;
	if (diff == 0)
		move->deltaSumX = 0.0;
	else
		move->deltaSumX += diff;

	diff = posY - move->previousY;
	// lessen the effect of small diffs
	if ((diff > -3 && diff < -1) || (diff > 1 && diff < 3))
		diff /= 2;
	if (diff == 0)
		move->deltaSumY = 0.0;
	else
		move->deltaSumY += diff;

	move->previousX = posX;
	move->previousY = posY;

	// compute current delta and reset accumulated delta if
	// abs() is greater than 1
	xDelta = move->deltaSumX / 10.0;
	yDelta = move->deltaSumY / 10.0;
	if (xDelta > 1.0) {
		move->deltaSumX = 0.0;
		xDelta = 1.0 + (xDelta - 1.0) * acceleration;
	} else if (xDelta < -1.0) {
		move->deltaSumX = 0.0;
		xDelta = -1.0 + (xDelta + 1.0) * acceleration;
	}

	if (yDelta > 1.0) {
		move->deltaSumY = 0.0;
		yDelta = 1.0 + (yDelta - 1.0) * acceleration;
	} else if (yDelta < -1.0) {
		move->deltaSumY = 0.0;
		yDelta = -1.0 + (yDelta + 1.0) * acceleration;
	}

	move->xDelta = make_small(xDelta);
	move->yDelta = make_small(yDelta);
}


void
compute_acceleration(movement_maker *move, int8 accel_factor)
{
	// acceleration
	float acceleration = 1;
	if (accel_factor != 0) {
		acceleration = 1 + sqrtf(move->xDelta * move->xDelta
			+ move->yDelta * move->yDelta) * accel_factor / 50.0;
	}

	move->xDelta = make_small(move->xDelta * acceleration);
	move->yDelta = make_small(move->yDelta * acceleration);
}


void
get_movement(movement_maker *move, uint32 posX, uint32 posY)
{
	get_raw_movement(move, posX, posY);

//	INFO("SYN: pos: %lu x %lu, delta: %ld x %ld, sums: %ld x %ld\n",
//		posX, posY, move->xDelta, move->yDelta,
//		move->deltaSumX, move->deltaSumY);

	move->xDelta = move->xDelta * move->speed;
	move->yDelta = move->yDelta * move->speed;
}


void
get_scrolling(movement_maker *move, uint32 posX, uint32 posY)
{
	int32 stepsX = 0, stepsY = 0;

	get_raw_movement(move, posX, posY);
	compute_acceleration(move, move->scroll_acceleration);

	move->scrolling_x += move->xDelta;
	move->scrolling_y += move->yDelta;

	stepsX = make_small(move->scrolling_x / move->scrolling_xStep);
	stepsY = make_small(move->scrolling_y / move->scrolling_yStep);

	move->scrolling_x-= stepsX * move->scrolling_xStep;
	move->scrolling_y-= stepsY * move->scrolling_yStep;

	move->xDelta = stepsX;
	move->yDelta = -1 * stepsY;
}


void
start_new_movment(movement_maker *move)
{
	if (move->scrolling_xStep <= 0)
		move->scrolling_xStep = 1;
	if (move->scrolling_yStep <= 0)
		move->scrolling_yStep = 1;

	move->movementStarted = true;
	move->scrolling_x = 0;
	move->scrolling_y = 0;
}


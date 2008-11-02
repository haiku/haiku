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
	int16 i;
	float meanXOld = 0, meanYOld = 0;
	float meanX = 0, meanY = 0;

	// calculate mean
	for (i = 0; i < move->n_points; i++) {
		meanXOld += move->historyX[i];
		meanYOld += move->historyY[i];
	}
	if (move->n_points == 0) {
		meanXOld = posX;
		meanYOld = posY;
	} else {
		meanXOld = meanXOld / move->n_points;
		meanYOld = meanYOld / move->n_points;
	}

	meanX = (meanXOld + posX) / 2;
	meanY = (meanYOld + posY) / 2;

	// fill history
	for (i = 0; i < HISTORY_SIZE - 1; i++) {
		move->historyX[i] = move->historyX[i + 1];
		move->historyY[i] = move->historyY[i + 1];
	}
	move->historyX[HISTORY_SIZE - 1] = meanX;
	move->historyY[HISTORY_SIZE - 1] = meanY;

	if (move->n_points < HISTORY_SIZE) {
		move->n_points++;
		move->xDelta = 0;
		move->yDelta = 0;
		return;
	}

	move->xDelta = make_small((meanX - meanXOld) / 16);
	move->yDelta = make_small((meanY - meanYOld) / 16);
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
	move->xDelta = move->xDelta * move->speed;
	move->yDelta = move->yDelta * move->speed;
}


void
get_scrolling(movement_maker *move, uint32 posX, uint32 posY)
{
	int32 stepsX = 0, stepsY = 0;

	get_raw_movement(move, posX, posY);
	compute_acceleration(move, move->scroll_acceleration);

	move->scrolling_x+= move->xDelta;
	move->scrolling_y+= move->yDelta;

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

	move->n_points = 0;
	move->scrolling_x = 0;
	move->scrolling_y = 0;
}


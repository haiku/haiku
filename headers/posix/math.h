/* 
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _MATH_H_
#define _MATH_H_


#define M_E				2.7182818284590452354	/* e */
#define M_LOG2E			1.4426950408889634074	/* log 2e */
#define M_LOG10E		0.43429448190325182765	/* log 10e */
#define M_LN2			0.69314718055994530942	/* log e2 */
#define M_LN10			2.30258509299404568402	/* log e10 */
#define M_PI			3.14159265358979323846	/* pi */
#define M_PI_2			1.57079632679489661923	/* pi/2 */
#define M_PI_4			0.78539816339744830962	/* pi/4 */
#define M_1_PI			0.31830988618379067154	/* 1/pi */
#define M_2_PI			0.63661977236758134308	/* 2/pi */
#define M_2_SQRTPI		1.12837916709551257390	/* 2/sqrt(pi) */
#define M_SQRT2			1.41421356237309504880	/* sqrt(2) */
#define M_SQRT1_2		0.70710678118654752440	/* 1/sqrt(2) */

#define PI				M_PI
#define PI2				M_PI_2

/* platform independent IEEE floating point special values */
#define	__HUGE_VAL_v	0x7ff0000000000000LL
#define __huge_val_t	union { unsigned char __c[8]; long long __ll; double __d; }
#define HUGE_VAL		(((__huge_val_t) { __ll: __HUGE_VAL_v }).__d)

#define __HUGE_VALF_v	0x7f800000L
#define __huge_valf_t	union { unsigned char __c[4]; long __l; float __f; }
#define HUGE_VALF		(((__huge_valf_t) { __l: __HUGE_VALF_v }).__f)

// ToDo: define HUGE_VALL for long doubles

#define __NAN_VALF_v	0x7fc00000L
#define NAN				(((__huge_valf_t) { __l: __NAN_VALF_v }).__f)

#define INFINITY		HUGE_VALF

/* floating-point categories */
#define FP_NAN			0
#define FP_INFINITE		1
#define FP_ZERO			2
#define FP_SUBNORMAL	3
#define FP_NORMAL		4

#ifdef __cplusplus
struct __exception;
extern "C" int matherr(struct __exception *);
struct __exception {
#else
struct exception;
extern int matherr(struct exception *);
struct exception {
#endif
	int		type;
	char	*name;
	double	arg1;
	double	arg2;
	double	retval;
};

#define DOMAIN		1
#define SING		2
#define OVERFLOW	3
#define UNDERFLOW	4
#define TLOSS		5
#define PLOSS		6

extern int signgam;


#ifdef __cplusplus
extern "C" {
#endif

extern double	acos(double x);
extern double	asin(double x);
extern double	atan(double x);
extern double	atan2(double x, double y);
extern double	ceil(double x);
extern double	cos(double x);
extern double	cosh(double x);
extern double	exp(double x);
extern double	fabs(double x);
extern double	floor(double x);
extern double	fmod(double x, double y);
extern double	frexp(double x, int *_exponent);
extern double	ldexp(double x, int exponent);
extern double	log(double x);
extern double	log10(double x);
extern double	modf(double x, double *y);
extern double	pow(double x, double y);
extern double	sin(double x);
extern double	sinh(double x);
extern double	sqrt(double x);
extern double	tan(double x);
extern double	tanh(double x);

/* some BSD non-ANSI or POSIX math functions */
extern double	acosh(double x);
extern double	asinh(double x);
extern double	atanh(double x);
extern double	cbrt(double x);
extern double	erf(double x);
extern double	erfc(double x);
extern double	expm1(double x);
extern double	gamma(double x);
extern double	gamma_r(double x, int *y);
extern double	hypot(double x, double y);
extern int		ilogb(double x);
extern double	j0(double x);
extern double	j1(double x);
extern double	jn(int x, double y);
extern double	lgamma(double x);
extern double	lgamma_r(double x, int *y);
extern double	log1p(double x);
extern double	logb(double x);
extern double	nextafter(double x, double y);
extern double	remainder(double x, double y);
extern double	rint(double x);
extern double	scalb (double x, double y);
extern double	y0(double x);
extern double	y1(double x);
extern double	yn(int x, double y);

/* other stuff as defined in BeOS */
extern double	significand(double x);
extern double	copysign(double x, double y);
extern double	scalbn(double x, int y);
extern double	drem(double x, double y);
extern int		isnan(double x);
extern int		isfinite(double x);
extern int		finite(double x);
extern float	modff(float x, float *y);
extern float	acosf(float x);
extern float	asinf(float x);
extern float	atanf(float x);
extern float	atan2f(float y, float x);
extern float	cosf(float x);
extern float	sinf(float x);
extern float	tanf(float x);
extern float	coshf(float x);
extern float	sinhf(float x);
extern float	tanhf(float x);
extern float	acoshf(float x);
extern float	asinhf(float x);
extern float	atanhf(float x);
extern float	expf(float x);
extern float	frexpf(float x, int *_exponent);
extern float	ldexpf(float x, int exponent);
extern float	logf(float x);
extern float	log10f(float x);
extern float	expm1f(float x);
extern float	log1pf(float x);
extern float	logbf(float x);
extern float	powf(float x, float y);
extern float	sqrtf(float x);
extern float	hypotf(float x, float y);
extern float	cbrtf(float x);
extern float	ceilf(float x);
extern float	fabsf(float x);
extern float	floorf(float x);
extern float	fmodf(float x, float y);
extern int		isinff(float value);
extern int		finitef(float value);
extern float	infnanf(int error);
extern float	dremf(float x, float y);
extern float	significandf(float x);
extern float	copysignf(float x, float y);
extern int		isnanf(float value);
extern float	j0f(float x);
extern float	j1f(float x);
extern float	jnf(int x, float y);
extern float	y0f(float x);
extern float	y1f(float x);
extern float	ynf(int x, float y);
extern float	erff(float x);
extern float	erfcf(float x);
extern float	gammaf(float x);
extern float	lgammaf(float x);
extern float	gammaf_r(float x, int *y);
extern float	lgammaf_r(float x, int *y);
extern float	rintf(float x);
extern float	nextafterf(float x, float y);
extern float	remainderf(float x, float y);
extern float	scalbf(float x, float n);
extern float	scalbnf(float x, int n);
extern int		ilogbf(float x);

/* prototypes for functions used in the macros below */
extern int		__fpclassifyf(float value);
extern int		__signbitf(float value);
extern int		__finitef(float value);
extern int		__isnanf(float value);
extern int		__isinff(float value);

extern int		__fpclassifyl(long double value);
extern int		__signbitl(long double value);
extern int		__finitel(long double value);
extern int		__isnanl(long double value);
extern int		__isinfl(long double value);

extern int		__fpclassify(double value);
extern int		__signbit(double value);
extern int		__finite(double value);
extern int		__isnan(double value);
extern int		__isinf(double value);

/* returns number of classification appropriate for 'value' */
#define fpclassify(value) \
	(sizeof(value) == sizeof(float) ? __fpclassifyf(value)		\
		: sizeof(value) == sizeof(double) ? __fpclassify(value)	\
		: __fpclassifyl(value))

/* returns non-zero if 'value' is negative */
# define signbit(value) \
	(sizeof(value) == sizeof(float) ? __signbitf(value)			\
		: sizeof(value) == sizeof(double) ? __signbit(value)	\
		: __signbitl(value))

/* returns non-zero if 'value' is not Inf or NaN */
# define isfinite(value) \
	(sizeof(value) == sizeof(float) ? __finitef(value)			\
		: sizeof(value) == sizeof(double) ? __finite(value)		\
		: __finitel(value))

/* returns non-zero if 'value' is neither zero, sub-normal, Inf, nor NaN */
# define isnormal(value) \
	(fpclassify(value) == FP_NORMAL)

/* returns non-zero if 'value' is NaN */
# define isnan(value) \
	(sizeof(value) == sizeof(float) ? __isnanf(value)			\
		: sizeof(value) == sizeof(double) ? __isnan(value)		\
		: __isnanl(value))

/* returns non-zero if 'value is Inf */
# define isinf(x) \
	(sizeof(value) == sizeof(float) ? __isinff(value)			\
		: sizeof(value) == sizeof(double) ? __isinf(value)		\
		: __isinfl(value))

#ifdef __cplusplus
}
#endif

#endif	/* _MATH_H_ */

/*
This is a temporary file that adds the float versions of the math functions
to our libroot.so. They are required for binary compatibility.
These are generated from the inline versions on PPC in <math.h>.

TODO: Remove this file when we have all the functions replaced
*/

#include <math.h>

float acosf(float x) {return (float)acos((double)x);};
float asinf(float x) {return (float)asin((double)x);};
float atanf(float x) {return (float)atan((double)x);};
float atan2f(float y, float x) {return (float)atan2((double)y, (double)x);};
float cosf(float x) {return (float)cos((double)x);};
float sinf(float x) {return (float)sin((double)x);};
float tanf(float x) {return (float)tan((double)x);};
float coshf(float x) {return (float)cosh((double)x);};
float sinhf(float x) {return (float)sinh((double)x);};
float tanhf(float x) {return (float)tanh((double)x);};
float acoshf(float x) {return (float)acosh((double)x);};
float asinhf(float x) {return (float)asinh((double)x);};
float atanhf(float x) {return (float)atanh((double)x);};
float expf(float x) {return (float)exp((double)x);};
float frexpf(float x, int *exp) {return (float)frexp((double)x, exp);};
float ldexpf(float x, int exp) {return (float)ldexp((double)x, exp);};
float logf(float x) {return (float)log((double)x);};
float log10f(float x) {return (float)log10((double)x);};
float expm1f(float x) {return (float)expm1((double)x);};
float log1pf(float x) {return (float)log1p((double)x);};
float logbf(float x) {return (float)logb((double)x);};
float powf(float x, float y) {return (float)pow((double)x, (double)y);};
float sqrtf(float x) {return (float)sqrt((double)x);};
float hypotf(float x, float y) {return (float)hypot((double)x, (double)y);};
float fabsf(float x) {return (float)fabs((double)x);};
float fmodf(float x, float y) {return (float)fmod((double)x, (double)y);};
float copysignf(float x, float y) {return (float)copysign((double)x, (double)y);};
float erff(float x) {return (float)erf((double)x);};
float erfcf(float x) {return (float)erfc((double)x);};
float gammaf(float x) {return (float)gamma((double)x);};
float lgammaf(float x) {return (float)lgamma((double)x);};
float rintf(float x) {return (float)rint((double)x);};
float scalbf(float x, float n) {return (float)scalb((double)x, (double)n);};

#include "Utility.h"
#include <math.h>

rgb_color blackColor = { 0, 0, 0, 255 };
rgb_color greyColor = { 216, 216, 216, 255 };
rgb_color darkColor = { 184, 184, 184, 255 };
rgb_color whiteColor = { 255, 255, 255, 255 };
rgb_color redColor = { 228, 0, 0, 255 };
	
float round(float n, int32 max)
{	
	max = (int32)pow(10, (float)max);
	
	n *= max;
	n += 0.5;
	
	int32 tmp = (int32)floor(n);
	return (float)tmp / (max);
}

#include "Utility.h"
#include <math.h>
	
float round(float n, int32 max)
{	
	max = (int32)pow(10, (float)max);
	
	n *= max;
	n += 0.5;
	
	int32 tmp = (int32)floor(n);
	return (float)tmp / (max);
}

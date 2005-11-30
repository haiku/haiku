#include "clipping.h"
#include "Region.h"

#include <stdio.h>

int main()
{
	BRegion region(BRect(10, 10, 20, 20));
	
	BRegion other(BRect(15, 15, 50, 19));
	region.IntersectWith(&other);
	if (region.Intersects(other.Frame()))
		printf("Okay!\n");
		
	
	region.PrintToStream();
}


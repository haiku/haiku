#include "clipping.h"
#include "Region.h"

#include <stdio.h>

int main()
{
	BRegion region(BRect(10, 10, 20, 20));
	
	BRegion other(BRect(15, 15, 50, 19));
	other.IntersectWith(&region);
	//if (region.Intersects(other.Frame()))
	//	printf("Okay!\n");
		
	
	BRegion c;
	c.Set(BRect(10, 20, 15, 56));

	other.Include(&c);
	region.Include(&c);
	
	region.PrintToStream();
	other.PrintToStream();
}


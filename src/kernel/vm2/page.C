#include "page.h"

void page::zero(void)
{
	for (int i=0;i<(PAGE_SIZE/4);i++)
		((long *)physicalAddress)[i]=0;
}

#include "page.h"
#include "string.h"

void page::zero(void)
{
	memset(physicalAddress,'0',PAGE_SIZE);
//	for (int i=0;i<(PAGE_SIZE/4);i++)
//		((long *)physicalAddress)[i]=0;
}

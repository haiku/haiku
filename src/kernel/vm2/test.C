#include "vmInterface.h"
#include <stdio.h>

unsigned long addr;
vmInterface vm(10);

void writeByte(unsigned int offset, char value) { vm.setByte(addr+offset,value); }

unsigned char readByte(unsigned int offset ) { char value=vm.getByte(addr+offset); return value; }

int createFillAndTest(int pages)
{
	int area1;
	area1=vm.createArea("Mine",pages,(void **)(&addr));
	for (int i=0;i<pages*PAGE_SIZE;i++)
		writeByte(i,i%256);
	for (int i=0;i<pages*PAGE_SIZE;i++)
		if (i%256!=readByte(i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(i));
	return area1;
}

int main(int argc,char **argv)
{
	int area1,area2;
	printf ("Creating a new area1\n");
	area1=createFillAndTest(2);
	snooze(5000000);
	printf ("Creating a new area2\n");
	area2=createFillAndTest(2);
	snooze(500000);
	printf ("Freeing area1\n");
	vm.freeArea(area1);
	snooze(500000);
	printf ("Freeing area2\n");
	vm.freeArea(area2);
	printf ("Done Freeing area2\n");
	snooze(20000000);

	printf ("Creating a new area\n");
	area1=createFillAndTest(2);
	vm.freeArea(area1);
	return 0;
}

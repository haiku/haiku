#include "vmInterface.h"
#include <stdio.h>

unsigned long addr;
vmInterface vm(10);

void writeByte(unsigned int offset, char value)
{
	//printf ("writeByte: writing %d to offset %d\n",value,offset);
	vm.setByte(addr+offset,value);
}

unsigned char readByte(unsigned int offset )
{
	char value=vm.getByte(addr+offset);
	//printf ("readByte: read %d from offset %d\n",value,offset);
	return value;
}

int main(int argc,char **argv)
{
	int area1,area2;
	area1=vm.createArea("Mine",2,(void **)(&addr));
	writeByte(0,99);
	readByte(0);
	printf ("\n\n\n\n\n");
	writeByte(4097,99);
	readByte(4097);
	printf ("\n\n\n\n\n");
	for (int i=0;i<8192;i++)
		writeByte(i,i%256);
	for (int i=0;i<8192;i++)
		if (i%256!=readByte(i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(i));
	snooze(5000000);
	area2=vm.createArea("Mine2",2,(void **)(&addr));
	writeByte(0,99);
	readByte(0);
	printf ("\n\n\n\n\n");
	writeByte(4097,99);
	readByte(4097);
	printf ("\n\n\n\n\n");
	for (int i=0;i<8192;i++)
		writeByte(i,i%256);
	for (int i=0;i<8192;i++)
		if (i%256!=readByte(i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(i));
	snooze(500000);
	printf ("Freeing area1\n");
	vm.freeArea(area1);
	printf ("Freeing area2\n");
	vm.freeArea(area2);
	printf ("Done Freeing area2\n");
	snooze(20000000);

	printf ("Creating a new area\n");
	area1=vm.createArea("Mine",2,(void **)(&addr));
	writeByte(0,99);
	readByte(0);
	printf ("\n\n\n\n\n");
	writeByte(4097,99);
	readByte(4097);
	printf ("\n\n\n\n\n");
	for (int i=0;i<8192;i++)
		writeByte(i,i%256);
	for (int i=0;i<8192;i++)
		if (i%256!=readByte(i))
				printf ("ERROR! Byte at offset %d does not match: expected: %d, found: %d\n",i,i%256,readByte(i));
	return 0;
}

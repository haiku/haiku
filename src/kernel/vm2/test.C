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
	vm.createArea("Mine",2,(void **)(&addr));
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
	snooze(10000000);
	vm.createArea("Mine",2,(void **)(&addr));
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

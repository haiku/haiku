#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface *vm;

void writeByte(unsigned long addr,unsigned int offset, char value) { vm->setByte(addr+offset,value); }
unsigned char readByte(unsigned long addr,unsigned int offset ) { char value=vm->getByte(addr+offset); return value; }

int main(int argc,char **argv)
{
	try {
	vm = new vmInterface (90);
	
	int myfd=open("mmap_test_2_file",O_RDWR|O_CREAT,0707);
	if (myfd==-1)
		error ("file opening error = %s\n",strerror(errno));

	unsigned long result=(unsigned long)(vm->mmap(NULL,32768,PROT_WRITE|PROT_READ,MAP_SHARED,myfd,0));
	for (int i=0;i<16384;i++) {
		writeByte(result,i*2,'B');
		writeByte(result,2*i+1,' ');
	}
	vm->munmap((void *)result,32768);
	close (myfd);
//-----------------------------------------------------------------------------------------------------------
	myfd=open("mmap_test_2_file",O_RDONLY,0444);
	if (myfd==-1)
		error ("file reopening error = %s\n",strerror(errno));

	result=(unsigned long)(vm->mmap(NULL,32768,PROT_WRITE|PROT_READ,MAP_SHARED,myfd,0));

	for (int i=0;i<16384;i++) {
		if ('B' != readByte(result,i*2))
			error ("Non matching byte %c at offset %d\n");
		if (' ' != readByte(result,2*i+1))
			error ("Non matching byte %c at offset %d\n");
	}
	vm->munmap((void *)result,32768);
	close (myfd);
	}
	catch (const char *t)
	{
		error ("Exception thrown! %s\n",t);
		exit(1);
	}
	catch (...)
	{
		error ("Unknown Exception thrown!\n");
		exit(1);
	}

	return 0;
}

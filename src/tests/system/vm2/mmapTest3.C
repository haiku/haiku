#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface *vm;

void writeByte(unsigned long addr,unsigned int offset, char value) { vm->setByte(addr+offset,value); }
unsigned char readByte(unsigned long addr,unsigned int offset ) { return vm->getByte(addr+offset);}

int main(int argc,char **argv)
{
	try {
	vm = new vmInterface (90);
	
	system ("rm mmap_test_3_file");
	int myfd=open("mmap_test_3_file",O_RDWR|O_CREAT,0777);
	if (myfd==-1)
		error ("file opening error = %s\n",strerror(errno));

	unsigned long result=(unsigned long)(vm->mmap(NULL,32768,PROT_WRITE|PROT_READ,MAP_SHARED,myfd,0));
	error ("result = %x\n",result);
	for (int i=0;i<16384;i++) {
		writeByte(result,i*2,'B');
		writeByte(result,2*i+1,' ');
	}

	unsigned long result2=(unsigned long)(vm->mmap(NULL,32768,PROT_WRITE|PROT_READ,MAP_SHARED,myfd,0));
	error ("result2 = %x\n",result2);
	char a;
	for (int i=0;i<16384;i+=2048) {
		a = readByte(result2,i*2);
		if ('B' != a)
			error ("Non matching byte %c at offset %d\n",a,i*2);
		a = readByte(result2,i*2+1);
		if (' ' != a)
			error ("Non matching byte %c at offset %d\n",a,i*2+1);
	}
	vm->munmap((void *)result,32768);
	close (myfd);
	vm->munmap((void *)result2,32768);
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

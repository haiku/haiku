#include "vmInterface.h"
#include <stdio.h>

int main(int argc,char **argv)
{
	vmInterface vm(10);

	unsigned long addr;
	vm.createArea("Mine",2,(void **)(&addr));
	printf ("addr = %d\n",addr);
	vm.setByte(addr,99);
	printf ("addr = %d\n",addr);
	printf ("Byte = %d\n",vm.getByte(addr));
	return 0;
}

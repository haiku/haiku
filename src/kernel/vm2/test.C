#include "vmInterface.h"
#include <stdio.h>

int main(int argc,char **argv)
{
	vmInterface vm(10);

	void *addr;
	vm.createArea("Mine",1,&addr);
	return 0;
}

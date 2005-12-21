#include "vmInterface.h"
#include <stdio.h>
#include <unistd.h>
#include <mman.h>
#include <errno.h>
#include <string.h>

vmInterface *vm;

int main(int argc,char **argv)
{
	try {
	vm = new vmInterface (90);

	while (1)
		snooze(1000000);
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

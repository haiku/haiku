#include <stdio.h>
#include <OS.h>

int main(int argc, char **argv)
{
	area_id area, cloned;
	char *ptr, *p;
	area_info ai;

	if (argc < 2) {
		printf("catarea areaid\n");
		return 1;
	}
	area = atoi(argv[1]);

	cloned = clone_area("cloned for catarea", (void **)&ptr, B_ANY_ADDRESS, B_READ_AREA, area);
	get_area_info(cloned, &ai);
	//fprintf(stderr, "copy of bios: size=0x%08lx\n", ai.size);
	write(1, ptr, ai.size);
	delete_area(cloned);
	return 0;
}

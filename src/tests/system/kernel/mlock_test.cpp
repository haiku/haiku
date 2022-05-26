#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/OS.h>

#define SPACE_SIZE (B_PAGE_SIZE * 9)

int
main()
{
	void* space = NULL;
	area_id area = create_area("mlock test area", &space, B_EXACT_ADDRESS,
		SPACE_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	/* --------- */
	int result = mlock(space + B_PAGE_SIZE, B_PAGE_SIZE * 7);
	/* -xxxxxxx- */
	assert(result == 0);
	result = munlock(space + 2 * B_PAGE_SIZE, B_PAGE_SIZE * 5);
	/* -x-----x- */
	assert(result == 0);
	result = mlock(space + 2 * B_PAGE_SIZE, B_PAGE_SIZE * 3);
	/* -xxxx--x- */
	assert(result == 0);
	result = mlock(space, B_PAGE_SIZE * 9);
	/* xxxxxxxxx */
	assert(result == 0);
	result = munlock(space + 4 * B_PAGE_SIZE, B_PAGE_SIZE * 5);
	/* xxxx----- */
	assert(result == 0);
	result = munlock(space, B_PAGE_SIZE * 9);
	assert(result == 0);

	delete_area(area);
	return 0;
}

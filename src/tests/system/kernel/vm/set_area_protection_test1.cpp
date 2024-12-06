#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>


int
test_function()
{
	return 0;
}


static area_id
create_test_area(const char* name, int** address, uint32 protection)
{
	area_id area = create_area(name, (void**)address, B_ANY_ADDRESS,
		B_PAGE_SIZE, B_NO_LOCK, protection);
	if (area < 0) {
		fprintf(stderr, "Error: Failed to create area %s: %s\n", name,
			strerror(area));
		exit(1);
	}

	return area;
}


static area_id
clone_test_area(const char* name, int** address, uint32 protection,
	area_id source)
{
	area_id area = clone_area(name, (void**)address, B_ANY_ADDRESS,
		protection, source);
	if (area < 0) {
		fprintf(stderr, "Error: Failed to clone area %s: %s\n", name,
			strerror(area));
		exit(1);
	}

	return area;
}


int
main()
{
	// allocate read-only areas
	const int kAreaCount = 4;
	area_id areas[kAreaCount];
	int* areaAddresses[kAreaCount];

	areas[0] = create_test_area("area0", &areaAddresses[0], B_READ_AREA);
	areas[1] = create_test_area("area1", &areaAddresses[1], B_READ_AREA);
	areas[2] = create_test_area("area2", &areaAddresses[2], B_READ_AREA);
	areaAddresses[3] = (int*)test_function;
	areas[3] = area_for(areaAddresses[3]);

	int* area2CloneAddress;
	/*area_id area2Clone =*/ clone_test_area("area2clone", &area2CloneAddress,
		B_READ_AREA | B_WRITE_AREA, areas[2]);

	int area3Value = *areaAddresses[3];

	for (int i = 0; i < kAreaCount; i++) {
		printf("parent: areas[%d]: %ld, %p (0x%08x)\n", i, areas[i],
			areaAddresses[i], *areaAddresses[i]);
	}

	// fork()
	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Error: Failed to fork(): %s\n", strerror(errno));
		exit(1);
	}

	if (pid == 0) {
		// child
		pid = find_thread(NULL);

		int expectedValues[kAreaCount] = {
			0,				// CoW -- the child should see the original value
			pid,			// clone -- the child should see the change
			pid,			// clone -- the child should see the change
			area3Value		// CoW -- the child should see the original value
							// Note: It looks alright in BeOS in the first run,
							// but the parent actually seems to modify some
							// cached page, and in the next run, we'll see
							// the changed value.
		};

		// get the IDs of the copied areas
		area_id parentAreas[kAreaCount];
		for (int i = 0; i < kAreaCount; i++) {
			parentAreas[i] = areas[i];
			areas[i] = area_for(areaAddresses[i]);
		}

		for (int i = 0; i < kAreaCount; i++) {
			printf("child: areas[%d]: %ld, %p\n", i, areas[i],
				areaAddresses[i]);
		}

		// clone area 1
		delete_area(areas[1]);
		areas[1] = clone_test_area("child:area1", &areaAddresses[1],
			B_READ_AREA, parentAreas[1]);

		// clone area 2
		delete_area(areas[2]);
		areas[2] = clone_test_area("child:area2", &areaAddresses[2],
			B_READ_AREA, parentAreas[2]);

		snooze(400000);

		for (int i = 0; i < kAreaCount; i++) {
			printf("child: area[%d] contains: 0x%08x (expected: 0x%08x)\n", i,
				*areaAddresses[i], expectedValues[i]);
		}

	} else {
		// parent

		snooze(200000);

		for (int i = 0; i < kAreaCount; i++) {
			status_t error = set_area_protection(areas[i],
				B_READ_AREA | B_WRITE_AREA);
			if (error == B_OK) {
				*areaAddresses[i] = pid;
			} else {
				fprintf(stderr, "parent: Error: set_area_protection(areas[%d]) "
					"failed: %s\n", i, strerror(error));
			}
		}

		status_t result;
		wait_for_thread(pid, &result);
	}

	return 0;
}

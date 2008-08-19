#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <OS.h>


static const int kAreaPagesCount = 4 * 1024;	// 16 MB


int
main()
{
	while (true) {
		uint8* address;
		area_id area = create_area("test area", (void**)&address, B_ANY_ADDRESS,
			kAreaPagesCount * B_PAGE_SIZE, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
		if (area < 0) {
			fprintf(stderr, "Creating the area failed: %s", strerror(area));
			exit(1);
		}

		// touch half of the pages
		for (int i = 0; i < kAreaPagesCount / 2; i++)
			address[i * B_PAGE_SIZE] = 42;

		// fork
		pid_t child = fork();
		if (child < 0) {
			perror("fork() failed");
			exit(1);
		}

		if (child == 0) {
			// child

			// delete the copied area
			delete_area(area_for(address));

			exit(0);
		}

		// parent

		// touch the other half of the pages
		for (int i = kAreaPagesCount / 2; i < kAreaPagesCount; i++)
			address[i * B_PAGE_SIZE] = 42;

		int status;
		while (wait(&status) != child);

		delete_area(area);
	}

	return 0;
}

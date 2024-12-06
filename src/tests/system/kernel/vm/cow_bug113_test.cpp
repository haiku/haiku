#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <OS.h>

const char* kInitialValue = "/dev/null";
const char* kChangedValue = "Argh!";

int
main(int argc, const char* const* argv)
{
	thread_id parent = find_thread(NULL);

	char* globalVar = NULL;
	area_id area = create_area("cow test", (void**)&globalVar,
		B_ANY_ADDRESS, B_PAGE_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		printf("failed to create area\n");
		return 1;
	}

	strcpy(globalVar, kInitialValue);

	printf("[%ld] parent: before fork(): globalVar(%p): \"%s\"\n", parent,
		globalVar, globalVar);

	pid_t child = fork();
	if (child == 0) {
		// child
		child = find_thread(NULL);

		// let the kernel read access the page
		struct stat st;
		stat(globalVar, &st);

		printf("[%ld] child: after kernel read: globalVar: \"%s\"\n",
			child, globalVar);

		// write access the page from userland
		strcpy(globalVar, kChangedValue);

		printf("[%ld] child: after change: globalVar: \"%s\"\n", child,
			globalVar);

	} else {
		// parent

		// wait for the child
		status_t exitVal;
		while (wait_for_thread(child, &exitVal) == B_INTERRUPTED);

		// check the value
		printf("[%ld] parent: after exit child: globalVar: \"%s\"\n",
			parent, globalVar);

		if (strcmp(globalVar, kInitialValue) == 0)
			printf("test OK\n");
		else
			printf("test FAILED: child process changed parent's memory!\n");
	}

	return 0;
}

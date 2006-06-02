#include <Node.h>

#include <stdio.h>
#include <string.h>


int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: lock_node <file>\n");
		return 1;
	}

	BNode node;
	status_t status = node.SetTo(argv[1]);
	if (status != B_OK) {
		fprintf(stderr, "lock_node: could not open \"%s\": %s\n", argv[1], strerror(status));
		return 1;
	}

	status = node.Lock();
	if (status != B_OK) {
		fprintf(stderr, "lock_node: could not lock \"%s\": %s\n", argv[1], strerror(status));
		return 1;
	}

	puts("press <enter> to continue...");
	char a[5];
	fgets(a, 5, stdin);

	node.Unlock();
	node.Unset();
	return 0;
}


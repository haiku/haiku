// ufs_mount.cpp

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <String.h>

const char* kUsage =
"Usage: ufs_mount <file system> <device> <mount point> [ <parameters> ]\n"
;

// print_usage
void
print_usage(bool error = true)
{
	fprintf((error ? stderr : stdout), kUsage);
}

// main
int
main(int argc, char** argv)
{
	// check and get the parameters
	if (argc < 4 || argc > 5) {
		print_usage();
		return 1;
	}
	const char* fileSystem = argv[1];
	const char* device = argv[2];
	const char* mountPoint = argv[3];
	const char* fsParameters = (argc >= 5 ? argv[4] : NULL);
	// get prepare the parameters for the mount() call
	if (strlen(device) == 0)
		device = NULL;
	BString parameters(fileSystem);
	if (fsParameters)
		parameters << ' ' << fsParameters;
	// mount
	ulong flags = 0;
printf("mount('userlandfs', '%s', '%s', %lu, '%s', %ld)\n", mountPoint, device,
flags, parameters.String(), parameters.Length() + 1);
	if (mount("userlandfs", mountPoint, device, flags,
		(void*)parameters.String(), parameters.Length() + 1) < 0) {
		fprintf(stderr, "mounting failed: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

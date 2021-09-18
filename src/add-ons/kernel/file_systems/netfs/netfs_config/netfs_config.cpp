// netfs_config.cpp

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SupportDefs.h>

#include "netfs_ioctl.h"

// usage
static const char* kUsage =
"Usage: netfs_config -h | --help\n"
"       netfs_config <mount point> -a <server name>\n"
"       netfs_config <mount point> -r <server name>\n"
"options:\n"
"  -a                - adds the supplied server\n"
"  -h, --help        - print this text\n"
"  -r                - removes the supplied server\n"
;

// print_usage
static
void
print_usage(bool error)
{
	fputs(kUsage, (error ? stderr : stdout));
}

// add_server
static
status_t
add_server(int fd, const char* serverName)
{
	netfs_ioctl_add_server params;
	if (strlen(serverName) >= sizeof(params.serverName))
		return B_BAD_VALUE;
	strcpy(params.serverName, serverName);
	if (ioctl(fd, NET_FS_IOCTL_ADD_SERVER, &params) < 0)
		return errno;
	return B_OK;
}

// remove_server
static
status_t
remove_server(int fd, const char* serverName)
{
	netfs_ioctl_remove_server params;
	if (strlen(serverName) >= sizeof(params.serverName))
		return B_BAD_VALUE;
	strcpy(params.serverName, serverName);
	if (ioctl(fd, NET_FS_IOCTL_REMOVE_SERVER, &params) < 0)
		return errno;
	return B_OK;
}

// main
int
main(int argc, char** argv)
{
	// parse the arguments
	if (argc < 2) {
		print_usage(true);
		return 1;
	}
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		print_usage(false);
		return 0;
	}
	// add or remove
	if (argc != 4) {
		print_usage(true);
		return 1;
	}
	const char* mountPoint = argv[1];
	bool add = false;
	if (strcmp(argv[2], "-a") == 0) {
		add = true;
	} else if (strcmp(argv[2], "-r") == 0) {
		add = false;
	} else {
		print_usage(true);
		return 1;
	}
	const char* serverName = argv[3];
	// open the mount point
	int fd = open(mountPoint, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Opening `%s' failed: %s\n", mountPoint,
			strerror(errno));
		return 1;
	}
	// do the ioctl
	status_t error = B_OK;
	if (add)
		error = add_server(fd, serverName);
	else
		error = remove_server(fd, serverName);
	if (error != B_OK)
		fprintf(stderr, "Operation failed: %s\n", strerror(error));
	// clean up
	close(fd);
	return (error == B_OK ? 0 : 1);
}


// mkvirtualdrive.cpp

#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Path.h>

#include "virtualdrive.h"

extern "C" const char * const*__libc_argv;

const char *kUsage =
"Usage: %s [ --install ] [ --size <size> ] <file>\n"
"       %s --uninstall  <device>\n"
"       %s --halt <device> (dangerous!)\n"
"       %s --info  <device>\n"
"       %s ( --help | -h )\n"
;

// print_usage
static
void
print_usage(bool error = false)
{
	const char *name = "mkvirtualdrive";
	BPath path;
	if (path.SetTo(__libc_argv[0]) == B_OK)
		name = path.Leaf();
	fprintf((error ? stderr : stdout), kUsage, name, name, name, name);
}


// parse_size
static
bool
parse_size(const char *str, off_t *_size)
{
	if (!str || !_size)
		return false;
	int32 len = strlen(str);
	char buffer[22];
	if (len == 0 || len + 1 > (int32)sizeof(buffer))
		return false;
	strcpy(buffer, str);
	// check the last char for one of the suffixes
	off_t suffix = 1;
	if (!isdigit(buffer[len - 1])) {
		switch (buffer[len - 1]) {
			case 'K':
				suffix = 1LL << 10;
				break;
			case 'M':
				suffix = 1LL << 20;
				break;
			case 'G':
				suffix = 1LL << 30;
				break;
			case 'T':
				suffix = 1LL << 40;
				break;
			case 'P':
				suffix = 1LL << 50;
				break;
			case 'E':
				suffix = 1LL << 60;
				break;
			default:
				return false;
		}
		buffer[len - 1] = '\0';
		len--;
		if (len == 0)
			return false;
	}
	// all other chars must be digits
	for (int i = 0; i < len; i++) {
		if (!isdigit(buffer[i]))
			return false;
	}
	off_t size = atoll(buffer);
	*_size = size * suffix;
	// check for overflow
	if (*_size / suffix != size)
		return false;
	return true;
}


// install_file
status_t
install_file(const char *file, off_t size)
{
	// open the control device
	int fd = open(VIRTUAL_DRIVE_CONTROL_DEVICE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open control device: %s\n",
				strerror(errno));
		return errno;
	}

	// set up the info
	virtual_drive_info info;
	info.magic = VIRTUAL_DRIVE_MAGIC;
	info.drive_info_size = sizeof(info);

	strcpy(info.file_name, file);
	if (size >= 0) {
		info.use_geometry = true;
		// fill in the geometry
		// default to 512 bytes block size
		uint32 blockSize = 512;
		// Optimally we have only 1 block per sector and only one head.
		// Since we have only a uint32 for the cylinder count, this won't work
		// for files > 2TB. So, we set the head count to the minimally possible
		// value.
		off_t blocks = size / blockSize;
		uint32 heads = (blocks + ULONG_MAX - 1) / ULONG_MAX;
		if (heads == 0)
			heads = 1;
		info.geometry.bytes_per_sector = blockSize;
	    info.geometry.sectors_per_track = 1;
	    info.geometry.cylinder_count = blocks / heads;
	    info.geometry.head_count = heads;
	    info.geometry.device_type = B_DISK;	// TODO: Add a new constant.
	    info.geometry.removable = false;
	    info.geometry.read_only = false;
	    info.geometry.write_once = false;
	} else {
		info.use_geometry = false;
	}

	// issue the ioctl
	status_t error = B_OK;
	if (ioctl(fd, VIRTUAL_DRIVE_REGISTER_FILE, &info) != 0) {
		error = errno;
		fprintf(stderr, "Failed to install device: %s\n", strerror(error));
	} else {
		printf("File `%s' registered as device `%s'.\n", file, info.device_name);
	}
	// close the control device
	close(fd);
	return error;
}


// uninstall_file
status_t
uninstall_file(const char *device, bool immediately)
{
	// open the device
	int fd = open(device, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device `%s': %s\n",
				device, strerror(errno));
		return errno;
	}

	// issue the ioctl
	status_t error = B_OK;
	if (ioctl(fd, VIRTUAL_DRIVE_UNREGISTER_FILE, immediately) != 0) {
		error = errno;
		fprintf(stderr, "Failed to uninstall device: %s\n", strerror(error));
	}
	// close the control device
	close(fd);
	return error;
}


status_t
info_for_device(const char *device)
{
	// open the device
	int fd = open(device, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device `%s': %s\n",
				device, strerror(errno));
		return errno;
	}

	// set up the info
	virtual_drive_info info;
	info.magic = VIRTUAL_DRIVE_MAGIC;
	info.drive_info_size = sizeof(info);

	// issue the ioctl
	status_t error = B_OK;
	if (ioctl(fd, VIRTUAL_DRIVE_GET_INFO, &info) != 0) {
		error = errno;
		fprintf(stderr, "Failed to get device info: %s\n", strerror(error));
	} else {
		printf("Device `%s' points to file `%s'.\n", info.device_name, info.file_name);
		off_t size = (off_t)info.geometry.bytes_per_sector
				* info.geometry.sectors_per_track
				* info.geometry.cylinder_count
				* info.geometry.head_count;
		printf("\tdisk size is %Ld bytes (%g MB)\n", size, size / (1024.0 * 1024));
		if (info.halted)
			printf("\tdevice is currently halted.\n");
	}
	// close the control device
	close(fd);
	return error;
}


// main
int
main(int argc, const char **argv)
{
	status_t error = B_OK;
	int argIndex = 1;
	enum { INSTALL, UNINSTALL, INFO } mode = INSTALL;
	bool immediately = false;
	off_t size = -1;

	// parse options
	for (; error == B_OK && argIndex < argc
		   && argv[argIndex][0] == '-'; argIndex++) {
		const char *arg = argv[argIndex];
		if (arg[1] == '-') {
			// "--" option
			arg += 2;
			if (!strcmp(arg, "install")) {
				mode = INSTALL;
			} else if (!strcmp(arg, "size")) {
				argIndex++;
				if (argIndex >= argc) {
					fprintf(stderr, "Parameter expected for `--%s'.\n", arg);
					print_usage(1);
					return 1;
				}
				if (!parse_size(argv[argIndex], &size)) {
					fprintf(stderr, "Parameter for `--%s' must be of the form "
							"`<number>[K|M|G|T|P|E]'.\n", arg);
					print_usage(1);
					return 1;
				}
			} else if (!strcmp(arg, "uninstall")) {
				mode = UNINSTALL;
			} else if (!strcmp(arg, "halt")) {
				mode = UNINSTALL;
				immediately = true;
			} else if (!strcmp(arg, "info")) {
				mode = INFO;
			} else if (!strcmp(arg, "help")) {
				print_usage();
				return 0;
			} else {
				fprintf(stderr, "Invalid option `-%s'.\n", arg);
				print_usage(true);
				return 1;
			}
		} else {
			// "-" options
			arg++;
			int32 count = strlen(arg);
			for (int i = 0; error == B_OK && i < count; i++) {
				switch (arg[i]) {
					case 'h':
						print_usage();
						return 0;
					default:
						fprintf(stderr, "Invalid option `-%c'.\n", arg[i]);
						print_usage(true);
						return 1;
				}
			}
		}
	}

	// parse rest (the file name)
	if (argIndex != argc - 1) {
		print_usage(true);
		return 1;
	}
	const char *file = argv[argIndex];

	// do the job
	switch (mode) {
		case INSTALL:
			error = install_file(file, size);
			break;
		case UNINSTALL:
			error = uninstall_file(file, immediately);
			break;
		case INFO:
			error = info_for_device(file);
			break;
	}

	return (error == B_OK ? 0 : 1);
}


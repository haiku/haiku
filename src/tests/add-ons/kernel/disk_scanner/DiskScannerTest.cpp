// DiskScannerTest.cpp
// run that program from the current/ top directory

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <disk_scanner.h>

// include the implementation if the disk_scanner functions
#include "disk_scanner.cpp"

// the test code starts here

// print_session_info
static
void
print_session_info(const char *prefix, const session_info &info)
{
	printf("%soffset:        %lld\n", prefix, info.offset);
	printf("%ssize:          %lld\n", prefix, info.size);
	printf("%sblock size:    %ld\n", prefix, info.logical_block_size);
	printf("%sindex:         %ld\n", prefix, info.index);
	printf("%sflags:         %lx\n", prefix, info.flags);
}

// print_partition_info
static
void
print_partition_info(const char *prefix, const extended_partition_info &info)
{
	printf("%soffset:         %lld\n", prefix, info.info.offset);
	printf("%ssize:           %lld\n", prefix, info.info.size);
	printf("%sblock size:     %ld\n", prefix, info.info.logical_block_size);
	printf("%ssession ID:     %ld\n", prefix, info.info.session);
	printf("%spartition ID:   %ld\n", prefix, info.info.partition);
	printf("%sdevice:         `%s'\n", prefix, info.info.device);
	printf("%sflags:          %lx\n", prefix, info.flags);
	printf("%spartition name: `%s'\n", prefix, info.partition_name);
	printf("%spartition type: `%s'\n", prefix, info.partition_type);
	printf("%sFS short name:  `%s'\n", prefix, info.file_system_short_name);
	printf("%sFS long name:   `%s'\n", prefix, info.file_system_long_name);
	printf("%svolume name:    `%s'\n", prefix, info.volume_name);
	printf("%sFS flags:       0x%lx\n", prefix, info.file_system_flags);
}

// main
int
main(int argc, char **argv)
{
	char buffer[B_FILE_NAME_LENGTH];
	const char *deviceName = "/dev/disk/ide/ata/0/master/0/raw";

	if (argc > 1) {
		bool atapi = false;
		int32 controller = 0;
		bool master = true;
		
		if (argv[1][0] == '/') {
			deviceName = argv[1];
		} else if (strstr(argv[1], "virtual") != NULL) {
			deviceName = "/dev/disk/virtual/0/raw";
		} else {
			if (strstr(argv[1], "cd") != NULL || strstr(argv[1], "atapi") != NULL)
				atapi = true;
			if (strstr(argv[1], "slave") != NULL)
				master = false;
			if (char *digit = strpbrk(argv[1], "0123456789"))
				controller = atol(digit);
				
			sprintf(buffer, "/dev/disk/ide/ata%s/%ld/%s/0/raw", atapi ? "pi" : "",
				controller, master ? "master" : "slave");
			
			deviceName = buffer;
		}
	}

	int device = open(deviceName, 0);
	if (device < 0) {
		fprintf(stderr, "Could not open device \"%s\": %s\n", deviceName, strerror(device));
		return -1;
	}

	printf("device: `%s'\n", deviceName);

	session_info sessionInfo;
	for (int32 i = 0; ; i++) {
		status_t status = get_nth_session_info(device, i, &sessionInfo);
		if (status < B_OK) {
			if (status != B_ENTRY_NOT_FOUND)
				fprintf(stderr, "get_nth_session_info() failed: %s\n", strerror(status));
			break;
		}

		printf("session %ld\n", i);
		print_session_info("  ", sessionInfo);

		for (int32 k = 0; ; k++) {
			extended_partition_info partitionInfo;
			char partitionMapName[B_FILE_NAME_LENGTH];
			status = get_nth_partition_info(device, i, k, &partitionInfo,
											partitionMapName);
			if (status < B_OK) {
				if (status != B_ENTRY_NOT_FOUND)
					fprintf(stderr, "get_nth_partition_info() failed: %s\n", strerror(status));
				break;
			}

			if (k == 0)
				printf("  partition map: `%s'\n", partitionMapName);

			printf("  partition %ld_%ld\n", i, k);
			print_partition_info("    ", partitionInfo);
		}
	}
	close(device);

	return 0;
}


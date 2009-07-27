#include <stdio.h>
#include <string.h>

#include <USBKit.h>


int
main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("usage: %s <device> <configuration index>\n", argv[0]);
		return 1;
	}

	BUSBDevice device(argv[1]);
	if (device.InitCheck() != B_OK) {
		printf("failed to open device %s\n", argv[1]);
		return 2;
	}

	uint32 index;
	if (sscanf(argv[2], "%lu", &index) != 1) {
		printf("could not parse configuration index\n");
		return 3;
	}

	const BUSBConfiguration *config = device.ConfigurationAt(index);
	if (config == NULL) {
		printf("couldn't get configuration at %lu\n", index);
		return 4;
	}

	status_t result = device.SetConfiguration(config);
	if (result != B_OK) {
		printf("failed to set configuration: %s\n", strerror(result));
		return 5;
	}

	printf("configuration %lu set on device %s\n", index, argv[1]);
	return 0;
}

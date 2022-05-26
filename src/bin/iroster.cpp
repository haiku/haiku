/*
 * iroster.cpp
 * (c) 2002, Carlos Hasan, for Haiku.
 * Compile: gcc -Wall -Wno-multichar -O2 -o iroster iroster.cpp -lbe
 */

#include <stdio.h>
#include <string.h>
#include <interface/Input.h>
#include <support/List.h>

static int list_devices()
{
	BList list;
	BInputDevice *device;
	int i, n;
	status_t err;

	printf("         name                  type         state \n");
	printf("--------------------------------------------------\n");

	if ((err = get_input_devices(&list))!=B_OK) {
		fprintf(stderr, "error while get_input_devices: %s\n", strerror(err));
		return -1;
	}

	n = list.CountItems();
	if (n == 0) {
		printf("...no input devices found...\n");
	}

	for (i = 0; i < n; i++) {
		device = (BInputDevice *) list.ItemAt(i);

		printf("%23s %18s %7s\n",
			device->Name(),
			device->Type() == B_POINTING_DEVICE ? "B_POINTING_DEVICE" :
			device->Type() == B_KEYBOARD_DEVICE ? "B_KEYBOARD_DEVICE" : "B_UNDEFINED_DEVICE",
			device->IsRunning() ? "running" : "stopped");
	}

	return 0;
}

static void start_device(const char *name)
{
	BInputDevice *device;
	status_t status;

	device = find_input_device(name);
	if (device == NULL) {
		printf("Error finding device \"%s\"\n", name);
	}
	else if ((status = device->Start()) != B_OK) {
		printf("Error starting device \"%s\" (%" B_PRId32 ")\n", name, status);
	}
	else {
		printf("Started device \"%s\"\n", name);
	}
	if (device != NULL)
		delete device;
}

static void stop_device(const char *name)
{
	BInputDevice *device;
	status_t status;

	device = find_input_device(name);
	if (device == NULL) {
		printf("Error finding device \"%s\"\n", name);
	}
	else if ((status = device->Stop()) != B_OK) {
		printf("Error stopping device \"%s\" (%" B_PRId32 ")\n", name, status);
	}
	else {
		printf("Stopped device \"%s\"\n", name);
	}
	if (device != NULL)
		delete device;
}

int main(int argc, char *argv[])
{
	int i;
	const char *name;

	if (argc <= 1) {
		return list_devices();
	}
	else {
		for (i = 1; i < argc; i++) {
			name = argv[i];
			if (name[0] == '+') {
				start_device(name + 1);
			}
			else if (name[0] == '-') {
				stop_device(name + 1);
			}
			else {
				printf("USAGE: %s [+|-]input_device_name\n", argv[0]);
			}
		}
	}
	return 0;
}


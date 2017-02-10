/* 
** Copyright 2004, Jérôme Duval. All rights reserved.
**
** Distributed under the terms of the MIT License.
*/

#include <Input.h>
#include <List.h>

#include <string.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
	BList list;
	status_t err;
	if ((err = get_input_devices(&list))!=B_OK)
		printf("get_input_devices returned %s\n", strerror(err));
	
	for (uint32 i=0; i<(unsigned)list.CountItems(); i++) {
		BInputDevice *device = (BInputDevice*)list.ItemAt(i);
		if (device == NULL) {
			printf("device %ld is NULL\n", i);
			continue;
		}
		
		printf("device %ld %s ", i, device->Name());
		if (device->Type() == B_POINTING_DEVICE) 
			printf("B_POINTING_DEVICE\n");
		if (device->Type() == B_KEYBOARD_DEVICE)
			printf("B_KEYBOARD_DEVICE\n"); 
		if (device->Type() == B_UNDEFINED_DEVICE)
			printf("B_UNDEFINED_DEVICE\n");
	
		
		device = find_input_device(device->Name());
		if (device == NULL) {
			printf("device %ld with find_input_device is NULL\n", i);
			continue;
		}
		
		printf("device %ld with find_input_device %s ", i, device->Name()); 
		if (device->Type() == B_POINTING_DEVICE) 
			printf("B_POINTING_DEVICE");
		if (device->Type() == B_KEYBOARD_DEVICE)
			printf("B_KEYBOARD_DEVICE"); 
		if (device->Type() == B_UNDEFINED_DEVICE)
			printf("B_UNDEFINED_DEVICE");
		
		printf("\n");
		printf(" %s", device->IsRunning() ? "true" : "false");
		device->Stop();
		printf(" %s", device->IsRunning() ? "true" : "false");
		device->Start();
		printf(" %s", device->IsRunning() ? "true" : "false");
		device->Stop();
		printf(" %s", device->IsRunning() ? "true" : "false");
		device->Start();
		printf(" %s", device->IsRunning() ? "true" : "false");
	
		printf("\n");
		
	}
	
	if (find_input_device("blahha") != NULL ) 
		printf("find_input_device(\"blahha\") not returned NULL\n");
	else
		printf("find_input_device(\"blahha\") returned NULL\n");
	
	printf("\n");
	BInputDevice::Start(B_POINTING_DEVICE);
	for (uint32 i=0; i<(unsigned)list.CountItems(); i++) {
		BInputDevice *device = (BInputDevice*)list.ItemAt(i);
		printf(" %s", device->IsRunning() ? "true" : "false");
	}
	BInputDevice::Stop(B_POINTING_DEVICE);
	for (uint32 i=0; i<(unsigned)list.CountItems(); i++) {
		BInputDevice *device = (BInputDevice*)list.ItemAt(i);
		printf(" %s", device->IsRunning() ? "true" : "false");
	}
	BInputDevice::Start(B_POINTING_DEVICE);
	for (uint32 i=0; i<(unsigned)list.CountItems(); i++) {
		BInputDevice *device = (BInputDevice*)list.ItemAt(i);
		printf(" %s", device->IsRunning() ? "true" : "false");
	}
	BInputDevice::Stop(B_POINTING_DEVICE);
	for (uint32 i=0; i<(unsigned)list.CountItems(); i++) {
		BInputDevice *device = (BInputDevice*)list.ItemAt(i);
		printf(" %s", device->IsRunning() ? "true" : "false");
	}
	BInputDevice::Start(B_POINTING_DEVICE);
	for (uint32 i=0; i<(unsigned)list.CountItems(); i++) {
		BInputDevice *device = (BInputDevice*)list.ItemAt(i);
		printf(" %s", device->IsRunning() ? "true" : "false");
	}
	printf("\n");
	return 0;
}


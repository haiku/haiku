/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include <String.h>

#include "BluetoothServer.h"
#include "HCITransportAccessor.h"
#include "Output.h"

HCITransportAccessor::HCITransportAccessor(BPath* path) : HCIDelegate(path)
{
	status_t status;

	fDescriptor = open (path->Path(), O_RDWR);
	if (fDescriptor > 0) {
		// find out which ID was assigned
		status = ioctl(fDescriptor, GET_HCI_ID, &fIdentifier, 0);
		printf("%s: hid retrieved %lx status=%ld\n", __FUNCTION__,
			fIdentifier, status);
	} else {
		printf("%s: Device driver %s could not be opened %ld\n", __FUNCTION__,
			path->Path(), fIdentifier);
		fIdentifier = B_ERROR;
	}

}


HCITransportAccessor::~HCITransportAccessor()
{
	if (fDescriptor  > 0)
	{
		close(fDescriptor);
		fDescriptor = -1;
		fIdentifier = B_ERROR;
	}
}


status_t
HCITransportAccessor::IssueCommand(raw_command rc, size_t size)
{
	if (Id() < 0 || fDescriptor < 0)
		return B_ERROR;
/*
printf("### Command going: len = %ld\n", size);
for (uint16 index = 0 ; index < size; index++ ) {
	printf("%x:",((uint8*)rc)[index]);
}
printf("### \n");
*/

	return ioctl(fDescriptor, ISSUE_BT_COMMAND, rc, size);
}


status_t
HCITransportAccessor::Launch() {

	uint32 dummy;
	return ioctl(fDescriptor, BT_UP, &dummy, sizeof(uint32));

}

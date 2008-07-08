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


}

status_t
HCITransportAccessor::IssueCommand(raw_command rc, size_t size)
{
	if (GetID() < 0 || fFD < 0)
		return B_ERROR;

printf("### Command going: len = %ld\n", size);
for (uint16 index = 0 ; index < size; index++ ) {
	printf("%x:",((uint8*)rc)[index]);
}
printf("### \n");


	return ioctl(fFD, ISSUE_BT_COMMAND, rc, size);
}


status_t 
HCITransportAccessor::Launch() {

	return B_OK;

}

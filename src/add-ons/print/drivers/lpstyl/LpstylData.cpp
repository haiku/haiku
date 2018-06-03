/*
* Copyright 2017, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/


#include "LpstylData.h"

#include <termios.h>

#include <Node.h>
#include <SupportKit.h>


void
LpstylData::Save()
{
	PrinterData::Save();
	// force baudrate for the serial transport
	int32 rate = B57600;
	fNode->WriteAttr("transport_baudrate", B_INT32_TYPE, 0, &rate, sizeof(int32));
}

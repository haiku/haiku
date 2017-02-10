/* Volume - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "Volume.h"

#include <stdio.h>


void 
Volume::Panic()
{
	printf("PANIC!\n");
}


int32
Volume::GenerateTransactionID()
{
	static int32 sTransactionID = 1;
	return sTransactionID++;
}

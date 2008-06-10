/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Rudolf Cornelissen 9/2003-6/2008.
*/

/*
	note:
	moved DMA acceleration 'top-level' routines to be integrated in the engine:
	it is costly to call the engine for every single function within a loop!
	(measured with BeRoMeter 1.2.6: upto 15% speed increase on all CPU's.)
*/

#define MODULE_BIT 0x40000000

#include "acc_std.h"

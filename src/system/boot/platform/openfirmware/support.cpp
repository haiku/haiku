/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "support.h"
#include <platform/openfirmware/openfirmware.h>


bigtime_t
system_time(void)
{
	int result = of_milliseconds();
	return (result == OF_FAILED ? 0 : bigtime_t(result) * 1000);
}


/** given the package provided, get the number of cells
+   in the reg property
+ */

int32
of_address_cells(intptr_t package) {
	uint32 address_cells;
	if (of_getprop(package, "#address-cells",
		&address_cells, sizeof(address_cells)) == OF_FAILED)
		return OF_FAILED;

	return address_cells;
}


int32
of_size_cells(intptr_t package) {
	uint32 size_cells;
	if (of_getprop(package, "#size-cells",
		&size_cells, sizeof(size_cells)) == OF_FAILED)
		return OF_FAILED;

	return size_cells;
}



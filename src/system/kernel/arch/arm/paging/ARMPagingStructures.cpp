/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/ARMPagingStructures.h"


ARMPagingStructures::ARMPagingStructures()
	:
	ref_count(1),
	active_on_cpus(0)
{
}


ARMPagingStructures::~ARMPagingStructures()
{
}

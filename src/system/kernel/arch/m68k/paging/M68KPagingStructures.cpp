/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/M68KPagingStructures.h"


M68KPagingStructures::M68KPagingStructures()
	:
	ref_count(1),
	active_on_cpus(0)
{
}


M68KPagingStructures::~M68KPagingStructures()
{
}

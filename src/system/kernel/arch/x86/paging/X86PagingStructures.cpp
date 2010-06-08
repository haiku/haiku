/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/X86PagingStructures.h"


X86PagingStructures::X86PagingStructures()
	:
	ref_count(1),
	active_on_cpus(0)
{
}


X86PagingStructures::~X86PagingStructures()
{
}

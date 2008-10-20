/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "x86_physical_page_mapper.h"


PhysicalPageMapper* gPhysicalPageMapper;
TranslationMapPhysicalPageMapper* gKernelPhysicalPageMapper;


TranslationMapPhysicalPageMapper::~TranslationMapPhysicalPageMapper()
{
}


PhysicalPageMapper::~PhysicalPageMapper()
{
}

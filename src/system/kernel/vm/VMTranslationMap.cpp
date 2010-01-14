/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <vm/VMTranslationMap.h>


// #pragma mark - VMTranslationMap


VMTranslationMap::VMTranslationMap()
	:
	fMapCount(0)
{
	recursive_lock_init(&fLock, "translation map");
}


VMTranslationMap::~VMTranslationMap()
{
	recursive_lock_destroy(&fLock);
}


// #pragma mark - VMPhysicalPageMapper


VMPhysicalPageMapper::VMPhysicalPageMapper()
{
}


VMPhysicalPageMapper::~VMPhysicalPageMapper()
{
}

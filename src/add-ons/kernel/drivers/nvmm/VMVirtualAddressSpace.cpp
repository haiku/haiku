/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "VMVirtualAddressSpace.h"


VMVirtualAddressSpace::VMVirtualAddressSpace(VMTranslationMap* map,
	addr_t base, size_t size)
	:
	VMUserAddressSpace(-1, base, size)
{
	fTranslationMap = map;
}

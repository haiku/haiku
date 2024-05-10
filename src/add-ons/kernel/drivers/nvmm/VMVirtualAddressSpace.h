/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VM_VIRTUAL_ADDRESS_SPACE_H
#define VM_VIRTUAL_ADDRESS_SPACE_H


#include <vm/VMUserAddressSpace.h>


struct VMVirtualAddressSpace final : VMUserAddressSpace {
public:
								VMVirtualAddressSpace(VMTranslationMap* map,
									addr_t base, size_t size);
};


#endif	/* VM_VIRTUAL_ADDRESS_SPACE_H */

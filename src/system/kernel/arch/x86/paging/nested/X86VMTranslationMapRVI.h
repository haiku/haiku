/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_NESTED_X86_VM_TRANSLATION_MAP_RVI_H
#define KERNEL_ARCH_X86_PAGING_NESTED_X86_VM_TRANSLATION_MAP_RVI_H


#include "paging/64bit/X86VMTranslationMap64Bit.h"


struct X86VMTranslationMapRVI final : X86VMTranslationMap64Bit {
								X86VMTranslationMapRVI();
	virtual						~X86VMTranslationMapRVI();

			status_t			Init();

			void				SetFlushCallback(void (**callback)(void*), void* arg);
	virtual	void				Flush() final;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation) override;

	virtual	status_t			Protect(addr_t base, addr_t top,
									uint32 attributes, uint32 memoryType) override;

private:
	void (**fFlushCallback)(void*);
	void* fFlushCallbackArg;
};


#endif	// KERNEL_ARCH_X86_PAGING_NESTED_X86_VM_TRANSLATION_MAP_RVI_H

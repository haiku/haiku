/*
** Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_SPARC_MMU_H
#define _KERNEL_ARCH_SPARC_MMU_H


#include <SupportDefs.h>
#include <string.h>

#include <arch_cpu.h>


struct TsbEntry {
public:
	bool IsValid();
	void SetTo(int64_t tag, void* physicalAddress, uint64 mode);

public:
	uint64_t fTag;
	uint64_t fData;
};


extern void sparc_get_instruction_tsb(TsbEntry **_pageTable, size_t *_size);
extern void sparc_get_data_tsb(TsbEntry **_pageTable, size_t *_size);


#endif	/* _KERNEL_ARCH_SPARC_MMU_H */

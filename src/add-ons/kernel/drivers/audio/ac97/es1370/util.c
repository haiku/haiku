/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <Errors.h>
#include <KernelExport.h>
#include <OS.h>
#include <string.h>

//#define DEBUG 2

#include "debug.h"
#include "util.h"

spinlock slock = B_SPINLOCK_INITIALIZER;

uint32 round_to_pagesize(uint32 size);


cpu_status
lock(void)
{
	cpu_status status = disable_interrupts();
	acquire_spinlock(&slock);
	return status;
}


void
unlock(cpu_status status)
{
	release_spinlock(&slock);
	restore_interrupts(status);
}


uint32
round_to_pagesize(uint32 size)
{
	return (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
}


area_id
alloc_mem(phys_addr_t *phy, void **log, size_t size, const char *name)
{
	physical_entry pe;
	void * logadr;
	area_id areaid;
	status_t rv;

	LOG(("allocating %" B_PRIuSIZE " bytes for %s\n", size, name));

	size = round_to_pagesize(size);
	areaid = create_area(name, &logadr, B_ANY_KERNEL_ADDRESS, size,
		B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
	if (areaid < B_OK) {
		PRINT(("couldn't allocate area %s\n",name));
		return B_ERROR;
	}
	rv = get_memory_map(logadr, size, &pe, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		PRINT(("couldn't map %s\n", name));
		return B_ERROR;
	}
	user_memset(logadr, 0, size);
	if (log)
		*log = logadr;
	if (phy)
		*phy = pe.address;
	LOG(("area = %" B_PRId32 ", size = %" B_PRIuSIZE ", log = %p, phy = %#08"
		B_PRIxPHYSADDR "\n", areaid, size, logadr, pe.address));
	return areaid;
}


/* This is not the most advanced method to map physical memory for io access.
 * Perhaps using B_ANY_KERNEL_ADDRESS instead of B_ANY_KERNEL_BLOCK_ADDRESS
 * makes the whole offset calculation and relocation obsolete. But the code
 * below does work, and I can't test if using B_ANY_KERNEL_ADDRESS also works.
 */
area_id
map_mem(void **log, phys_addr_t phy, size_t size, const char *name)
{
	uint32 offset;
	phys_addr_t phyadr;
	void *mapadr;
	area_id area;

	LOG(("mapping physical address %" B_PRIxPHYSADDR " with %" B_PRIxSIZE
		" bytes for %s\n", phy, size, name));

	offset = (uint32)phy & (B_PAGE_SIZE - 1);
	phyadr = phy - offset;
	size = round_to_pagesize(size + offset);
	area = map_physical_memory(name, phyadr, size, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, &mapadr);
	*log = (uint8 *)mapadr + offset;

	LOG(("physical = %" B_PRIxPHYSADDR ", logical = %p, offset = %" B_PRIu32
		", phyadr = %" B_PRIxPHYSADDR ", mapadr = %p, size = %" B_PRIxSIZE
		", area = %" B_PRId32 "\n", phy, *log, offset, phyadr, mapadr, size,
		area));

	return area;
}

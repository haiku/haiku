/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <KernelExport.h>
#include <Errors.h>
#include <OS.h>
#include <string.h>
#include "util.h"

#define TRACE_UTIL
#ifdef TRACE_UTIL
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif


area_id
map_mem(void **virt, phys_addr_t phy, size_t size, uint32 protection,
	const char *name)
{
	uint32 offset;
	phys_addr_t phyadr;
	void *mapadr;
	area_id area;

	TRACE("mapping physical address %" B_PRIxPHYSADDR " with %ld bytes for %s\n",
		phy, size, name);

	offset = (uint32)phy & (B_PAGE_SIZE - 1);
	phyadr = phy - offset;
	size = ROUNDUP(size + offset, B_PAGE_SIZE);
	area = map_physical_memory(name, phyadr, size,
		B_ANY_KERNEL_BLOCK_ADDRESS, protection, &mapadr);
	if (area < B_OK) {
		TRACE("mapping '%s' failed, error 0x%" B_PRIx32 " (%s)\n", name, area,
			strerror(area));
		return area;
	}

	*virt = (char *)mapadr + offset;

	TRACE("physical = %" B_PRIxPHYSADDR ", virtual = %p, offset = %" B_PRIu32
		", phyadr = %" B_PRIxPHYSADDR ", mapadr = %p, size = %" B_PRIuSIZE
		", area = 0x%08" B_PRIx32 "\n", phy, *virt, offset, phyadr, mapadr,
		size, area);

	return area;
}


area_id
alloc_mem(void **virt, phys_addr_t *phy, size_t size, uint32 protection,
	const char *name)
{
	physical_entry pe;
	void * virtadr;
	area_id areaid;
	status_t rv;

	TRACE("allocating %" B_PRIuSIZE " bytes for %s\n", size, name);

	size = ROUNDUP(size, B_PAGE_SIZE);
	areaid = create_area(name, &virtadr, B_ANY_KERNEL_ADDRESS, size,
		B_32_BIT_CONTIGUOUS, protection);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
	if (areaid < B_OK) {
		TRACE("couldn't allocate area %s\n", name);
		return B_ERROR;
	}
	rv = get_memory_map(virtadr, size, &pe, 1);
	if (rv < B_OK) {
		delete_area(areaid);
		TRACE("couldn't map %s\n", name);
		return B_ERROR;
	}
	if (virt)
		*virt = virtadr;
	if (phy)
		*phy = pe.address;
	TRACE("area = %" B_PRId32 ", size = %" B_PRIuSIZE ", virt = %p, phy = %"
		B_PRIxPHYSADDR "\n", areaid, size, virtadr, pe.address);
	return areaid;
}

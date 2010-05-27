/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <Errors.h>
#include <OS.h>
#include <string.h>

#include "debug.h"
#include "util.h"

area_id
map_mem(void **virt, void *phy, size_t size, uint32 protection, const char *name)
{
	uint32 offset;
	void *phyadr;
	void *mapadr;
	area_id area;

	INIT_DEBUGOUT3("mapping physical address %p with %ld bytes for %s\n", phy, size, name);

	offset = (uint32)phy & (B_PAGE_SIZE - 1);
	phyadr = (char *)phy - offset;
	size = ROUNDUP(size + offset, B_PAGE_SIZE);
	area = map_physical_memory(name, (addr_t)phyadr, size,
		B_ANY_KERNEL_BLOCK_ADDRESS, protection, &mapadr);
	if (area < B_OK) {
		ERROROUT3("mapping '%s' failed, error 0x%lx (%s)\n", name, area, strerror(area));
		return area;
	}

	*virt = (char *)mapadr + offset;

	INIT_DEBUGOUT7("physical = %p, virtual = %p, offset = %ld, phyadr = %p, mapadr = %p, size = %ld, area = 0x%08lx\n",
		phy, *virt, offset, phyadr, mapadr, size, area);

	return area;
}

void *
area_malloc(size_t size)
{
	void *p;
	size = ROUNDUP(size, B_PAGE_SIZE);

	if (create_area("area_malloc", &p, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, 0) < 0)
		return 0;
	return p;
}

void
area_free(void *p)
{
	delete_area(area_for(p));
}

/*
 * Copyright 2009, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include <uimage.h>

#include <KernelExport.h>
#include <ByteOrder.h>


void dump_uimage(struct image_header *image)
{
	uint32 *sizes;
	int i;

	dprintf("uimage @ %p:\n", image);

	if (!image)
		return;
	dprintf("magic: %x\n", B_BENDIAN_TO_HOST_INT32(image->ih_magic));
	dprintf("size: %d\n", B_BENDIAN_TO_HOST_INT32(image->ih_size));
	dprintf("load: %p\n", (void *)B_BENDIAN_TO_HOST_INT32(image->ih_load));
	dprintf("ep: %p\n", (void *)B_BENDIAN_TO_HOST_INT32(image->ih_ep));
	dprintf("os: %d\n", image->ih_os);
	dprintf("arch: %d\n", image->ih_arch);
	dprintf("type: %d\n", image->ih_type);
	dprintf("comp: %d\n", image->ih_comp);
	dprintf("name: '%-32s'\n", image->ih_name);
	if (image->ih_type != IH_TYPE_MULTI)
		return;
	sizes = (uint32 *)(&image[1]);
	for (i = 0; sizes[i]; i++) {
		dprintf("contents[%d] :", i);
		dprintf("%d bytes\n", (int)B_BENDIAN_TO_HOST_INT32(sizes[i]));
	}
}


bool
image_multi_getimg(struct image_header *image, uint32 idx, uint32 *data, uint32 *size)
{
	uint32 *sizes;
	uint32 base;
	int i, count = 0;

	sizes = (uint32 *)(&image[1]);
	base = (uint32)sizes;
	for (i = 0; sizes[i]; i++)
		count++;
	base += (count + 1) * sizeof(uint32);
	for (i = 0; sizes[i] && i < count; i++) {
		if (idx == i) {
			*data = base;
			*size = B_BENDIAN_TO_HOST_INT32(sizes[i]);
			return true;
		}
		base += (B_BENDIAN_TO_HOST_INT32(sizes[i]) + 3) & ~3;
	}
	return false;
}


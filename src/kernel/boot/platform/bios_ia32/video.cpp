/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "video.h"
#include "bios.h"
#include "vesa.h"
#include "mmu.h"
#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/kernel_args.h>

#include <string.h>


static vbe_info_block sInfo;
uint16 sMode;
bool sVesaCompatible;


static void
vga_set_palette(const uint8 *palette, int32 firstIndex, int32 numEntries)
{
	out8(firstIndex, 0x03c8);
	// write VGA palette
	for (int32 i = firstIndex; i < numEntries; i++) {
		// VGA (usually) has only 6 bits per gun
		out8(palette[i * 3 + 0] >> 2, 0x03c9);
		out8(palette[i * 3 + 1] >> 2, 0x03c9);
		out8(palette[i * 3 + 2] >> 2, 0x03c9);
	}
}


//	#pragma mark -
//	VESA functions


static status_t
vesa_get_mode_info(uint16 mode, struct vbe_mode_info *modeInfo)
{
	struct bios_regs regs;
	regs.eax = 0x4f01;
	regs.ecx = mode;
	regs.es = ADDRESS_SEGMENT(modeInfo);
	regs.edi = ADDRESS_OFFSET(modeInfo);
	call_bios(0x10, &regs);

	if ((regs.eax & 0xffff) != 0x4f)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


static status_t
vesa_get_vbe_info_block(vbe_info_block *info)
{
	memset(info, 0, sizeof(vbe_info_block));
	info->signature = VBE2_SIGNATURE;

	struct bios_regs regs;
	regs.eax = 0x4f00;
	regs.es = ADDRESS_SEGMENT(info);
	regs.edi = ADDRESS_OFFSET(info);
	call_bios(0x10, &regs);

	if ((regs.eax & 0xffff) != 0x4f)
		return B_ERROR;

	if (info->signature != VESA_SIGNATURE)
		return B_ERROR;

	dprintf("VESA version = %lx\n", info->version);

	if (info->version.major < 2) {
		dprintf("VESA support too old\n", info->version);
		return B_ERROR;
	}

	info->oem_string = SEGMENTED_TO_LINEAR(info->oem_string);
	info->mode_list = SEGMENTED_TO_LINEAR(info->mode_list);
	dprintf("oem string: %s\n", (const char *)info->oem_string);

	return B_OK;
}


static status_t
vesa_init(vbe_info_block *info, uint16 *_standardMode)
{
	if (vesa_get_vbe_info_block(info) != B_OK)
		return B_ERROR;

	dprintf("mode list:\n");
	int32 i = 0;
	while (true) {
		uint16 mode = ((uint16 *)info->mode_list)[i++];
		if (mode == 0xffff)
			break;

		dprintf("  %lx: ", mode);

		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(mode, &modeInfo) == B_OK) {
			dprintf("%ld x %ld x %ld (a = %ld, mem = %ld, phy = %lx, p = %ld, b = %ld)\n",
				modeInfo.width, modeInfo.height, modeInfo.bits_per_pixel, modeInfo.attributes,
				modeInfo.memory_model, modeInfo.physical_base, modeInfo.num_planes,
				modeInfo.num_banks);

			const uint32 requiredAttributes = MODE_ATTR_AVAILABLE | MODE_ATTR_GRAPHICS_MODE
								| MODE_ATTR_COLOR_MODE | MODE_ATTR_LINEAR_BUFFER;

			if (modeInfo.width >= 800
				&& modeInfo.physical_base != 0
				&& modeInfo.num_planes == 1
				&& (modeInfo.memory_model == MODE_MEMORY_PACKED_PIXEL
					|| modeInfo.memory_model == MODE_MEMORY_DIRECT_COLOR)
				&& (modeInfo.attributes & requiredAttributes) == requiredAttributes
				&& modeInfo.bits_per_pixel == 8) {
				*_standardMode = mode;
				return B_OK;
			}
		} else
			dprintf("(failed)\n");
	}

	// no usable VESA mode found...
	return B_ERROR;
}


#if 0
static status_t
vesa_get_mode(uint16 *_mode)
{
	struct bios_regs regs;
	regs.eax = 0x4f03;
	call_bios(0x10, &regs);

	if ((regs.eax & 0xffff) != 0x4f)
		return B_ERROR;

	*_mode = regs.ebx & 0xffff;
	return B_OK;
}
#endif


static status_t
vesa_set_mode(uint16 mode)
{
	struct bios_regs regs;
	regs.eax = 0x4f02;
	regs.ebx = (mode & SET_MODE_MASK) | SET_MODE_LINEAR_BUFFER;
	call_bios(0x10, &regs);

	if ((regs.eax & 0xffff) != 0x4f)
		return B_ERROR;

#if 0
	// make sure we have 8 bits per color channel
	regs.eax = 0x4f08;
	regs.ebx = 8 << 8;
	call_bios(0x10, &regs);
#endif

	return B_OK;
}


static status_t
vesa_set_palette(const uint8 *palette, int32 firstIndex, int32 numEntries)
{
#if 0
	// is this an 8 bit indexed color mode?
	if (gKernelArgs.fb.bit_depth = modeInfo.bits_per_pixel != 8)
		return B_BAD_TYPE;
#endif

	struct bios_regs regs;
	regs.eax = 0x4f09;
	regs.ebx = 0;
	regs.ecx = numEntries;
	regs.edx = firstIndex;
	regs.es = (addr_t)palette >> 4;
	regs.edi = (addr_t)palette & 0xf;
	call_bios(0x10, &regs);

	if ((regs.eax & 0xffff) != 0x4f) {
		// the VESA call does not work, just try good old VGA mechanism
		vga_set_palette(palette, firstIndex, numEntries);
		return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	if (!sVesaCompatible)
		// no logo for now...
		return;

	if (vesa_set_mode(sMode) != B_OK)
		return;

	gKernelArgs.fb.enabled = 1;

	if (!gKernelArgs.fb.already_mapped) {
		// the graphics memory has not been mapped yet!
		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(sMode, &modeInfo) != B_OK) {
			platform_switch_to_text_mode();
			return;
		}

		gKernelArgs.fb.x_size = modeInfo.width;
		gKernelArgs.fb.y_size = modeInfo.height;
		gKernelArgs.fb.bit_depth = modeInfo.bits_per_pixel;
		gKernelArgs.fb.mapping.size = gKernelArgs.fb.x_size * gKernelArgs.fb.y_size * (gKernelArgs.fb.bit_depth/8);
		gKernelArgs.fb.mapping.start = mmu_map_physical_memory(modeInfo.physical_base, gKernelArgs.fb.mapping.size, 0x03);
		gKernelArgs.fb.already_mapped = 1;
	}

	if (vesa_set_palette((const uint8 *)kPalette, 0, 256) != B_OK)
		dprintf("set palette failed!\n");

#if 0
	uint8 *bits = (uint8 *)gKernelArgs.fb.mapping.start;
	uint32 bytesPerRow = gKernelArgs.fb.x_size;
	for (int32 y = 10; y < 30; y++) {
		for (int32 i = 0; i < 256; i++) {
			bits[y * bytesPerRow + i * 3] = i;
			bits[y * bytesPerRow + i * 3 + 1] = i;
			bits[y * bytesPerRow + i * 3 + 2] = i;
		}
	}
#endif

	// ToDo: this is a temporary hack!
	addr_t start = gKernelArgs.fb.mapping.start + gKernelArgs.fb.x_size * (gKernelArgs.fb.y_size - kHeight - 60) * (gKernelArgs.fb.bit_depth/8) + gKernelArgs.fb.x_size - kWidth - 40;
	for (int32 i = 0; i < kHeight; i++) {
		memcpy((void *)(start + gKernelArgs.fb.x_size * i), &kImageData[i * kWidth], kWidth);
	}
}


extern "C" void
platform_switch_to_text_mode(void)
{
	if (!gKernelArgs.fb.enabled)
		return;

	bios_regs regs;
	regs.eax = 3;
	call_bios(0x10, &regs);
	gKernelArgs.fb.enabled = 0;
}


//	#pragma mark -


extern "C" status_t
video_init(void)
{
	gKernelArgs.fb.enabled = 0;

	sVesaCompatible = vesa_init(&sInfo, &sMode) == B_OK;
	if (!sVesaCompatible) {
		dprintf("No VESA compatible graphics!\n");
		return B_ERROR;
	}

	dprintf("VESA compatible graphics!\n");
	return B_OK;
}


/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "video.h"
#include "bios.h"
#include "vesa.h"
#include "mmu.h"
#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <util/list.h>

#include <stdio.h>
#include <string.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct video_mode {
	list_link	link;
	uint16		mode;
	int32		width, height, bits_per_pixel;
};

static vbe_info_block sInfo;
video_mode *sMode;
bool sVesaCompatible;
struct list sModeList;
static addr_t sFrameBuffer;


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
vesa_init(vbe_info_block *info, video_mode **_standardMode)
{
	if (vesa_get_vbe_info_block(info) != B_OK)
		return B_ERROR;

	// fill mode list

	video_mode *standardMode = NULL;

	int32 i = 0;
	while (true) {
		uint16 mode = ((uint16 *)info->mode_list)[i++];
		if (mode == 0xffff)
			break;

		TRACE(("  %lx: ", mode));

		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(mode, &modeInfo) == B_OK) {
			TRACE(("%ld x %ld x %ld (a = %ld, mem = %ld, phy = %lx, p = %ld, b = %ld)\n",
				modeInfo.width, modeInfo.height, modeInfo.bits_per_pixel, modeInfo.attributes,
				modeInfo.memory_model, modeInfo.physical_base, modeInfo.num_planes,
				modeInfo.num_banks));

			const uint32 requiredAttributes = MODE_ATTR_AVAILABLE | MODE_ATTR_GRAPHICS_MODE
								| MODE_ATTR_COLOR_MODE | MODE_ATTR_LINEAR_BUFFER;

			if (modeInfo.width >= 640
				&& modeInfo.physical_base != 0
				&& modeInfo.num_planes == 1
				&& (modeInfo.memory_model == MODE_MEMORY_PACKED_PIXEL
					|| modeInfo.memory_model == MODE_MEMORY_DIRECT_COLOR)
				&& (modeInfo.attributes & requiredAttributes) == requiredAttributes) {
				// this mode fits our needs
				video_mode *videoMode = (video_mode *)malloc(sizeof(struct video_mode));
				if (videoMode == NULL)
					continue;

				videoMode->mode = mode;
				videoMode->width = modeInfo.width;
				videoMode->height = modeInfo.height;
				videoMode->bits_per_pixel = modeInfo.bits_per_pixel;

				// ToDo: for now, only accept 8 bit modes as standard
				if (standardMode == NULL && modeInfo.bits_per_pixel == 8)
					standardMode = videoMode;
				else if (standardMode != NULL) {
					// switch to the one with the higher resolution
					// ToDo: is that always a good idea? for now we'll use 800x600
					if (modeInfo.width > standardMode->width && modeInfo.width <= 800)
						standardMode = videoMode;
				}

				list_add_item(&sModeList, videoMode);
			}
		} else
			TRACE(("(failed)\n"));
	}

	if (standardMode == NULL) {
		// no usable VESA mode found...
		return B_ERROR;
	}

	*_standardMode = standardMode;
	return B_OK;
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
#if 0
	struct bios_regs regs;
	regs.eax = 0x4f09;
	regs.ebx = 0;
	regs.ecx = numEntries;
	regs.edx = firstIndex;
	regs.es = (addr_t)palette >> 4;
	regs.edi = (addr_t)palette & 0xf;
	call_bios(0x10, &regs);

	if ((regs.eax & 0xffff) != 0x4f) {
#endif
		// the VESA call does not work, just try good old VGA mechanism
		vga_set_palette(palette, firstIndex, numEntries);
#if 0
		return B_ERROR;
	}
#endif
	return B_OK;
}


//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
	// find selected mode
	video_mode *mode = NULL;

	menu = item->Submenu();
	item = menu->FindMarked();
	if (item != NULL)
		mode = (video_mode *)item->Data();

	if (mode != sMode) {
		// update standard mode
		// ToDo: update fb settings!
		sMode = mode;
	}

	return true;
}


Menu *
video_mode_menu()
{
	Menu *menu = new Menu(CHOICE_MENU, "Select Video Mode");
	MenuItem *item;

	menu->AddItem(item = new MenuItem("Default"));
	item->SetMarked(true);
	item->Select(true);
	item->SetHelpText("The Default video mode is the one currently configured in "
		"the system. If there is no mode configured yet, a viable mode will be chosen "
		"automatically.");

	menu->AddItem(new MenuItem("Standard VGA"));

	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode)) != NULL) {
		char label[64];
		sprintf(label, "%ldx%ld %ld bit", mode->width, mode->height, mode->bits_per_pixel);

		menu->AddItem(item = new MenuItem(label));
		item->SetData(mode);

		// ToDo: remove this!
		if (mode->bits_per_pixel != 8)
			item->SetHelpText("The boot logo will currently only be rendered correctly in 8 bit modes.");
	}

	menu->AddSeparatorItem();
	menu->AddItem(item = new MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	if (!sVesaCompatible || sMode == NULL)
		// no logo for now...
		return;

	if (vesa_set_mode(sMode->mode) != B_OK)
		return;

	gKernelArgs.frame_buffer.enabled = 1;

	struct vbe_mode_info modeInfo;
	if (vesa_get_mode_info(sMode->mode, &modeInfo) != B_OK) {
		platform_switch_to_text_mode();
		return;
	}

	addr_t lastBase = gKernelArgs.frame_buffer.physical_buffer.start;

	gKernelArgs.frame_buffer.width = modeInfo.width;
	gKernelArgs.frame_buffer.height = modeInfo.height;
	gKernelArgs.frame_buffer.depth = modeInfo.bits_per_pixel;
	gKernelArgs.frame_buffer.physical_buffer.size = gKernelArgs.frame_buffer.width
		* gKernelArgs.frame_buffer.height * (gKernelArgs.frame_buffer.depth / 8);
	gKernelArgs.frame_buffer.physical_buffer.start = modeInfo.physical_base;

	// ToDo: we assume that physical base is constant through the different resolutions
	// ToDo: this is broken - if the newly chosen video mode is larger than the old one
	//	the boot loader will crash while trying to access unmapped memory
	if (lastBase == 0) {
		// the graphics memory has not been mapped yet!
		sFrameBuffer = mmu_map_physical_memory(modeInfo.physical_base,
							gKernelArgs.frame_buffer.physical_buffer.size, 0x03);
	}

	// clear the video memory
	// ToDo: this shouldn't be necessary on real hardware (and Bochs), but
	//	at least booting with Qemu looks ugly when this is missing
	memset((void *)sFrameBuffer, 0, gKernelArgs.frame_buffer.physical_buffer.size);

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
	addr_t start = sFrameBuffer + gKernelArgs.frame_buffer.width
		* (gKernelArgs.frame_buffer.height - kHeight - 60)
		* (gKernelArgs.frame_buffer.depth/8) + gKernelArgs.frame_buffer.width - kWidth - 40;
	for (int32 i = 0; i < kHeight; i++) {
		memcpy((void *)(start + gKernelArgs.frame_buffer.width * i),
			&kImageData[i * kWidth], kWidth);
	}
}


extern "C" void
platform_switch_to_text_mode(void)
{
	if (!gKernelArgs.frame_buffer.enabled)
		return;

	bios_regs regs;
	regs.eax = 3;
	call_bios(0x10, &regs);
	gKernelArgs.frame_buffer.enabled = 0;
}


extern "C" status_t 
platform_init_video(void)
{
	gKernelArgs.frame_buffer.enabled = 0;
	list_init(&sModeList);

	sVesaCompatible = vesa_init(&sInfo, &sMode) == B_OK;
	if (!sVesaCompatible) {
		TRACE(("No VESA compatible graphics!\n"));
		return B_ERROR;
	}

	TRACE(("VESA compatible graphics!\n"));

	return B_OK;
}


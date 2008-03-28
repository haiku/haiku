/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "video.h"
#include "bios.h"
#include "vesa.h"
#include "vga.h"
#include "mmu.h"

#include <edid.h>

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/images.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
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
	uint16		width, height, bits_per_pixel;
	uint32		bytes_per_row;
};

static vbe_info_block sInfo;
static video_mode *sMode, *sDefaultMode;
static bool sVesaCompatible;
struct list sModeList;
static addr_t sFrameBuffer;
static bool sModeChosen;
static bool sSettingsLoaded;


static int
compare_video_modes(video_mode *a, video_mode *b)
{
	int compare = a->width - b->width;
	if (compare != 0)
		return compare;

	compare = a->height - b->height;
	if (compare != 0)
		return compare;

	// TODO: compare video_mode::mode?
	return a->bits_per_pixel - b->bits_per_pixel;
}


/*!	Insert the video mode into the list, sorted by resolution and bit depth.
	Higher resolutions/depths come first.
*/
static void
add_video_mode(video_mode *videoMode)
{
	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode))
			!= NULL) {
		int compare = compare_video_modes(videoMode, mode);
		if (compare == 0) {
			// mode already exists
			return;
		}

		if (compare > 0)
			break;
	}

	list_insert_item_before(&sModeList, mode, videoMode);
}


/*! \brief Finds a video mode with the given resolution.
	If \a allowPalette is true, 8-bit modes are considered, too.
	If \a height is \c -1, the height is ignored, and only the width
	matters.
*/
static video_mode *
find_video_mode(int32 width, int32 height, bool allowPalette)
{
	video_mode *found = NULL;
	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode))
			!= NULL) {
		if (mode->width == width && (height == -1 || mode->height == height)
			&& (mode->bits_per_pixel > 8 || allowPalette)) {
			if (found == NULL || found->bits_per_pixel < mode->bits_per_pixel)
				found = mode;
		}
	}

	return found;
}


static video_mode *
find_video_mode(int32 width, int32 height, int32 depth)
{
	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode))
			!= NULL) {
		if (mode->width == width
			&& mode->height == height
			&& mode->bits_per_pixel == depth) {
			return mode;
		}
	}

	return NULL;
}


static video_mode *
find_edid_mode(edid1_info &info, bool allowPalette)
{
	video_mode *mode = NULL;

	// try detailed timing first
	for (int32 i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; i++) {
		edid1_detailed_monitor &monitor = info.detailed_monitor[i];

		if (monitor.monitor_desc_type == EDID1_IS_DETAILED_TIMING) {
			mode = find_video_mode(monitor.data.detailed_timing.h_active,
				monitor.data.detailed_timing.v_active, allowPalette);
			if (mode != NULL)
				return mode;
		}
	}

	// try standard timings next
	for (int32 i = 0; i < EDID1_NUM_STD_TIMING; i++) {
		if (info.std_timing[i].h_size <= 256)
			continue;

		video_mode *found = find_video_mode(info.std_timing[i].h_size,
			info.std_timing[i].v_size, allowPalette);
		if (found != NULL) {
			if (mode != NULL) {
				// prefer higher resolutions
				if (found->width > mode->width)
					mode = found;
			} else
				mode = found;
		}
	}

	return NULL;
}


static bool
get_mode_from_settings(void)
{
	if (sSettingsLoaded)
		return true;

	void *handle = load_driver_settings("vesa");
	if (handle == NULL)
		return false;

	bool found = false;

	const driver_settings *settings = get_driver_settings(handle);
	if (settings == NULL)
		goto out;

	sSettingsLoaded = true;

	for (int32 i = 0; i < settings->parameter_count; i++) {
		driver_parameter &parameter = settings->parameters[i];

		if (!strcmp(parameter.name, "mode") && parameter.value_count > 2) {
			// parameter found, now get its values
			int32 width = strtol(parameter.values[0], NULL, 0);
			int32 height = strtol(parameter.values[1], NULL, 0);
			int32 depth = strtol(parameter.values[2], NULL, 0);

			// search mode that fits

			video_mode *mode = find_video_mode(width, height, depth);
			if (mode != NULL) {
				found = true;
				sMode = mode;
			}
		}
	}

out:
	unload_driver_settings(handle);
	return found;
}


//	#pragma mark - vga


static void
vga_set_palette(const uint8 *palette, int32 firstIndex, int32 numEntries)
{
	out8(firstIndex, VGA_COLOR_WRITE_MODE);
	// write VGA palette
	for (int32 i = firstIndex; i < numEntries; i++) {
		// VGA (usually) has only 6 bits per gun
		out8(palette[i * 3 + 0] >> 2, VGA_COLOR_DATA);
		out8(palette[i * 3 + 1] >> 2, VGA_COLOR_DATA);
		out8(palette[i * 3 + 2] >> 2, VGA_COLOR_DATA);
	}
}


static void
vga_enable_bright_background_colors(void)
{
	// reset attribute controller
	in8(VGA_INPUT_STATUS_1);

	// select mode control register
	out8(0x30, VGA_ATTRIBUTE_WRITE);

	// read mode control register, change it (we need to clear bit 3), and write it back
	uint8 mode = in8(VGA_ATTRIBUTE_READ) & 0xf7;
	out8(mode, VGA_ATTRIBUTE_WRITE);
}


//	#pragma mark - vesa


static status_t
vesa_get_edid(edid1_info *info)
{
	struct bios_regs regs;
	regs.eax = 0x4f15;
	regs.ebx = 0;
		// report DDC service
	regs.ecx = 0;
	regs.es = 0;
	regs.edi = 0;
	call_bios(0x10, &regs);

	dprintf("EDID1: %lx\n", regs.eax);
	// %ah contains the error code
	// %al determines wether or not the function is supported
	if (regs.eax != 0x4f)
		return B_NOT_SUPPORTED;

	dprintf("EDID2: ebx %lx\n", regs.ebx);
	// test if DDC is supported by the monitor
	if ((regs.ebx & 3) == 0)
		return B_NOT_SUPPORTED;

	edid1_raw edidRaw;

	regs.eax = 0x4f15;
	regs.ebx = 1;
		// read EDID
	regs.ecx = 0;
	regs.edx = 0;
	regs.es = ADDRESS_SEGMENT(&edidRaw);
	regs.edi = ADDRESS_OFFSET(&edidRaw);
	call_bios(0x10, &regs);
	dprintf("EDID3: %lx\n", regs.eax);

	if (regs.eax != 0x4f)
		return B_NOT_SUPPORTED;

	// retrieved EDID - now parse it
	dprintf("Got EDID!\n");
	edid_decode(info, &edidRaw);
	edid_dump(info);
	return B_OK;
}


static status_t
vesa_get_mode_info(uint16 mode, struct vbe_mode_info *modeInfo)
{
	memset(modeInfo, 0, sizeof(vbe_mode_info));

	struct bios_regs regs;
	regs.eax = 0x4f01;
	regs.ecx = mode;
	regs.es = ADDRESS_SEGMENT(modeInfo);
	regs.edi = ADDRESS_OFFSET(modeInfo);
	call_bios(0x10, &regs);

	// %ah contains the error code
	if ((regs.eax & 0xff00) != 0)
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

	// %ah contains the error code
	if ((regs.eax & 0xff00) != 0)
		return B_ERROR;

	if (info->signature != VESA_SIGNATURE)
		return B_ERROR;

	dprintf("VESA version = %d.%d\n", info->version.major, info->version.minor);

	if (info->version.major < 2) {
		dprintf("VESA support too old\n");
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

	// fill mode list and find standard video mode

	video_mode *standardMode = NULL;

	for (int32 i = 0; true; i++) {
		uint16 mode = ((uint16 *)info->mode_list)[i];
		if (mode == 0xffff)
			break;

		TRACE(("  %x: ", mode));

		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(mode, &modeInfo) == B_OK) {
			TRACE(("%u x %u x %u (a = %d, mem = %d, phy = %lx, p = %d, b = %d)\n",
				modeInfo.width, modeInfo.height, modeInfo.bits_per_pixel, modeInfo.attributes,
				modeInfo.memory_model, modeInfo.physical_base, modeInfo.num_planes,
				modeInfo.num_banks));

			const uint32 requiredAttributes = MODE_ATTR_AVAILABLE
				| MODE_ATTR_GRAPHICS_MODE | MODE_ATTR_COLOR_MODE
				| MODE_ATTR_LINEAR_BUFFER;

			if (modeInfo.width >= 640
				&& modeInfo.physical_base != 0
				&& modeInfo.num_planes == 1
				&& (modeInfo.memory_model == MODE_MEMORY_PACKED_PIXEL
					|| modeInfo.memory_model == MODE_MEMORY_DIRECT_COLOR)
				&& (modeInfo.attributes & requiredAttributes)
					== requiredAttributes) {
				// this mode fits our needs
				video_mode *videoMode = (video_mode *)malloc(
					sizeof(struct video_mode));
				if (videoMode == NULL)
					continue;

				videoMode->mode = mode;
				videoMode->bytes_per_row = modeInfo.bytes_per_row;
				videoMode->width = modeInfo.width;
				videoMode->height = modeInfo.height;
				videoMode->bits_per_pixel = modeInfo.bits_per_pixel;
				if (modeInfo.bits_per_pixel == 16
					&& modeInfo.red_mask_size + modeInfo.green_mask_size
						+ modeInfo.blue_mask_size == 15) {
					// this is really a 15-bit mode
					videoMode->bits_per_pixel = 15;
				}

				add_video_mode(videoMode);
			}
		} else
			TRACE(("(failed)\n"));
	}

	// Choose default resolution (when no EDID information is available)
	// TODO: eventually enlarge this to 1024x768?
	const uint32 kWidth = 800;
	standardMode = find_video_mode(kWidth, -1, false);
	if (standardMode == NULL)
		standardMode = find_video_mode(kWidth, -1, true);

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
	// is this an 8 bit indexed color mode?
	if (gKernelArgs.frame_buffer.depth != 8)
		return B_BAD_TYPE;

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
	if (item != NULL) {
		switch (menu->IndexOf(item)) {
			case 0:
				// "Default" mode special
				sMode = sDefaultMode;
				sModeChosen = false;
				return true;
			case 1:
				// "Standard VGA" mode special
				// sets sMode to NULL which triggers VGA mode
				break;
			default:
				mode = (video_mode *)item->Data();
				break;
		}
	}

	if (mode != sMode) {
		// update standard mode
		// ToDo: update fb settings!
		sMode = mode;
	}

	sModeChosen = true;
	return true;
}


Menu *
video_mode_menu()
{
	Menu *menu = new(nothrow) Menu(CHOICE_MENU, "Select Video Mode");
	MenuItem *item;

	menu->AddItem(item = new(nothrow) MenuItem("Default"));
	item->SetMarked(true);
	item->Select(true);
	item->SetHelpText("The Default video mode is the one currently configured "
		"in the system. If there is no mode configured yet, a viable mode will "
		"be chosen automatically.");

	menu->AddItem(new(nothrow) MenuItem("Standard VGA"));

	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode)) != NULL) {
		char label[64];
		sprintf(label, "%ux%u %u bit", mode->width, mode->height,
			mode->bits_per_pixel);

		menu->AddItem(item = new(nothrow) MenuItem(label));
		item->SetData(mode);
	}

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


static void
set_vga_mode(void)
{
	// sets 640x480 16 colors graphics mode
	bios_regs regs;
	regs.eax = 0x12;
	call_bios(0x10, &regs);
}


static void
set_text_mode(void)
{
	// sets 80x25 text console
	bios_regs regs;
	regs.eax = 3;
	call_bios(0x10, &regs);
}


//	#pragma mark - blit


static void
blit32(const uint8 *data, uint16 width, uint16 height, uint16 left, uint16 top)
{
	uint32 *start = (uint32 *)(sFrameBuffer
		+ gKernelArgs.frame_buffer.bytes_per_row * top + 4 * left);

	for (int32 y = 0; y < height; y++) {
		const uint8* src = data;
		uint32* dst = start;
		for (int32 x = 0; x < width; x++) {
			dst[0] = (src[2] << 16) | (src[1] << 8) | (src[0]);
			dst++;
			src += 3;
		}

		data += width * 3;
		start = (uint32 *)((addr_t)start
			+ gKernelArgs.frame_buffer.bytes_per_row);
	}
}


static void
blit24(const uint8 *data, uint16 width, uint16 height, uint16 left, uint16 top)
{
	uint8 *start = (uint8 *)sFrameBuffer
		+ gKernelArgs.frame_buffer.bytes_per_row * top + 3 * left;

	for (int32 y = 0; y < height; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = 0; x < width; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3;
			src += 3;
		}

		data += width * 3;
		start = start + gKernelArgs.frame_buffer.bytes_per_row;
	}
}


static void
blit16(const uint8 *data, uint16 width, uint16 height, uint16 left, uint16 top)
{
	uint16 *start = (uint16 *)(sFrameBuffer
		+ gKernelArgs.frame_buffer.bytes_per_row * top + 2 * left);

	for (int32 y = 0; y < height; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = 0; x < width; x++) {
			dst[0] = ((src[2] >> 3) << 11)
				| ((src[1] >> 2) << 5)
				| ((src[0] >> 3));
			dst++;
			src += 3;
		}

		data += width * 3;
		start = (uint16 *)((addr_t)start
			+ gKernelArgs.frame_buffer.bytes_per_row);
	}
}


static void
blit15(const uint8 *data, uint16 width, uint16 height, uint16 left, uint16 top)
{
	uint16 *start = (uint16 *)(sFrameBuffer
		+ gKernelArgs.frame_buffer.bytes_per_row * top + 2 * left);

	for (int32 y = 0; y < height; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = 0; x < width; x++) {
			dst[0] = ((src[2] >> 3) << 10)
				| ((src[1] >> 3) << 5)
				| ((src[0] >> 3));
			dst++;
			src += 3;
		}

		data += width * 3;
		start = (uint16 *)((addr_t)start
			+ gKernelArgs.frame_buffer.bytes_per_row);
	}
}


static void
blit8(const uint8 *data, uint16 width, uint16 height,
	const uint8 *palette, uint16 left, uint16 top)
{
	if (!data || !palette)
		return;

	if (vesa_set_palette((const uint8 *)palette, 0, 256) != B_OK)
		dprintf("set palette failed!\n");

	addr_t start = sFrameBuffer + gKernelArgs.frame_buffer.bytes_per_row * top
		+ left;

	for (int32 i = 0; i < height; i++) {
		memcpy((void *)(start + gKernelArgs.frame_buffer.bytes_per_row * i),
			&data[i * width], width);
	}
}


static void
blit4(const uint8 *data, uint16 width, uint16 height,
	const uint8 *palette, uint16 left, uint16 top)
{
	if (!data || !palette)
		return;
	//	vga_set_palette((const uint8 *)kPalette16, 0, 16);
	// ToDo: no boot logo yet in VGA mode
#if 1
// this draws 16 big rectangles in all the available colors
	uint8 *bits = (uint8 *)sFrameBuffer;
	uint32 bytesPerRow = 80;
	for (int32 i = 0; i < 32; i++) {
		bits[9 * bytesPerRow + i + 2] = 0x55;
		bits[30 * bytesPerRow + i + 2] = 0xaa;
	}

	for (int32 y = 10; y < 30; y++) {
		for (int32 i = 0; i < 16; i++) {
			out16((15 << 8) | 0x02, VGA_SEQUENCER_INDEX);
			bits[32 * bytesPerRow + i*2 + 2] = i;

			if (i & 1) {
				out16((1 << 8) | 0x02, VGA_SEQUENCER_INDEX);
				bits[y * bytesPerRow + i*2 + 2] = 0xff;
				bits[y * bytesPerRow + i*2 + 3] = 0xff;
			}
			if (i & 2) {
				out16((2 << 8) | 0x02, VGA_SEQUENCER_INDEX);
				bits[y * bytesPerRow + i*2 + 2] = 0xff;
				bits[y * bytesPerRow + i*2 + 3] = 0xff;
			}
			if (i & 4) {
				out16((4 << 8) | 0x02, VGA_SEQUENCER_INDEX);
				bits[y * bytesPerRow + i*2 + 2] = 0xff;
				bits[y * bytesPerRow + i*2 + 3] = 0xff;
			}
			if (i & 8) {
				out16((8 << 8) | 0x02, VGA_SEQUENCER_INDEX);
				bits[y * bytesPerRow + i*2 + 2] = 0xff;
				bits[y * bytesPerRow + i*2 + 3] = 0xff;
			}
		}
	}

	// enable all planes again
	out16((15 << 8) | 0x02, VGA_SEQUENCER_INDEX);
#endif
}


static void
blit_image(const uint8 *data, const uint8* indexedData, uint16 width, uint16 height,
	const uint8 *palette, uint16 left, uint16 top)
{
	switch (gKernelArgs.frame_buffer.depth) {
		case 4:
			return blit4(indexedData, width, height, palette, left, top);
		case 8:
			return blit8(indexedData, width, height, palette, left, top);
		case 15:
			return blit15(data, width, height, left, top);
		case 16:
			return blit16(data, width, height, left, top);
		case 24:
			return blit24(data, width, height, left, top);
		case 32:
			return blit32(data, width, height, left, top);
	}
}

//	#pragma mark -

extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	addr_t lastBase = gKernelArgs.frame_buffer.physical_buffer.start;
	size_t lastSize = gKernelArgs.frame_buffer.physical_buffer.size;

	if (sVesaCompatible && sMode != NULL) {
		if (!sModeChosen)
			get_mode_from_settings();

		if (vesa_set_mode(sMode->mode) != B_OK)
			goto fallback;

		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(sMode->mode, &modeInfo) != B_OK)
			goto fallback;

		gKernelArgs.frame_buffer.width = modeInfo.width;
		gKernelArgs.frame_buffer.height = modeInfo.height;
		gKernelArgs.frame_buffer.bytes_per_row = modeInfo.bytes_per_row;
		gKernelArgs.frame_buffer.depth = modeInfo.bits_per_pixel;
		gKernelArgs.frame_buffer.physical_buffer.size = modeInfo.bytes_per_row
			* gKernelArgs.frame_buffer.height;
		gKernelArgs.frame_buffer.physical_buffer.start = modeInfo.physical_base;
	} else {
fallback:
		// use standard VGA mode 640x480x4
		set_vga_mode();

		gKernelArgs.frame_buffer.width = 640;
		gKernelArgs.frame_buffer.height = 480;
		gKernelArgs.frame_buffer.bytes_per_row = 640 / 2;
		gKernelArgs.frame_buffer.depth = 4;
		gKernelArgs.frame_buffer.physical_buffer.size
			= gKernelArgs.frame_buffer.width
			* gKernelArgs.frame_buffer.height / 2;
		gKernelArgs.frame_buffer.physical_buffer.start = 0xa0000;
	}

	dprintf("video mode: %ux%ux%u\n", gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth);

	gKernelArgs.frame_buffer.enabled = true;

	// If the new frame buffer is either larger than the old one or located at
	// a different address, we need to remap it, so we first have to throw
	// away its previous mapping
	if (lastBase != 0
		&& (lastBase != gKernelArgs.frame_buffer.physical_buffer.start
			|| lastSize < gKernelArgs.frame_buffer.physical_buffer.size)) {
		mmu_free((void *)sFrameBuffer, lastSize);
		lastBase = 0;
	}
	if (lastBase == 0) {
		// the graphics memory has not been mapped yet!
		sFrameBuffer = mmu_map_physical_memory(
			gKernelArgs.frame_buffer.physical_buffer.start,
			gKernelArgs.frame_buffer.physical_buffer.size, kDefaultPageFlags);
	}

	// clear the video memory
	memset((void *)sFrameBuffer, 0,
		gKernelArgs.frame_buffer.physical_buffer.size);

	// TODO: support indexed versions of the images!
	// TODO: support compressed RGB image data

	// render splash logo
	int x = gKernelArgs.frame_buffer.width / 2 - kSplashLogoWidth / 2;
	int y = gKernelArgs.frame_buffer.height / 2 - kSplashLogoHeight / 2;
	blit_image(kSplashLogoImage, NULL, kSplashLogoWidth, kSplashLogoHeight,
		NULL, x, y);

	// render initial (grayed out) icons
	x = gKernelArgs.frame_buffer.width / 2 - kSplashIconsWidth / 2;
	y = y + kSplashLogoHeight;
	uint16 iconsHalfHeight = kSplashIconsHeight / 2;
	const uint8* lowerHalfIconImage = kSplashIconsImage
		+ (kSplashIconsWidth * iconsHalfHeight) * 3;
	blit_image(lowerHalfIconImage, NULL, kSplashIconsWidth, iconsHalfHeight,
		NULL, x, y);
}


extern "C" void
platform_switch_to_text_mode(void)
{
	if (!gKernelArgs.frame_buffer.enabled) {
		vga_enable_bright_background_colors();
		return;
	}

	set_text_mode();
	gKernelArgs.frame_buffer.enabled = 0;

	vga_enable_bright_background_colors();
}


extern "C" status_t
platform_init_video(void)
{
	gKernelArgs.frame_buffer.enabled = 0;
	list_init(&sModeList);

	set_text_mode();
		// You may wonder why we do this here:
		// Obviously, some graphics card BIOS implementations don't
		// report all available modes unless you've done this before
		// getting the VESA information.
		// One example of those is the SiS 630 chipset in my laptop.

	sVesaCompatible = vesa_init(&sInfo, &sDefaultMode) == B_OK;
	if (!sVesaCompatible) {
		TRACE(("No VESA compatible graphics!\n"));
		return B_ERROR;
	}

	TRACE(("VESA compatible graphics!\n"));

	edid1_info info;
	if (vesa_get_edid(&info) == B_OK) {
		// we got EDID information from the monitor, try to find a new default
		// mode
		video_mode *defaultMode = find_edid_mode(info, false);
		if (defaultMode == NULL)
			find_edid_mode(info, true);

		if (defaultMode != NULL) {
			// We found a new default mode to use!
			sDefaultMode = defaultMode;
		}
	}

	sMode = sDefaultMode;
	return B_OK;
}


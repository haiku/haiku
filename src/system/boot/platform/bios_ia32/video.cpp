/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2008, Philippe Saint-Pierre <stpere@gmail.com>
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "video.h"
#include "bios.h"
#include "vesa.h"
#include "vesa_info.h"
#include "vga.h"
#include "mmu.h"

#include <edid.h>

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRACE_VIDEO
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
	crtc_info_block* timing;
};

static vbe_info_block sInfo;
static video_mode *sMode, *sDefaultMode;
static bool sVesaCompatible;
static struct list sModeList;
static uint32 sModeCount;
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
	sModeCount++;
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


/*! Returns the VESA mode closest to the one specified, with a width less or
	equal as specified.
	The height as well as the depth may vary in both directions, though.
*/
static video_mode *
closest_video_mode(int32 width, int32 height, int32 depth)
{
	video_mode *bestMode = NULL;
	uint32 bestDiff = 0;

	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode))
			!= NULL) {
		if (mode->width > width) {
			// Only choose modes with a width less or equal than the searched
			// one; or else it might well be that the monitor cannot keep up.
			continue;
		}

		uint32 diff = 2 * abs(mode->width - width) + abs(mode->height - height)
			+ abs(mode->bits_per_pixel - depth);

		if (bestMode == NULL || bestDiff > diff) {
			bestMode = mode;
			bestDiff = diff;
		}
	}

	return bestMode;
}


static crtc_info_block*
get_crtc_info_block(edid1_detailed_timing& timing)
{
	// This feature is only available on chipsets supporting VBE3 and up
	if (sInfo.version.major < 3)
		return NULL;

	// Copy timing structure to set the mode with
	crtc_info_block* crtcInfo = (crtc_info_block*)malloc(
		sizeof(crtc_info_block));
	if (crtcInfo == NULL)
		return NULL;

	memset(crtcInfo, 0, sizeof(crtc_info_block));
	crtcInfo->horizontal_sync_start = timing.h_active + timing.h_sync_off;
	crtcInfo->horizontal_sync_end = crtcInfo->horizontal_sync_start
		+ timing.h_sync_width;
	crtcInfo->horizontal_total = timing.h_active + timing.h_blank;
	crtcInfo->vertical_sync_start = timing.v_active + timing.v_sync_off;
	crtcInfo->vertical_sync_end = crtcInfo->vertical_sync_start
		+ timing.v_sync_width;
	crtcInfo->vertical_total = timing.v_active + timing.v_blank;
	crtcInfo->pixel_clock = timing.pixel_clock * 10000L;
	crtcInfo->refresh_rate = crtcInfo->pixel_clock
		/ (crtcInfo->horizontal_total / 10)
		/ (crtcInfo->vertical_total / 10);

	TRACE(("crtc: h %u/%u/%u, v %u/%u/%u, pixel clock %lu, refresh %u\n",
		crtcInfo->horizontal_sync_start, crtcInfo->horizontal_sync_end,
		crtcInfo->horizontal_total, crtcInfo->vertical_sync_start,
		crtcInfo->vertical_sync_end, crtcInfo->vertical_total,
		crtcInfo->pixel_clock, crtcInfo->refresh_rate));

	crtcInfo->flags = 0;
	if (timing.sync == 3) {
		// TODO: this switches the default sync when sync != 3 (compared to
		// create_display_modes().
		if ((timing.misc & 1) == 0)
			crtcInfo->flags |= CRTC_NEGATIVE_HSYNC;
		if ((timing.misc & 2) == 0)
			crtcInfo->flags |= CRTC_NEGATIVE_VSYNC;
	}
	if (timing.interlaced)
		crtcInfo->flags |= CRTC_INTERLACED;

	return crtcInfo;
}


static crtc_info_block*
get_crtc_info_block(edid1_std_timing& timing)
{
	// TODO: implement me!
	return NULL;
}


static video_mode*
find_edid_mode(edid1_info& info, bool allowPalette)
{
	video_mode *mode = NULL;

	// try detailed timing first
	for (int32 i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; i++) {
		edid1_detailed_monitor& monitor = info.detailed_monitor[i];

		if (monitor.monitor_desc_type == EDID1_IS_DETAILED_TIMING) {
			mode = find_video_mode(monitor.data.detailed_timing.h_active,
				monitor.data.detailed_timing.v_active, allowPalette);
			if (mode != NULL) {
				mode->timing
					= get_crtc_info_block(monitor.data.detailed_timing);
				return mode;
			}
		}
	}

	int32 best = -1;

	// try standard timings next
	for (int32 i = 0; i < EDID1_NUM_STD_TIMING; i++) {
		if (info.std_timing[i].h_size <= 256)
			continue;

		video_mode* found = find_video_mode(info.std_timing[i].h_size,
			info.std_timing[i].v_size, allowPalette);
		if (found != NULL) {
			if (mode != NULL) {
				// prefer higher resolutions
				if (found->width > mode->width) {
					mode = found;
					best = i;
				}
			} else {
				mode = found;
				best = i;
			}
		}
	}

	if (best >= 0)
		mode->timing = get_crtc_info_block(info.std_timing[best]);

	return mode;
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

			video_mode *mode = closest_video_mode(width, height, depth);
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

	TRACE(("EDID1: %lx\n", regs.eax));
	// %ah contains the error code
	// %al determines whether or not the function is supported
	if (regs.eax != 0x4f)
		return B_NOT_SUPPORTED;

	TRACE(("EDID2: ebx %lx\n", regs.ebx));
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
	TRACE(("EDID3: %lx\n", regs.eax));

	if (regs.eax != 0x4f)
		return B_NOT_SUPPORTED;

	// retrieved EDID - now parse it
	edid_decode(info, &edidRaw);

#ifdef TRACE_VIDEO
	edid_dump(info);
#endif
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

	dprintf("VESA version = %d.%d, capabilities %lx\n", info->version.major,
		info->version.minor, info->capabilities);

	if (info->version.major < 2) {
		dprintf("VESA support too old\n");
		return B_ERROR;
	}

	info->oem_string = SEGMENTED_TO_LINEAR(info->oem_string);
	info->mode_list = SEGMENTED_TO_LINEAR(info->mode_list);
	dprintf("OEM string: %s\n", (const char *)info->oem_string);

	return B_OK;
}


static void
vesa_fixups(vbe_info_block *info)
{
	const char *oem_string = (const char *)info->oem_string;

	if (!strcmp(oem_string, "NVIDIA")) {
		dprintf("Disabling nvidia scaling.\n");
		struct bios_regs regs;
		regs.eax = 0x4f14;
		regs.ebx = 0x0102;
		regs.ecx = 1;	// centered unscaled
		call_bios(0x10, &regs);
	}
}


static status_t
vesa_init(vbe_info_block *info, video_mode **_standardMode)
{
	if (vesa_get_vbe_info_block(info) != B_OK)
		return B_ERROR;

	vesa_fixups(info);

	// fill mode list and find standard video mode

	video_mode *standardMode = NULL;

	for (int32 i = 0; true; i++) {
		uint16 mode = ((uint16 *)info->mode_list)[i];
		if (mode == 0xffff)
			break;

		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(mode, &modeInfo) == B_OK) {
			TRACE((" 0x%03x: %u x %u x %u (a = %d, mem = %d, phy = %lx, p = %d, b = %d)\n", mode,
				   modeInfo.width, modeInfo.height, modeInfo.bits_per_pixel, modeInfo.attributes,
				   modeInfo.memory_model, modeInfo.physical_base, modeInfo.num_planes,
				   modeInfo.num_banks));
			TRACE(("	mask: r: %d %d g: %d %d b: %d %d dcmi: %d\n",
				   modeInfo.red_mask_size, modeInfo.red_field_position,
				   modeInfo.green_mask_size, modeInfo.green_field_position,
				   modeInfo.blue_mask_size, modeInfo.blue_field_position,
				   modeInfo.direct_color_mode_info));

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
				videoMode->timing = NULL;

				if (modeInfo.bits_per_pixel == 16
					&& modeInfo.red_mask_size + modeInfo.green_mask_size
						+ modeInfo.blue_mask_size == 15) {
					// this is really a 15-bit mode
					videoMode->bits_per_pixel = 15;
				}

				add_video_mode(videoMode);
			}
		} else
			TRACE((" 0x%03x: (failed)\n", mode));
	}

	// Choose default resolution (when no EDID information is available)
	const uint32 kPreferredWidth = 1024;
	const uint32 kFallbackWidth = 800;

	standardMode = find_video_mode(kPreferredWidth, -1, false);
	if (standardMode == NULL) {
		standardMode = find_video_mode(kFallbackWidth, -1, false);
		if (standardMode == NULL) {
			standardMode = find_video_mode(kPreferredWidth, -1, true);
			if (standardMode == NULL)
				standardMode = find_video_mode(kFallbackWidth, -1, true);
		}
	}
	if (standardMode == NULL) {
		// just take any mode
		standardMode = (video_mode *)list_get_first_item(&sModeList);
	}

	if (standardMode == NULL) {
		// no usable VESA mode found...
		return B_ERROR;
	}

	TRACE(("Using mode 0x%03x\n", standardMode->mode));
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
vesa_set_mode(video_mode* mode, bool useTiming)
{
	struct bios_regs regs;
	regs.eax = 0x4f02;
	regs.ebx = (mode->mode & SET_MODE_MASK) | SET_MODE_LINEAR_BUFFER;

	if (useTiming && mode->timing != NULL) {
		regs.ebx |= SET_MODE_SPECIFY_CRTC;
		regs.es = ADDRESS_SEGMENT(mode->timing);
		regs.edi = ADDRESS_OFFSET(mode->timing);
	}

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
		snprintf(label, sizeof(label), "%ux%u %u bit", mode->width,
			mode->height, mode->bits_per_pixel);

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
	regs.eax = 0x0012;
	call_bios(0x10, &regs);
}


static void
set_text_mode(void)
{
	// sets 80x25 text console
	bios_regs regs;
	regs.eax = 0x0003;
	call_bios(0x10, &regs);

	video_hide_text_cursor();
}


void
video_move_text_cursor(int x, int y)
{
	bios_regs regs;
	regs.eax = 0x0200;
	regs.ebx = 0;
	regs.edx = (y << 8) | x;
	call_bios(0x10, &regs);
}


void
video_show_text_cursor(void)
{
	bios_regs regs;
	regs.eax = 0x0100;
	regs.ecx = 0x0607;
	call_bios(0x10, &regs);
}


void
video_hide_text_cursor(void)
{
	bios_regs regs;
	regs.eax = 0x0100;
	regs.ecx = 0x2000;
	call_bios(0x10, &regs);
}


//	#pragma mark - blit


void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
	if (!data)
		return;
	// ToDo: no boot logo yet in VGA mode
#if 1
// this draws 16 big rectangles in all the available colors
	uint8 *bits = (uint8 *)frameBuffer;
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


extern "C" void
platform_set_palette(const uint8 *palette)
{
	switch (gKernelArgs.frame_buffer.depth) {
		case 4:
			//vga_set_palette((const uint8 *)kPalette16, 0, 16);
			break;
		case 8:
			if (vesa_set_palette((const uint8 *)palette, 0, 256) != B_OK)
				dprintf("set palette failed!\n");

			break;
		default:
			break;
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

		// On some BIOS / chipset / monitor combinations, there seems to be a
		// timing issue between getting the EDID data and setting the video
		// mode. As such we wait here briefly to give everything enough time
		// to settle.
		spin(1000);

		if ((sMode->timing == NULL || vesa_set_mode(sMode, true) != B_OK)
			&& vesa_set_mode(sMode, false) != B_OK)
			goto fallback;

		struct vbe_mode_info modeInfo;
		if (vesa_get_mode_info(sMode->mode, &modeInfo) != B_OK)
			goto fallback;

		gKernelArgs.frame_buffer.width = modeInfo.width;
		gKernelArgs.frame_buffer.height = modeInfo.height;
		gKernelArgs.frame_buffer.bytes_per_row = modeInfo.bytes_per_row;
		gKernelArgs.frame_buffer.depth = modeInfo.bits_per_pixel;
		gKernelArgs.frame_buffer.physical_buffer.size
			= (phys_size_t)modeInfo.bytes_per_row
				* (phys_size_t)gKernelArgs.frame_buffer.height;
		gKernelArgs.frame_buffer.physical_buffer.start = modeInfo.physical_base;
	} else {
fallback:
		// use standard VGA mode 640x480x4
		set_vga_mode();

		gKernelArgs.frame_buffer.width = 640;
		gKernelArgs.frame_buffer.height = 480;
		gKernelArgs.frame_buffer.bytes_per_row = 80;
		gKernelArgs.frame_buffer.depth = 4;
		gKernelArgs.frame_buffer.physical_buffer.size
			= (phys_size_t)gKernelArgs.frame_buffer.width
				* (phys_size_t)gKernelArgs.frame_buffer.height / 2;
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

	video_display_splash(sFrameBuffer);
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
		gKernelArgs.vesa_capabilities = 0;
		return B_ERROR;
	}

	gKernelArgs.vesa_capabilities = sInfo.capabilities;

	TRACE(("VESA compatible graphics!\n"));

	// store VESA modes into kernel args
	vesa_mode *modes = (vesa_mode *)kernel_args_malloc(
		sModeCount * sizeof(vesa_mode));
	if (modes != NULL) {
		video_mode *mode = NULL;
		uint32 i = 0;
		while ((mode = (video_mode *)list_get_next_item(&sModeList, mode))
				!= NULL) {
			modes[i].mode = mode->mode;
			modes[i].width = mode->width;
			modes[i].height = mode->height;
			modes[i].bits_per_pixel = mode->bits_per_pixel;
			i++;
		}

		gKernelArgs.vesa_modes = modes;
		gKernelArgs.vesa_modes_size = sModeCount * sizeof(vesa_mode);
	}

	edid1_info info;
	// Note, we currently ignore EDID information for VBE2 - while the EDID
	// information itself seems to be reliable, older chips often seem to
	// use very strange default timings with higher modes.
	// TODO: Maybe add a setting to enable it anyway?
	if (sInfo.version.major >= 3 && vesa_get_edid(&info) == B_OK) {
		// we got EDID information from the monitor, try to find a new default
		// mode
		video_mode *defaultMode = find_edid_mode(info, false);
		if (defaultMode == NULL)
			defaultMode = find_edid_mode(info, true);

		if (defaultMode != NULL) {
			// We found a new default mode to use!
			sDefaultMode = defaultMode;
		}

		gKernelArgs.edid_info = kernel_args_malloc(sizeof(edid1_info));
		if (gKernelArgs.edid_info != NULL)
			memcpy(gKernelArgs.edid_info, &info, sizeof(edid1_info));
	}

	sMode = sDefaultMode;
	return B_OK;
}


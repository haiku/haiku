#include <KernelExport.h>
#include <kernel/elf32.h>
#include <kernel.h>
#include <lock.h>
#include <vm.h>
#include <fs/devfs.h>
#include <boot/images.h>
#include <boot/kernel_args.h>
#include <boot/splash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <graphics/vesa/vga.h>

//#define TRACE_BOOT_SPLASH 1
#ifdef TRACE_BOOT_SPLASH
#	define TRACE(x...) dprintf(x);
#else
#	define TRACE(x...) ;
#endif


struct boot_splash_info {
	mutex lock;
	area_id area;
	addr_t frame_buffer;
	bool enabled;
	int32 width;
	int32 height;
	int32 depth;
	int32 bytes_per_pixel;
	int32 bytes_per_row;
};

static struct boot_splash_info sBootSplash;

/*
static void
boot_splash_fb_vga_set_palette(const uint8 *palette, int32 firstIndex,
	int32 numEntries)
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
boot_splash_fb_blit4(const uint8 *data, uint16 width,
	uint16 height, const uint8 *palette, uint16 left, uint16 top)
{
	if (!data || !palette)
		return;
	// ToDo: no blit yet in VGA mode
}


static void
boot_splash_fb_blit8(const uint8 *data, uint16 width,
	uint16 height, const uint8 *palette, uint16 left, uint16 top)
{
	if (!data || !palette)
		return;

	boot_splash_fb_vga_set_palette(palette, 0, 256);

	addr_t start = sBootSplash.frame_buffer + sBootSplash.bytes_per_row * top
		+ left;

	for (int32 i = 0; i < height; i++) {
		memcpy((void *)(start + sBootSplash.bytes_per_row * i),
			&data[i * width], width);
	}
}


static void
boot_splash_fb_blit15(const uint8 *data, uint16 width, uint16 height,
	uint16 left, uint16 top)
{
	uint16* start = (uint16*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * top + 2 * left);

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
		start = (uint16*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit16(const uint8 *data, uint16 width, uint16 height,
	uint16 left, uint16 top)
{
	uint16* start = (uint16*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * top + 2 * left);

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
		start = (uint16*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit24(const uint8 *data, uint16 width, uint16 height,
	uint16 left, uint16 top)
{
	uint8* start = (uint8*)sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * top + 3 * left;

	for (int32 y = 0; y < height; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = 0; x < width; x++) {
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst += 3;
			src += 3;
		}

		data += width * 3;
		start = start + sBootSplash.bytes_per_row;
	}
}


static void
boot_splash_fb_blit32(const uint8 *data, uint16 width, uint16 height,
	uint16 left, uint16 top)
{
	uint32* start = (uint32*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * top + 4 * left);

	for (int32 y = 0; y < height; y++) {
		const uint8* src = data;
		uint32* dst = start;
		for (int32 x = 0; x < width; x++) {
			dst[0] = (src[2] << 16) | (src[1] << 8) | (src[0]);
			dst++;
			src += 3;
		}

		data += width * 3;
		start = (uint32*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit(const uint8 *data, const uint8* indexedData, uint16 width,
	uint16 height, const uint8 *palette, uint16 left, uint16 top)
{
	switch (sBootSplash.depth) {
		case 4:
			boot_splash_fb_blit4(indexedData, width, height, palette,
				left, top);
			return;
		case 8:
			boot_splash_fb_blit8(indexedData, width, height, palette,
				left, top);
			return;
		case 15:
			boot_splash_fb_blit15(data, width, height, left, top);
			return;
		case 16:
			boot_splash_fb_blit16(data, width, height, left, top);
			return;
		case 24:
			boot_splash_fb_blit24(data, width, height, left, top);
			return;
		case 32:
			boot_splash_fb_blit32(data, width, height, left, top);
			return;
	}
}
*/

static void
boot_splash_fb_blit15_cropped(const uint8 *data, uint16 imageLeft,
	uint16 imageTop, uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 imageHeight, const uint8 *palette, uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint16* start = (uint16*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * (top + imageTop)
		+ 2 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = ((src[2] >> 3) << 10)
				| ((src[1] >> 3) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint16*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit16_cropped(const uint8 *data, uint16 imageLeft,
	uint16 imageTop, uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 imageHeight, const uint8 *palette, uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint16* start = (uint16*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * (top + imageTop)
		+ 2 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = ((src[2] >> 3) << 11)
				| ((src[1] >> 2) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint16*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit24_cropped(const uint8 *data, uint16 imageLeft,
	uint16 imageTop, uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 imageHeight, const uint8 *palette, uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint8* start = (uint8*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * (top + imageTop)
		+ 3 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint8*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit32_cropped(const uint8 *data, uint16 imageLeft,
	uint16 imageTop, uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 imageHeight, const uint8 *palette, uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint32* start = (uint32*)(sBootSplash.frame_buffer
		+ sBootSplash.bytes_per_row * (top + imageTop)
		+ 4 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint32* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = (src[0] << 16) | (src[1] << 8) | (src[2]);
			dst++;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint32*)((addr_t)start + sBootSplash.bytes_per_row);
	}
}


static void
boot_splash_fb_blit_cropped(const uint8* data, const uint8* indexedData,
	uint16 imageLeft, uint16 imageTop, uint16 imageRight, uint16 imageBottom,
	uint16 imageWidth, uint16 imageHeight, const uint8 *palette,
	uint16 left, uint16 top)
{
	switch (sBootSplash.depth) {
		case 4:
		case 8:
			return;
		case 15:
			boot_splash_fb_blit15_cropped(data, imageLeft, imageTop,
				imageRight, imageBottom, imageWidth, imageHeight, palette,
				left, top);
			return;
		case 16:
			boot_splash_fb_blit16_cropped(data, imageLeft, imageTop,
				imageRight, imageBottom, imageWidth, imageHeight, palette,
				left, top);
			return;
		case 24:
			boot_splash_fb_blit24_cropped(data, imageLeft, imageTop,
				imageRight, imageBottom, imageWidth, imageHeight, palette,
				left, top);
			return;
		case 32:
			boot_splash_fb_blit32_cropped(data, imageLeft, imageTop,
				imageRight, imageBottom, imageWidth, imageHeight, palette,
				left, top);
			return;
	}
}


static status_t
boot_splash_fb_update(addr_t baseAddress, int32 width, int32 height,
	int32 depth, int32 bytesPerRow)
{
	TRACE("boot_splash_fb_update: buffer=%p, width=%ld, height=%ld, depth=%ld, "
		"bytesPerRow=%ld\n", (void *)baseAddress, width, height, depth,
		bytesPerRow);
	
	mutex_lock(&sBootSplash.lock);
	
	sBootSplash.frame_buffer = baseAddress;
	sBootSplash.width = width;
	sBootSplash.height = height;
	sBootSplash.depth = depth;
	sBootSplash.bytes_per_pixel = (depth + 7) / 8;
	sBootSplash.bytes_per_row = bytesPerRow;

	TRACE("boot_splash_fb_update: frame buffer mapped at %p\n",
		(void *)sBootSplash.frame_buffer);
	
	mutex_unlock(&sBootSplash.lock);
	return B_OK;
}


bool
boot_splash_fb_available(void)
{
	TRACE("boot_splash_fb_available: enter\n");
	return sBootSplash.frame_buffer != (addr_t)NULL;
}


status_t
boot_splash_fb_init(struct kernel_args *args)
{
	TRACE("boot_splash_fb_init: enter\n");

	if (!args->frame_buffer.enabled/* || sDebugScreenEnabled*/) {
		sBootSplash.enabled = false;
		return B_OK;
	}
	
	sBootSplash.enabled = true;
	
	void *frameBuffer;
	sBootSplash.area = map_physical_memory("vesa_fb",
		(void *)args->frame_buffer.physical_buffer.start,
		args->frame_buffer.physical_buffer.size, B_ANY_KERNEL_ADDRESS,
		B_READ_AREA | B_WRITE_AREA | B_USER_CLONEABLE_AREA, &frameBuffer);
	if (sBootSplash.area < B_OK)
		return sBootSplash.area;
	
	boot_splash_fb_update((addr_t)frameBuffer, args->frame_buffer.width,
		args->frame_buffer.height, args->frame_buffer.depth,
		args->frame_buffer.bytes_per_row);
	
	return B_OK;
}


void
boot_splash_set_stage(int stage)
{
	TRACE("boot_splash_set_stage: stage=%d\n", stage);

	if (!sBootSplash.enabled)
		return;

	if (stage < 0 || stage >= BOOT_SPLASH_STAGE_MAX)
		return;

	// TODO: Use placement info from images.h
	int x = sBootSplash.width / 2 - kSplashIconsWidth / 2;
	int y = sBootSplash.height / 2 - kSplashLogoHeight / 2;
	y = y + kSplashLogoHeight;

	int stageLeftEdge = kSplashIconsWidth * stage / BOOT_SPLASH_STAGE_MAX;
	int stageRightEdge = kSplashIconsWidth * (stage + 1)
		/ BOOT_SPLASH_STAGE_MAX;
	boot_splash_fb_blit_cropped(kSplashIconsImage, NULL, stageLeftEdge, 0,
		stageRightEdge, kSplashIconsHeight / 2,
		kSplashIconsWidth, kSplashIconsHeight, NULL, x, y );
}


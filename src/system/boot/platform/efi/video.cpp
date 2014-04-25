/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/platform/generic/video.h>
#include <boot/stage2.h>

#include "efi_platform.h"


static EFI_GUID sGraphicsOutputGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *sGraphicsOutput;
static UINTN sGraphicsMode;


extern "C" status_t
platform_init_video(void)
{
	EFI_STATUS status = kBootServices->LocateProtocol(&sGraphicsOutputGuid,
		NULL, (void **)&sGraphicsOutput);
	if (sGraphicsOutput == NULL || status != EFI_SUCCESS) {
		gKernelArgs.frame_buffer.enabled = false;
		sGraphicsOutput = NULL;
		return B_ERROR;
	}

	UINTN bestArea = 0;
	UINTN bestDepth = 0;

	for (UINTN mode = 0; mode < sGraphicsOutput->Mode->MaxMode; ++mode) {
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
		UINTN size, depth;
		sGraphicsOutput->QueryMode(sGraphicsOutput, mode, &size, &info);
		UINTN area = info->HorizontalResolution * info->VerticalResolution;
		if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
			depth = 32;
		else if (info->PixelFormat == PixelBitMask
			&& info->PixelInformation.RedMask == 0xFF0000
			&& info->PixelInformation.GreenMask == 0x00FF00
			&& info->PixelInformation.BlueMask == 0x0000FF
			&& info->PixelInformation.ReservedMask == 0)
			depth = 24;
		else
			continue;

		area *= depth;
		if (area >= bestArea) {
			bestArea = area;
			bestDepth = depth;
			sGraphicsMode = mode;
		}
	}

	if (bestArea == 0 || bestDepth == 0) {
		sGraphicsOutput = NULL;
		return B_ERROR;
	}

	return B_OK;
}


extern "C" void
platform_switch_to_logo(void)
{
	if (sGraphicsOutput == NULL || gKernelArgs.frame_buffer.enabled)
		return;

	sGraphicsOutput->SetMode(sGraphicsOutput, sGraphicsMode);
	gKernelArgs.frame_buffer.enabled = true;
	gKernelArgs.frame_buffer.physical_buffer.start =
		sGraphicsOutput->Mode->FrameBufferBase;
	gKernelArgs.frame_buffer.physical_buffer.size =
		sGraphicsOutput->Mode->FrameBufferSize;
	gKernelArgs.frame_buffer.width =
		sGraphicsOutput->Mode->Info->HorizontalResolution;
	gKernelArgs.frame_buffer.height =
		sGraphicsOutput->Mode->Info->VerticalResolution;
	gKernelArgs.frame_buffer.depth =
		sGraphicsOutput->Mode->Info->PixelFormat == PixelBitMask ? 24 : 32;
	gKernelArgs.frame_buffer.bytes_per_row =
		sGraphicsOutput->Mode->Info->PixelsPerScanLine
			* gKernelArgs.frame_buffer.depth / 8;

	video_display_splash(gKernelArgs.frame_buffer.physical_buffer.start);
}


extern "C" void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth,
	uint16 left, uint16 top)
{
	panic("platform_blit4 unsupported");
	return;
}


extern "C" void
platform_set_palette(const uint8 *palette)
{
	panic("platform_set_palette unsupported");
	return;
}

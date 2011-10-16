/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *
 * The phase coefficient computation was taken from the X driver written by
 * Alan Hourihane and David Dawes.
 */


#include "accelerant.h"
#include "accelerant_protos.h"
#include "commands.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <AGP.h>


//#define TRACE_OVERLAY
#ifdef TRACE_OVERLAY
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


#define NUM_HORIZONTAL_TAPS		5
#define NUM_VERTICAL_TAPS		3
#define NUM_HORIZONTAL_UV_TAPS	3
#define NUM_VERTICAL_UV_TAPS	3
#define NUM_PHASES				17
#define MAX_TAPS				5

struct phase_coefficient {
	uint8	sign;
	uint8	exponent;
	uint16	mantissa;
};


/*!	Splits the coefficient floating point value into the 3 components
	sign, mantissa, and exponent.
*/
static bool
split_coefficient(double &coefficient, int32 mantissaSize,
	phase_coefficient &splitCoefficient)
{
	double absCoefficient = fabs(coefficient);

	int sign;
	if (coefficient < 0.0)
		sign = 1;
	else
		sign = 0;

	int32 intCoefficient, res;
	int32 maxValue = 1 << mantissaSize;
	res = 12 - mantissaSize;

	if ((intCoefficient = (int)(absCoefficient * 4 * maxValue + 0.5))
			< maxValue) {
		splitCoefficient.exponent = 3;
		splitCoefficient.mantissa = intCoefficient << res;
		coefficient = (double)intCoefficient / (double)(4 * maxValue);
	} else if ((intCoefficient = (int)(absCoefficient * 2 * maxValue + 0.5))
			< maxValue) {
		splitCoefficient.exponent = 2;
		splitCoefficient.mantissa = intCoefficient << res;
		coefficient = (double)intCoefficient / (double)(2 * maxValue);
	} else if ((intCoefficient = (int)(absCoefficient * maxValue + 0.5))
			< maxValue) {
		splitCoefficient.exponent = 1;
		splitCoefficient.mantissa = intCoefficient << res;
		coefficient = (double)intCoefficient / (double)maxValue;
	} else if ((intCoefficient = (int)(absCoefficient * maxValue * 0.5 + 0.5))
			< maxValue) {
		splitCoefficient.exponent = 0;
		splitCoefficient.mantissa = intCoefficient << res;
		coefficient = (double)intCoefficient / (double)(maxValue / 2);
	} else {
		// coefficient out of range
		return false;
	}

	splitCoefficient.sign = sign;
	if (sign)
		coefficient = -coefficient;

	return true;
}


static void
update_coefficients(int32 taps, double filterCutOff, bool horizontal, bool isY,
	phase_coefficient* splitCoefficients)
{
	if (filterCutOff < 1)
		filterCutOff = 1;
	if (filterCutOff > 3)
		filterCutOff = 3;

	bool isVerticalUV = !horizontal && !isY;
	int32 mantissaSize = horizontal ? 7 : 6;

	double rawCoefficients[MAX_TAPS * 32], coefficients[NUM_PHASES][MAX_TAPS];

	int32 num = taps * 16;
	for (int32 i = 0; i < num * 2; i++) {
		double sinc;
		double value = (1.0 / filterCutOff) * taps * M_PI * (i - num)
			/ (2 * num);
		if (value == 0.0)
			sinc = 1.0;
		else
			sinc = sin(value) / value;

		// Hamming window
		double window = (0.5 - 0.5 * cos(i * M_PI / num));
		rawCoefficients[i] = sinc * window;
	}

	for (int32 i = 0; i < NUM_PHASES; i++) {
		// Normalise the coefficients
		double sum = 0.0;
		int32 pos;
		for (int32 j = 0; j < taps; j++) {
			pos = i + j * 32;
			sum += rawCoefficients[pos];
		}
		for (int32 j = 0; j < taps; j++) {
			pos = i + j * 32;
			coefficients[i][j] = rawCoefficients[pos] / sum;
		}

		// split them into sign/mantissa/exponent
		for (int32 j = 0; j < taps; j++) {
			pos = j + i * taps;

			split_coefficient(coefficients[i][j], mantissaSize
				+ (((j == (taps - 1) / 2) && !isVerticalUV) ? 2 : 0),
				splitCoefficients[pos]);
		}

		int32 tapAdjust[MAX_TAPS];
		tapAdjust[0] = (taps - 1) / 2;
		for (int32 j = 1, k = 1; j <= tapAdjust[0]; j++, k++) {
			tapAdjust[k] = tapAdjust[0] - j;
			tapAdjust[++k] = tapAdjust[0] + j;
		}

		// Adjust the coefficients
		sum = 0.0;
		for (int32 j = 0; j < taps; j++) {
			sum += coefficients[i][j];
		}

		if (sum != 1.0) {
			for (int32 k = 0; k < taps; k++) {
				int32 tap2Fix = tapAdjust[k];
				double diff = 1.0 - sum;

				coefficients[i][tap2Fix] += diff;
				pos = tap2Fix + i * taps;

				split_coefficient(coefficients[i][tap2Fix], mantissaSize
					+ (((tap2Fix == (taps - 1) / 2) && !isVerticalUV) ? 2 : 0),
					splitCoefficients[pos]);

				sum = 0.0;
				for (int32 j = 0; j < taps; j++) {
					sum += coefficients[i][j];
				}
				if (sum == 1.0)
					break;
			}
		}
	}
}


static void
set_color_key(uint8 red, uint8 green, uint8 blue, uint8 redMask,
	uint8 greenMask, uint8 blueMask)
{
	overlay_registers* registers = gInfo->overlay_registers;

	registers->color_key_red = red;
	registers->color_key_green = green;
	registers->color_key_blue = blue;
	registers->color_key_mask_red = ~redMask;
	registers->color_key_mask_green = ~greenMask;
	registers->color_key_mask_blue = ~blueMask;
	registers->color_key_enabled = true;
}


static void
set_color_key(const overlay_window* window)
{
	switch (gInfo->shared_info->current_mode.space) {
		case B_CMAP8:
			set_color_key(0, 0, window->blue.value, 0x0, 0x0, 0xff);
			break;
		case B_RGB15:
			set_color_key(window->red.value << 3, window->green.value << 3,
				window->blue.value << 3, window->red.mask << 3,
				window->green.mask << 3, window->blue.mask << 3);
			break;
		case B_RGB16:
			set_color_key(window->red.value << 3, window->green.value << 2,
				window->blue.value << 3, window->red.mask << 3,
				window->green.mask << 2, window->blue.mask << 3);
			break;

		default:
			set_color_key(window->red.value, window->green.value,
				window->blue.value, window->red.mask, window->green.mask,
				window->blue.mask);
			break;
	}
}


static void
update_overlay(bool updateCoefficients)
{
	if (!gInfo->shared_info->overlay_active
		|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_965))
		return;

	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);
	queue.PutFlush();
	queue.PutWaitFor(COMMAND_WAIT_FOR_OVERLAY_FLIP);
	queue.PutOverlayFlip(COMMAND_OVERLAY_CONTINUE, updateCoefficients);

	// make sure the flip is done now
	queue.PutWaitFor(COMMAND_WAIT_FOR_OVERLAY_FLIP);
	queue.PutFlush();

	TRACE(("update overlay: UP: %lx, TST: %lx, ST: %lx, CMD: %lx (%lx), "
		"ERR: %lx\n", read32(INTEL_OVERLAY_UPDATE), read32(INtEL_OVERLAY_TEST),
		read32(INTEL_OVERLAY_STATUS),
		*(((uint32*)gInfo->overlay_registers) + 0x68/4), read32(0x30168),
		read32(0x2024)));
}


static void
show_overlay(void)
{
	if (gInfo->shared_info->overlay_active
		|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_965))
		return;

	gInfo->shared_info->overlay_active = true;
	gInfo->overlay_registers->overlay_enabled = true;

	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);
	queue.PutOverlayFlip(COMMAND_OVERLAY_ON, true);
	queue.PutFlush();

	TRACE(("show overlay: UP: %lx, TST: %lx, ST: %lx, CMD: %lx (%lx), "
		"ERR: %lx\n", read32(INTEL_OVERLAY_UPDATE), read32(INTEL_OVERLAY_TEST),
		read32(INTEL_OVERLAY_STATUS),
		*(((uint32*)gInfo->overlay_registers) + 0x68/4), read32(0x30168),
		read32(0x2024)));
}


static void
hide_overlay(void)
{
	if (!gInfo->shared_info->overlay_active
		|| gInfo->shared_info->device_type.InGroup(INTEL_TYPE_965))
		return;

	overlay_registers* registers = gInfo->overlay_registers;

	gInfo->shared_info->overlay_active = false;
	registers->overlay_enabled = false;

	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);

	// flush pending commands
	queue.PutFlush();
	queue.PutWaitFor(COMMAND_WAIT_FOR_OVERLAY_FLIP);

	// clear overlay enabled bit
	queue.PutOverlayFlip(COMMAND_OVERLAY_CONTINUE, false);
	queue.PutWaitFor(COMMAND_WAIT_FOR_OVERLAY_FLIP);

	// turn off overlay engine
	queue.PutOverlayFlip(COMMAND_OVERLAY_OFF, false);
	queue.PutWaitFor(COMMAND_WAIT_FOR_OVERLAY_FLIP);

	gInfo->current_overlay = NULL;
}


//	#pragma mark -


uint32
intel_overlay_count(const display_mode* mode)
{
	// TODO: make this depending on the amount of RAM and the screen mode
	// (and we could even have more than one when using 3D as well)
	return 1;
}


const uint32*
intel_overlay_supported_spaces(const display_mode* mode)
{
	static const uint32 kSupportedSpaces[] = {B_RGB15, B_RGB16, B_RGB32,
		B_YCbCr422, 0};
	static const uint32 kSupportedi965Spaces[] = {B_YCbCr422, 0};
	intel_shared_info &sharedInfo = *gInfo->shared_info;

	if (sharedInfo.device_type.InGroup(INTEL_TYPE_96x))
		return kSupportedi965Spaces;

	return kSupportedSpaces;
}


uint32
intel_overlay_supported_features(uint32 colorSpace)
{
	return B_OVERLAY_COLOR_KEY
		| B_OVERLAY_HORIZONTAL_FILTERING
		| B_OVERLAY_VERTICAL_FILTERING
		| B_OVERLAY_HORIZONTAL_MIRRORING;
}


const overlay_buffer* 
intel_allocate_overlay_buffer(color_space colorSpace, uint16 width,
	uint16 height)
{
	TRACE(("intel_allocate_overlay_buffer(width %u, height %u, "
		"colorSpace %lu)\n", width, height, colorSpace));

	intel_shared_info &sharedInfo = *gInfo->shared_info;
	uint32 bytesPerPixel;

	switch (colorSpace) {
		case B_RGB15:
			bytesPerPixel = 2;
			break;
		case B_RGB16:
			bytesPerPixel = 2;
			break;
		case B_RGB32:
			bytesPerPixel = 4;
			break;
		case B_YCbCr422:
			bytesPerPixel = 2;
			break;
		default:
			return NULL;
	}

	struct overlay* overlay = (struct overlay*)malloc(sizeof(struct overlay));
	if (overlay == NULL)
		return NULL;

	// TODO: locking!

	// alloc graphics mem

	int32 alignment = 0x3f;
	if (sharedInfo.device_type.InGroup(INTEL_TYPE_965))
		alignment = 0xff;

	overlay_buffer* buffer = &overlay->buffer;
	buffer->space = colorSpace;
	buffer->width = width;
	buffer->height = height;
	buffer->bytes_per_row = (width * bytesPerPixel + alignment) & ~alignment;

	status_t status = intel_allocate_memory(buffer->bytes_per_row * height,
		0, overlay->buffer_base);
	if (status < B_OK) {
		free(overlay);
		return NULL;
	}

	if (sharedInfo.device_type.InGroup(INTEL_TYPE_965)) {
		status = intel_allocate_memory(INTEL_i965_OVERLAY_STATE_SIZE,
			B_APERTURE_NON_RESERVED, overlay->state_base);
		if (status < B_OK) {
			intel_free_memory(overlay->buffer_base);
			free(overlay);
			return NULL;
		}

		overlay->state_offset = overlay->state_base
			- (addr_t)gInfo->shared_info->graphics_memory;
	}

	overlay->buffer_offset = overlay->buffer_base
		- (addr_t)gInfo->shared_info->graphics_memory;

	buffer->buffer = (uint8*)overlay->buffer_base;
	buffer->buffer_dma = (uint8*)gInfo->shared_info->physical_graphics_memory
		+ overlay->buffer_offset;

	TRACE(("allocated overlay buffer: base=%x, offset=%x, address=%x, "
		"physical address=%x\n", overlay->buffer_base, overlay->buffer_offset,
		buffer->buffer, buffer->buffer_dma));

	return buffer;
}


status_t
intel_release_overlay_buffer(const overlay_buffer* buffer)
{
	TRACE(("intel_release_overlay_buffer(buffer %p)\n", buffer));

	struct overlay* overlay = (struct overlay*)buffer;

	// TODO: locking!

	if (gInfo->current_overlay == overlay)
		hide_overlay();

	intel_free_memory(overlay->buffer_base);
	if (gInfo->shared_info->device_type.InGroup(INTEL_TYPE_965))
		intel_free_memory(overlay->state_base);
	free(overlay);

	return B_OK;
}


status_t
intel_get_overlay_constraints(const display_mode* mode,
	const overlay_buffer* buffer, overlay_constraints* constraints)
{
	TRACE(("intel_get_overlay_constraints(buffer %p)\n", buffer));

	// taken from the Radeon driver...

	// scaler input restrictions
	// TODO: check all these values; most of them are probably too restrictive

	// position
	constraints->view.h_alignment = 0;
	constraints->view.v_alignment = 0;

	// alignment
	switch (buffer->space) {
		case B_RGB15:
			constraints->view.width_alignment = 7;
			break;
		case B_RGB16:
			constraints->view.width_alignment = 7;
			break;
		case B_RGB32:
			constraints->view.width_alignment = 3;
			break;
		case B_YCbCr422:
			constraints->view.width_alignment = 7;
			break;
		case B_YUV12:
			constraints->view.width_alignment = 7;
			break;
		default:
			return B_BAD_VALUE;
	}
	constraints->view.height_alignment = 0;

	// size
	constraints->view.width.min = 4;		// make 4-tap filter happy
	constraints->view.height.min = 4;
	constraints->view.width.max = buffer->width;
	constraints->view.height.max = buffer->height;

	// scaler output restrictions
	constraints->window.h_alignment = 0;
	constraints->window.v_alignment = 0;
	constraints->window.width_alignment = 0;
	constraints->window.height_alignment = 0;
	constraints->window.width.min = 2;
	constraints->window.width.max = mode->virtual_width;
	constraints->window.height.min = 2;
	constraints->window.height.max = mode->virtual_height;

	// TODO: the minimum values are not tested
	constraints->h_scale.min = 1.0f / (1 << 4);
	constraints->h_scale.max = buffer->width * 7;
	constraints->v_scale.min = 1.0f / (1 << 4);
	constraints->v_scale.max = buffer->height * 7;

	return B_OK;
}


overlay_token
intel_allocate_overlay(void)
{
	TRACE(("intel_allocate_overlay()\n"));

	// we only have a single overlay channel
	if (atomic_or(&gInfo->shared_info->overlay_channel_used, 1) != 0)
		return NULL;

	return (overlay_token)++gInfo->shared_info->overlay_token;
}


status_t
intel_release_overlay(overlay_token overlayToken)
{
	TRACE(("intel_allocate_overlay(token %ld)\n", (uint32)overlayToken));

	// we only have a single token, which simplifies this
	if (overlayToken != (overlay_token)gInfo->shared_info->overlay_token)
		return B_BAD_VALUE;

	atomic_and(&gInfo->shared_info->overlay_channel_used, 0);

	return B_OK;
}


status_t
intel_configure_overlay(overlay_token overlayToken,
	const overlay_buffer* buffer, const overlay_window* window,
	const overlay_view* view)
{
	TRACE(("intel_configure_overlay: buffer %p, window %p, view %p\n",
		buffer, window, view));

	if (overlayToken != (overlay_token)gInfo->shared_info->overlay_token)
		return B_BAD_VALUE;

	if (window == NULL && view == NULL) {
		hide_overlay();
		return B_OK;
	}

	struct overlay* overlay = (struct overlay*)buffer;
	overlay_registers* registers = gInfo->overlay_registers;
	bool updateCoefficients = false;
	uint32 bytesPerPixel = 2;

	switch (buffer->space) {
		case B_RGB15:
			registers->source_format = OVERLAY_FORMAT_RGB15;
			break;
		case B_RGB16:
			registers->source_format = OVERLAY_FORMAT_RGB16;
			break;
		case B_RGB32:
			registers->source_format = OVERLAY_FORMAT_RGB32;
			bytesPerPixel = 4;
			break;
		case B_YCbCr422:
			registers->source_format = OVERLAY_FORMAT_YCbCr422;
			break;
	}

	if (!gInfo->shared_info->overlay_active
		|| memcmp(&gInfo->last_overlay_view, view, sizeof(overlay_view))
		|| memcmp(&gInfo->last_overlay_frame, window, sizeof(overlay_frame))) {
		// scaling has changed, program window and scaling factor

		// clip the window to on screen bounds
		// TODO: this is not yet complete or correct - especially if we start
		// to support moving the display!
		int32 left, top, right, bottom;
		left = window->h_start;
		right = window->h_start + window->width;
		top = window->v_start;
		bottom = window->v_start + window->height;
		if (left < 0)
			left = 0;
		if (top < 0)
			top = 0;
		if (right > gInfo->shared_info->current_mode.timing.h_display)
			right = gInfo->shared_info->current_mode.timing.h_display;
		if (bottom > gInfo->shared_info->current_mode.timing.v_display)
			bottom = gInfo->shared_info->current_mode.timing.v_display;
		if (left >= right || top >= bottom) {
			// overlay is not within visible bounds
			hide_overlay();
			return B_OK;
		}

		registers->window_left = left;
		registers->window_top = top;
		registers->window_width = right - left;
		registers->window_height = bottom - top;

		uint32 horizontalScale = (view->width << 12) / window->width;
		uint32 verticalScale = (view->height << 12) / window->height;
		uint32 horizontalScaleUV = horizontalScale >> 1;
		uint32 verticalScaleUV = verticalScale >> 1;
		horizontalScale = horizontalScaleUV << 1;
		verticalScale = verticalScaleUV << 1;

		// we need to offset the overlay view to adapt it to the clipping
		// (in addition to whatever offset is desired already)
		left = view->h_start - (int32)((window->h_start - left)
			* (horizontalScale / 4096.0) + 0.5);
		top = view->v_start - (int32)((window->v_start - top)
			* (verticalScale / 4096.0) + 0.5);
		right = view->h_start + view->width;
		bottom = view->v_start + view->height;

		gInfo->overlay_position_buffer_offset = buffer->bytes_per_row * top
			+ left * bytesPerPixel;

		// Note: in non-planar mode, you *must* not program the source
		// width/height UV registers - they must stay cleared, or the chip is
		// doing strange stuff.
		// On the other hand, you have to program the UV scaling registers, or
		// the result will be wrong, too.
		registers->source_width_rgb = right - left;
		registers->source_height_rgb = bottom - top;
		if (gInfo->shared_info->device_type.InFamily(INTEL_TYPE_8xx)) {
			registers->source_bytes_per_row_rgb = (((overlay->buffer_offset
				+ (view->width << 1) + 0x1f) >> 5)
				- (overlay->buffer_offset >> 5) - 1) << 2;
		} else {
			registers->source_bytes_per_row_rgb = ((((overlay->buffer_offset
				+ (view->width << 1) + 0x3f) >> 6)
				- (overlay->buffer_offset >> 6) << 1) - 1) << 2;
		}

		// horizontal scaling
		registers->scale_rgb.horizontal_downscale_factor
			= horizontalScale >> 12;
		registers->scale_rgb.horizontal_scale_fraction
			= horizontalScale & 0xfff;
		registers->scale_uv.horizontal_downscale_factor
			= horizontalScaleUV >> 12;
		registers->scale_uv.horizontal_scale_fraction
			= horizontalScaleUV & 0xfff;

		// vertical scaling
		registers->scale_rgb.vertical_scale_fraction = verticalScale & 0xfff;
		registers->scale_uv.vertical_scale_fraction = verticalScaleUV & 0xfff;
		registers->vertical_scale_rgb = verticalScale >> 12;
		registers->vertical_scale_uv = verticalScaleUV >> 12;

		TRACE(("scale: h = %ld.%ld, v = %ld.%ld\n", horizontalScale >> 12,
			horizontalScale & 0xfff, verticalScale >> 12,
			verticalScale & 0xfff));

		if (verticalScale != gInfo->last_vertical_overlay_scale
			|| horizontalScale != gInfo->last_horizontal_overlay_scale) {
			// Recompute phase coefficients (taken from X driver)
			updateCoefficients = true;

			phase_coefficient coefficients[NUM_HORIZONTAL_TAPS * NUM_PHASES];
			update_coefficients(NUM_HORIZONTAL_TAPS, horizontalScale / 4096.0,
				true, true, coefficients);

			phase_coefficient coefficientsUV[
				NUM_HORIZONTAL_UV_TAPS * NUM_PHASES];
			update_coefficients(NUM_HORIZONTAL_UV_TAPS,
				horizontalScaleUV / 4096.0, true, false, coefficientsUV);

			int32 pos = 0;
			for (int32 i = 0; i < NUM_PHASES; i++) {
				for (int32 j = 0; j < NUM_HORIZONTAL_TAPS; j++) {
					registers->horizontal_coefficients_rgb[pos]
						= coefficients[pos].sign << 15
							| coefficients[pos].exponent << 12
							| coefficients[pos].mantissa;
					pos++;
				}
			}

			pos = 0;
			for (int32 i = 0; i < NUM_PHASES; i++) {
				for (int32 j = 0; j < NUM_HORIZONTAL_UV_TAPS; j++) {
					registers->horizontal_coefficients_uv[pos]
						= coefficientsUV[pos].sign << 15
							| coefficientsUV[pos].exponent << 12
							| coefficientsUV[pos].mantissa;
					pos++;
				}
			}

			gInfo->last_vertical_overlay_scale = verticalScale;
			gInfo->last_horizontal_overlay_scale = horizontalScale;
		}

		gInfo->last_overlay_view = *view;
		gInfo->last_overlay_frame = *(overlay_frame*)window;
	}

	registers->color_control_output_mode = true;
	registers->select_pipe = 0;

	// program buffer

	registers->buffer_rgb0
		= overlay->buffer_offset + gInfo->overlay_position_buffer_offset;
	registers->stride_rgb = buffer->bytes_per_row;

	registers->mirroring_mode
		= (window->flags & B_OVERLAY_HORIZONTAL_MIRRORING) != 0
			? OVERLAY_MIRROR_HORIZONTAL : OVERLAY_MIRROR_NORMAL;
	registers->ycbcr422_order = 0;

	if (!gInfo->shared_info->overlay_active) {
		// overlay is shown for the first time
		set_color_key(window);
		show_overlay();
	} else
		update_overlay(updateCoefficients);

	gInfo->current_overlay = overlay;
	return B_OK;
}


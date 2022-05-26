/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GRAPHICS_CARD_H
#define _GRAPHICS_CARD_H


#include <GraphicsDefs.h>


/* #pragma mark - command constants */
enum {
	B_OPEN_GRAPHICS_CARD,
	B_CLOSE_GRAPHICS_CARD,
	B_GET_GRAPHICS_CARD_INFO,
	B_GET_GRAPHICS_CARD_HOOKS,
	B_SET_INDEXED_COLOR,
	B_GET_SCREEN_SPACES,
	B_CONFIG_GRAPHICS_CARD,
	B_GET_REFRESH_RATES,
	B_SET_SCREEN_GAMMA,

	B_GET_INFO_FOR_CLONE_SIZE,
	B_GET_INFO_FOR_CLONE,
	B_SET_CLONED_GRAPHICS_CARD,
	B_CLOSE_CLONED_GRAPHICS_CARD,
	B_PROPOSE_FRAME_BUFFER,
	B_SET_FRAME_BUFFER,
	B_SET_DISPLAY_AREA,
	B_MOVE_DISPLAY_AREA
};

/* #pragma mark - optional flags */
enum {
	B_CRT_CONTROL				= 0x0001,
	B_GAMMA_CONTROL				= 0x0002,
	B_FRAME_BUFFER_CONTROL		= 0x0004,
	B_PARALLEL_BUFFER_ACCESS	= 0x0008,
	B_LAME_ASS_CARD				= 0x0010
};


/* #pragma mark - structures */


typedef struct {
	int16			version;
	int16			id;
	void*			frame_buffer;
	char			rgba_order[4];
	int16			flags;
	int16			bits_per_pixel;
	int16			bytes_per_row;
	int16			width;
	int16			height;
} graphics_card_info;


typedef struct {
	int32			index;
	rgb_color		color;
} indexed_color;


typedef struct {
	uint32			space;
	float			refresh_rate;
	uchar			h_position;
	uchar			v_position;
	uchar			h_size;
	uchar			v_size;
} graphics_card_config;


typedef struct {
	float			min;
	float			max;
	float			current;
} refresh_rate_info;


typedef struct {
	void*			screen_base;
	void*			io_base;
	uint32			vendor_id;
	uint32			device_id;
	uint32			_reserved1_;
	uint32			_reserved2_;
} graphics_card_spec;


typedef struct {
	int16			x1;
	int16			y1;
	int16			x2;
	int16			y2;
	rgb_color		color;
} rgb_color_line;


typedef struct {
	int16			x1;
	int16			y1;
	int16			x2;
	int16			y2;
	uchar			color;
} indexed_color_line;


typedef struct {
    int16			bits_per_pixel;
	int16			bytes_per_row;
	int16			width;
	int16			height;
    int16			display_width;
	int16			display_height;
	int16			display_x;
	int16			display_y;
} frame_buffer_info;


typedef struct {
	uchar			red[256];
	uchar			green[256];
	uchar			blue[256];
} screen_gamma;


/* #pragma mark - hook function */


typedef void (*graphics_card_hook) ();

#define B_GRAPHICS_HOOK_COUNT	48


#ifdef __cplusplus
extern "C" {
#endif

int32 control_graphics_card(uint32, void*);	

#ifdef __cplusplus
}
#endif


/* #pragma mark - deprecated */
#define B_HOOK_COUNT  B_GRAPHICS_HOOK_COUNT


#endif /* _GRAPHICS_CARD_H */

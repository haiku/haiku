/*
   - Overlay support
   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
*/

#ifndef __VOODOO3_ACCELERANT_H__
#define __VOODOO3_ACCELERANT_H__

#include <SupportDefs.h>
#include <video_overlay.h>

typedef struct
{
	uint8 *regs_base;
	uint32 io_base;
	uint32 bpp, width, height;
} Voodoo3Accel;

typedef struct
{
	uint8 seq_regs[5];
	uint8 crtc_regs[25];
	uint8 attr_regs[21];
	uint8 graph_regs[9];
	uint8 misc;
} MonitorRegs;

typedef struct
{
	int x;
	int y;
	int w;
	int h;
} list_packet;

typedef struct
{
	int src_x;
	int src_y;
	int dest_x;
	int dest_y;
	int width;
	int height;
} list_packet_blit;

// Initialize
int voodoo3_init(uint8 *registers_base, uint32 io_base);
void voodoo3_set_monitor_defaults();
void voodoo3_wait_idle();
void voodoo_set_desktop_regs(uint32 bpp, display_mode *dm);

// Cursor functionality
void voodoo3_show_cursor();
void voodoo3_hide_cursor();
void voodoo3_move_cursor(int x, int y);
void voodoo3_set_cursor(uint8 *data, uint16 width, uint8 height, uint8 *andMask, uint8 *xorMask);
void voodoo3_set_cursor_colors(int bg, int fg);
void voodoo3_init_cursor_address();

// handle colortable
void voodoo3_set_palette(int index, uint32 color);

// accelerated 2d functions
void voodoo3_screen_to_screen_blit(list_packet_blit *list, uint32 bytes_per_row, uint32 bits_per_pixel);
void voodoo3_fill_rect(list_packet *list, uint32 color, uint32 bytes_per_row, uint32 bits_per_pixel);
void voodoo3_invert_rect(list_packet *list, uint32 color, uint32 bytes_per_row, uint32 bits_per_pixel);
void voodoo3_fill_span(list_packet *list, uint32 color, uint32 bytes_per_row, uint32 bits_per_pixel);

// Runtime information
uint32 voodoo3_bits_per_pixel();
uint32 voodoo3_get_memory_size();

void voodoo3_setup_monitor(display_mode *);

// Overlay
void voodoo3_display_overlay(const overlay_window *ow, const overlay_buffer *ob, const overlay_view* ov);
void voodoo3_reset_overlay();
void voodoo3_stop_overlay();

#endif

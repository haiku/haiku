/*
   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
  
*/

#include "GlobalData.h"
#include "generic.h"
#include "voodoo3_accelerant.h"

void
SCREEN_TO_SCREEN_BLIT (
    engine_token *et, blit_params *list, uint32 count)
{
	uint32 stride, bpp;
	stride = si->fbc.bytes_per_row;
	bpp = voodoo3_bits_per_pixel();
	
    while (count--)
    {
    	list_packet_blit packet;
    	packet.src_x = list->src_left;
    	packet.src_y = list->src_top;
    	packet.dest_x = list->dest_left;
    	packet.dest_y = list->dest_top;
    	packet.width  = list->width + 1;
    	packet.height = list->height + 1;
    	
		voodoo3_screen_to_screen_blit(&packet, stride, bpp);
        list ++;
    }
}

void
FILL_RECTANGLE (
    engine_token *et, uint32 color, fill_rect_params *list, uint32 count)
{
	uint32 stride, bpp;
	stride = si->fbc.bytes_per_row;
	bpp = voodoo3_bits_per_pixel();
	
    while (count--)
    {
    	list_packet packet;
    	packet.x = list->left;
    	packet.y = list->top;
    	packet.w = list->right - packet.x + 1;
    	packet.h = list->bottom - packet.y + 1;
    	voodoo3_fill_rect(&packet, color, stride, bpp);
        list++;
    }
}

void
INVERT_RECTANGLE (
    engine_token *et, fill_rect_params *list, uint32 count)
{
	uint32 stride, bpp, color;
	stride = si->fbc.bytes_per_row;
	bpp = voodoo3_bits_per_pixel();
	color = 0;
	
    while (count--)
    {
    	list_packet packet;
    	packet.x = list->left;
    	packet.y = list->top;
    	packet.w = list->right - packet.x + 1;
    	packet.h = list->bottom - packet.y + 1;
    	voodoo3_invert_rect(&packet, color, stride, bpp);
        list++;
    }
}

void
FILL_SPAN (
    engine_token *et, uint32 color, uint16 *list, uint32 count)
{
	uint32 stride, bpp;
	stride = si->fbc.bytes_per_row;
	bpp = voodoo3_bits_per_pixel();

    while (count--)
    {
		list_packet packet;
		packet.y = *list ++;
		packet.x = *list ++;
		packet.w = *list - packet.x + 1;
		packet.h = 1;
		list ++;
		voodoo3_fill_span(&packet, color, stride, bpp);
    }
}

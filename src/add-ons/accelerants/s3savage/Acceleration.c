/*
 * Copyright 1999  Erdi Chen
 */

#include "GlobalData.h"
#include "generic.h"
#include "s3accel.h"
#include "s3mmio.h"

void
SCREEN_TO_SCREEN_BLIT (
    engine_token *et, blit_params *list, uint32 count)
{
    s3accel_wait_for_fifo (1);
    UpdateGEReg16 (FRGD_MIX, 0x0067);
	    
    while (count--)
    {
        int cmd = 0xc0b1;
        int src_x = list->src_left;
        int src_y = list->src_top;
        int dest_x = list->dest_left;
        int dest_y = list->dest_top;
        int width  = list->width;
        int height = list->height;
        
        if (src_x < dest_x)
        {
            src_x += width; dest_x += width;
            cmd ^=32;
        }
                        
        if (src_y < dest_y)
        {
            src_y += height; dest_y += height;
            cmd ^=128;
        }
                
        UpdateGEReg (ALT_CURXY, (src_x<<16)|src_y);  
        UpdateGEReg (ALT_STEP, (dest_x<<16)|dest_y); 
        UpdateGEReg (ALT_PCNT, (width<<16)|height);  
        UpdateGEReg (CMD, cmd);
        
        list ++;
    }

    s3accel_wait_for_fifo (3);
    UpdateGEReg16 (FRGD_MIX, 0x0027);
}

void
FILL_RECTANGLE (
    engine_token *et, uint32 color, fill_rect_params *list, uint32 count)
{
    s3accel_wait_for_fifo (1);
    UpdateGEReg (FRGD_COLOR, color);
    UpdateGEReg16 (FRGD_MIX, 0x0027);
	
    while (count--)
    {
        int x = list->left;
        int y = list->top;
        int w = list->right - x;
        int h = list->bottom - y;

        s3accel_wait_for_fifo (2);
        UpdateGEReg   (ALT_CURXY, (x<<16)|y);
        UpdateGEReg   (ALT_PCNT, (w<<16)|h); 
        UpdateGEReg (CMD,  0x000040b1);

        list++;
    }
    s3accel_wait_for_fifo(3);
}

void
INVERT_RECTANGLE (
    engine_token *et, fill_rect_params *list, uint32 count)
{
    s3accel_wait_for_fifo (1);
    UpdateGEReg16 (FRGD_MIX, 0x0020);
	
    while (count--)
    {
        int x = list->left;
        int y = list->top;
        int w = list->right - x;
        int h = list->bottom - y;

        s3accel_wait_for_fifo (2);
        UpdateGEReg   (ALT_CURXY, (x<<16)|y);
        UpdateGEReg   (ALT_PCNT, (w<<16)|h); 
        UpdateGEReg (CMD,  0x000040b1);

        list++;
    }

    s3accel_wait_for_fifo (3);
    UpdateGEReg16 (FRGD_MIX, 0x0027); 
}

void
FILL_SPAN (
    engine_token *et, uint32 color, uint16 *list, uint32 count)
{
    s3accel_wait_for_fifo (1);
    UpdateGEReg (FRGD_COLOR, color);
    UpdateGEReg16 (FRGD_MIX, 0x0027);

    while (count--)
    {
        int y = *list ++;
        int x = *list ++;
        int w = *list - x; list ++;

        s3accel_wait_for_fifo (2);
        UpdateGEReg   (ALT_CURXY, (x<<16)|y);
        UpdateGEReg   (ALT_PCNT, (w<<16)|1);
        UpdateGEReg (CMD,  0x000040b1);
    } 
}

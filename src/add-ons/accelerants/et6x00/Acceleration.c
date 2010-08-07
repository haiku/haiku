/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"
#include "generic.h"


/*****************************************************************************/
/*
 * Set bits in a byte pointed by addr; mask must contain 0s at the bits
 * positions to be set and must contain 1s at all other bits; val must
 * contain the values of bits to be set.
 */
static __inline void set8(volatile unsigned char *addr, unsigned char mask,
	unsigned char val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
static __inline unsigned char get8(volatile unsigned char *addr)
{
    return *addr;
}
/*****************************************************************************/
static __inline void et6000aclTerminate(void) {
    set8(mmRegs+0x31, 0xef, 0x10); /* let ACL to operate */
    et6000aclWaitIdle();
    set8(mmRegs+0x30, 0, 0x00);
    set8(mmRegs+0x30, 0, 0x01);
    et6000aclWaitIdle();
    set8(mmRegs+0x30, 0, 0x00);
    set8(mmRegs+0x30, 0, 0x10);
    et6000aclWaitIdle();
    set8(mmRegs+0x30, 0, 0x00);
}
/*****************************************************************************/
/*
 * bpp must be bytes per pixel, not bits!
 */
void et6000aclInit(uint8 bpp) {

    et6000aclTerminate();

    set8(mmRegs+0x31, 0xef, 0x10); /* let ACL to operate */
    set8(mmRegs+0x32, 0x99, 0x00); /* maximize the performance */
    set8(mmRegs+0x8e, 0xcf, (bpp - 1) << 4); /* set pixel color depth */
    set8(mmRegs+0x91, 0x80, 0x00); /* maximize the performance */
    set8(mmRegs+0x9d, 0x00, 0x00); /* maximize the performance */
}
/*****************************************************************************/
/*
 * Wait until ACL becomes idle.
 */
void et6000aclWaitIdle(void) {
    while ((get8(mmRegs+0x36) & 0x02) == 0x02);
}
/*****************************************************************************/
/*
 * Wait until ACL queue becomes not full.
 */
static __inline void et6000aclWaitQueueNotFull(void) {
    while ((get8(mmRegs+0x36) & 0x01) == 0x01);
}
/*****************************************************************************/
/*
 * Move the specified list of rectangular regions from one location in
 * the frame buffer to another in the order they are specified in the
 * blit_params *list. The list is uint32 count elements in length.
 */
void SCREEN_TO_SCREEN_BLIT(engine_token *et,
                           blit_params *list,
                           uint32 count)
{
uint16 screenWidth = si->dm.virtual_width;
uint8 bpp = si->bytesPerPixel;
uint8 bltDir;
uint16 src_left, src_top, dest_left, dest_top, width, height;
uint32 srcAddr = 0, destAddr = 0;

    et6000aclWaitQueueNotFull();

    set8(mmRegs+0x92, 0x80, 0x77); /* no source wrap */    
    set8(mmRegs+0x9c, 0x00, 0x33); /* mask=1 always, always use FGR */
    set8(mmRegs+0x9f, 0x00, 0xcc); /* FGR ROP = copy of source */

    /* Set the source Y offset */
    *((vuint16 *)(mmRegs+0x8a)) = screenWidth * bpp - 1;
        
    /* Set the destination Y offset */
    *((vuint16 *)(mmRegs+0x8c)) = screenWidth * bpp - 1;
    
    while(count--) {
        src_left = list->src_left;
        src_top = list->src_top;
        dest_left = list->dest_left;
        dest_top = list->dest_top;
        width = list->width;
        height = list->height;

        et6000aclWaitQueueNotFull();

        /* Set the direction and opcode(BitBLT) register */
        bltDir = 0x00;
        if (src_left < dest_left) bltDir |= 0x01;
        if (src_top < dest_top) bltDir |= 0x02;
        set8(mmRegs+0x8f, 0x3c, bltDir);

        /* Set the X count register */
        *((vuint16 *)(mmRegs+0x98)) = (width + 1) * bpp - 1;
        
        /* Set the Y count register */
        *((vuint16 *)(mmRegs+0x9a)) = height;

        switch (bltDir & 0x03) {
        case 0x00:
            srcAddr = (src_top * screenWidth + src_left) * bpp;
            destAddr = (dest_top * screenWidth + dest_left) * bpp;
            break;

        case 0x01:
            srcAddr =  (src_top * screenWidth + src_left + width) * bpp + bpp-1;
            destAddr = (dest_top * screenWidth + dest_left + width) * bpp + bpp-1;
            break;

        case 0x02:
            srcAddr = ((src_top + height)*screenWidth + src_left) * bpp;
            destAddr = ((dest_top + height)*screenWidth + dest_left) * bpp;
            break;

        case 0x03:
            srcAddr = ((src_top + height)*screenWidth + src_left + width) * bpp + bpp-1;
            destAddr = ((dest_top + height)*screenWidth + dest_left + width) * bpp + bpp-1;
            break;
        }

        /* Set the source address */
        *((vuint32 *)(mmRegs+0x84)) = srcAddr;

        /*
         * Set the destination address -
         * this action starts the BitBLT operation.
         */
        *((vuint32 *)(mmRegs+0xa0)) = destAddr;

        list++;
    }

    si->engine.count++;
}
/*****************************************************************************/
/*
 * Fill the specified list of rectangular regions with the specified color.
 * The list is uint32 count elements in length. The rectangular regions are
 * inclusive. The uint32 color is specified in the same configuration and
 * byte order as the current display_mode. All coordinates in the list of
 * rectangles is guaranteed to have been clipped to the virtual limits of
 * the display_mode.
 */
void FILL_RECTANGLE(engine_token *et,
                    uint32 color,
                    fill_rect_params *list,
                    uint32 count)
{
uint16 screenWidth = si->dm.virtual_width;
uint8 bpp = si->bytesPerPixel;
uint16 left, top, right, bottom;
uint32 srcAddr;
uint8 i;

    /*
     * Normally WaitQueueNotFull should be required & enough, but in reality
     * this is somewhy sometimes not enough for pixel depth of 3 bytes.
     */
    if (bpp == 2)
        et6000aclWaitQueueNotFull();
    else
        et6000aclWaitIdle();

    /*
     * We'll put the color at 4 bytes just after the framebuffer.
     * The srcAddr must be 4 bytes aligned and is always for standard
     * resolutions.
     */
    srcAddr = (uint32)si->framebuffer - (uint32)si->memory +
        si->dm.virtual_width * si->dm.virtual_height * bpp;

    switch(bpp) {
        case 2:
            set8(mmRegs+0x92, 0x80, 0x02); /* 4x1 source wrap */
            for (i = 0; i < 2; i++) /* copy the color to source address */
                ((vuint16 *)((uint32)si->memory + srcAddr))[i] = (uint16)color;
            break;
        case 3:
            set8(mmRegs+0x92, 0x80, 0x0a); /* 3x1 source wrap */
            for (i = 0; i < 3; i++) /* copy the color to source address */
                ((vuint8 *)((uint32)si->memory + srcAddr))[i] = ((uint8 *)&color)[i];

            break;
    }

    set8(mmRegs+0x9c, 0x00, 0x33); /* mask=1 always, always use FGR */
    set8(mmRegs+0x9f, 0x00, 0xcc); /* FGR ROP = copy of source */

    /* Set the source Y offset */
    *((vuint16 *)(mmRegs+0x8a)) = screenWidth * bpp - 1;
    /* Set the destination Y offset */
    *((vuint16 *)(mmRegs+0x8c)) = screenWidth * bpp - 1;

    /* Set the direction and opcode(trapezoid) register (primary edge) */
    set8(mmRegs+0x8f, 0x18, 0x40);
    /* Set the secondary edge register */
    set8(mmRegs+0x93, 0x1a, 0x00);

    /* Set the primary delta minor register */
    *((vuint16 *)(mmRegs+0xac)) = 0;
    /* Set the secondary delta minor register */
    *((vuint16 *)(mmRegs+0xb4)) = 0;

    while(count--) {
        left = list->left;
        top = list->top;
        right = list->right;
        bottom = list->bottom;

        et6000aclWaitQueueNotFull();

        /* Set the X count register */
        *((vuint16 *)(mmRegs+0x98)) = (right-left+1)*bpp - 1;
        /* Set the Y count register */
        *((vuint16 *)(mmRegs+0x9a)) = bottom-top;

        /* Set the primary delta major register */
        *((vuint16 *)(mmRegs+0xae)) = bottom-top;

        /* Set the secondary delta major register */
        *((vuint16 *)(mmRegs+0xb6)) = bottom-top;

        /* Set the source address */
        *((vuint32 *)(mmRegs+0x84)) = srcAddr;

        /*
         * Set the destination address -
         * this action starts the trapezoid operation.
         */
        *((vuint32 *)(mmRegs+0xa0)) = (top * screenWidth + left) * bpp;

        list++;
    }

    si->engine.count++;
}
/*****************************************************************************/

/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 graphics driver for BeOS 5.
 * Copyright (c) 2003-2004, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "GlobalData.h"
#include "generic.h"
#include <string.h>
#include <sys/ioctl.h>


/*****************************************************************************/
#define T_POSITIVE_SYNC (B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS      (B_SCROLL | B_8_BIT_DAC | B_PARALLEL_ACCESS)
#define MODE_COUNT (sizeof (modesList) / sizeof (display_mode))
/*****************************************************************************/
static const display_mode modesList[] = {
{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_RGB24, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_RGB24, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_RGB24, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_RGB24, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_RGB24, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_RGB24, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
{ { 38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_RGB24, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
{ { 40000, 800, 840, 968, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_RGB24, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
{ { 49500, 800, 816, 896, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_RGB24, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
{ { 50000, 800, 856, 976, 1040, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_RGB24, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
{ { 56250, 800, 832, 896, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_RGB24, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_RGB24, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
{ { 75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_RGB24, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
{ { 78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_RGB24, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
{ { 94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_RGB24, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
{ { 94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_RGB24, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_RGB24, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
{ { 121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_RGB24, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
{ { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_RGB24, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
{ { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_RGB24, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
{ { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_RGB24, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
{ { 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_RGB24, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
{ { 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_RGB24, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
{ { 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_RGB24, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
{ { 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_RGB24, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
{ { 216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_RGB24, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
{ { 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_RGB24, 1600, 1200, 0, 0, MODE_FLAGS}  /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
};
/*****************************************************************************/
/*
 * Validate a target display mode is both
 *      a) a valid display mode for this device and
 *      b) falls between the contraints imposed by "low" and "high"
 *
 * If the mode is not (or cannot) be made valid for this device, return B_ERROR.
 * If a valid mode can be constructed, but it does not fall within the limits,
 *  return B_BAD_VALUE.
 * If the mode is both valid AND falls within the limits, return B_OK.
 */
status_t PROPOSE_DISPLAY_MODE(display_mode *target,
                              const display_mode *low,
                              const display_mode *high)
{
ET6000DisplayMode mode;

    mode.magic = ET6000_PRIVATE_DATA_MAGIC;
    mode.mode = *target;
    mode.memSize = si->memSize;
    
    return ioctl(fd, ET6000_PROPOSE_DISPLAY_MODE, &mode, sizeof(mode));
}
/*****************************************************************************/
/*
 * Return the number of modes this device will return from GET_MODE_LIST().
 */
uint32 ACCELERANT_MODE_COUNT(void) {
        /* return the number of 'built-in' display modes */
        return si->modesNum;
}
/*****************************************************************************/
/*
 * Copy the list of guaranteed supported video modes to the location provided.
 */
status_t GET_MODE_LIST(display_mode *dm) {
    /* copy them to the buffer pointed at by *dm */
    memcpy(dm, et6000ModesList, si->modesNum * sizeof(display_mode));
    return B_OK;
}
/*****************************************************************************/
/*
 * Create a list of display_modes to pass back to the caller.
 */
status_t createModesList(void) {
size_t maxSize;
uint32 i, j, pixelClockRange;
const display_mode *src;
display_mode *dst, low, high;
color_space spaces[] = {B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB24_LITTLE};


    /* figure out how big the list could be, and adjust up to nearest multiple of B_PAGE_SIZE */
    maxSize = (((MODE_COUNT * 3) * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

    /* Create an area to hold the info */
    si->modesArea = et6000ModesListArea =
        create_area("ET6000 accelerant mode info", (void **)&et6000ModesList,
            B_ANY_ADDRESS, maxSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

    if (et6000ModesListArea < B_OK)
        return et6000ModesListArea;

    /* walk through our predefined list and see which modes fit this device */
    src = modesList;
    dst = et6000ModesList;
    si->modesNum = 0;
    for (i = 0; i < MODE_COUNT; i++) {
        /* set ranges for acceptable values */
        low = high = *src;
        /* range is 6.25% of default clock: arbitrarily picked */
        pixelClockRange = low.timing.pixel_clock >> 5;
        low.timing.pixel_clock -= pixelClockRange;
        high.timing.pixel_clock += pixelClockRange;
        /* some cards need wider virtual widths for certain modes */
        high.virtual_width = 2048;
        /* do it once for each depth we want to support */
        for (j = 0; j < (sizeof(spaces) / sizeof(color_space)); j++) {
            /* set target values */
            *dst = *src;
            /* poke the specific space */
            dst->space = low.space = high.space = spaces[j];
            /* ask for a compatible mode */
            if (PROPOSE_DISPLAY_MODE(dst, &low, &high) != B_ERROR) {
                /* count it, and move on to next mode */
                dst++;
                si->modesNum++;
            }
        }
        /* advance to next mode */
        src++;
    }

    return B_OK;
}
/*****************************************************************************/

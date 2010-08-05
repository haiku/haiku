/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#include <string.h>
#include <SupportDefs.h>
#include "GlobalData.h"

#define MODE_FLAGS (B_8_BIT_DAC|B_PARALLEL_ACCESS)
#define MODE_COUNT (sizeof(kModeList) / sizeof(display_mode))

static const display_mode kModeList[] =
{ { { 19660,  640,  672,  744,  776,  480,  490,  494,  505,  0 }, B_RGB32_LITTLE, 640,  480,  0, 0, MODE_FLAGS }, /* 640x480@50Hz */
  { { 19660,  640,  672,  744,  776,  480,  490,  494,  505,  0 }, B_CMAP8,	   640,  480,  0, 0, MODE_FLAGS }, /* 640x480@50Hz */
  { { 25250,  800,  832,  920,  952,  500,  511,  515,  526,  0 }, B_RGB32_LITTLE, 800,  500,  0, 0, MODE_FLAGS }, /* 800x500@50Hz */
  { { 30970,  800,  832,  944,  976,  600,  613,  618,  631,  0 }, B_RGB32_LITTLE, 800,  600,  0, 0, MODE_FLAGS }, /* 800x600@50Hz */
  { { 41980,  1024, 1056, 1208, 1240, 640,  653,  659,  673,  0 }, B_RGB32_LITTLE, 1024, 640,  0, 0, MODE_FLAGS }, /* 1024x640@50Hz */
  { { 51850,  1024, 1056, 1248, 1280, 768,  784,  791,  807,  0 }, B_RGB32_LITTLE, 1024, 768,  0, 0, MODE_FLAGS }, /* 1024x768@50Hz */
  { { 59420,  1280, 1312, 1536, 1568, 720,  735,  741,  757,  0 }, B_RGB32_LITTLE, 1280, 720,  0, 0, MODE_FLAGS }, /* 1280x720@50Hz */
  { { 64050,  1280, 1312, 1552, 1584, 768,  784,  791,  807,  0 }, B_RGB32_LITTLE, 1280, 768,  0, 0, MODE_FLAGS }, /* 1280x768@50Hz */
  { { 65740,  1152, 1184, 1432, 1464, 854,  872,  879,  897,  0 }, B_RGB32_LITTLE, 1152, 854,  0, 0, MODE_FLAGS }, /* 1152x854@50Hz */
  { { 67260,  1280, 1312, 1560, 1592, 800,  817,  824,  841,  0 }, B_RGB32_LITTLE, 1280, 800,  0, 0, MODE_FLAGS }, /* 1280x800@50Hz */
  { { 86730,  1440, 1472, 1800, 1832, 900,  919,  927,  946,  0 }, B_RGB32_LITTLE, 1440, 900,  0, 0, MODE_FLAGS }, /* 1440x900@50Hz */
  { { 90890,  1280, 1312, 1656, 1688, 1024, 1045, 1054, 1076, 0 }, B_RGB32_LITTLE, 1280, 1024, 0, 0, MODE_FLAGS }, /* 1280x1024@50Hz */
  { { 102150, 1400, 1432, 1816, 1848, 1050, 1072, 1081, 1103, 0 }, B_RGB32_LITTLE, 1400, 1050, 0, 0, MODE_FLAGS }, /* 1400x1050@50Hz */
  { { 121680, 1680, 1712, 2168, 2200, 1050, 1072, 1081, 1103, 0 }, B_RGB32_LITTLE, 1680, 1050, 0, 0, MODE_FLAGS }, /* 1680x1050@50Hz */
  { { 137970, 1600, 1632, 2152, 2184, 1200, 1225, 1235, 1261, 0 }, B_RGB32_LITTLE, 1600, 1200, 0, 0, MODE_FLAGS }, /* 1600x1200@50Hz */
  { { 164500, 1920, 1952, 2576, 2608, 1200, 1225, 1235, 1261, 0 }, B_RGB32_LITTLE, 1920, 1200, 0, 0, MODE_FLAGS }, /* 1920x1200@50Hz */
  { { 179080, 1792, 1824, 2504, 2536, 1344, 1372, 1383, 1412, 0 }, B_RGB32_LITTLE, 1792, 1344, 0, 0, MODE_FLAGS }, /* 1792x1344@50Hz */
  { { 194330, 1856, 1888, 2624, 2656, 1392, 1421, 1432, 1462, 0 }, B_RGB32_LITTLE, 1856, 1392, 0, 0, MODE_FLAGS }, /* 1856x1392@50Hz */
  { { 210640, 1920, 1952, 2752, 2784, 1440, 1470, 1482, 1513, 0 }, B_RGB32_LITTLE, 1920, 1440, 0, 0, MODE_FLAGS }, /* 1920x1440@50Hz */
  { { 245600, 2048, 2080, 3008, 3040, 1536, 1568, 1581, 1613, 0 }, B_RGB32_LITTLE, 2048, 1536, 0, 0, MODE_FLAGS }, /* 2048x1536@50Hz */
};


status_t
PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low,
	const display_mode *high)
{
	TRACE("PROPOSE_DISPLAY_MODE\n");

	/* Arbitrary - I don't know if VMware really has a lower limit */
	if (target->virtual_width < 320 || target->virtual_height < 240)
		return B_ERROR;

	if (target->virtual_width > gSi->maxWidth ||
		target->virtual_height > gSi->maxHeight)
		return B_ERROR;

	/* Any such mode is OK, even if it isn't in our propose list */
	return B_OK;
}


uint32
ACCELERANT_MODE_COUNT()
{
	TRACE("ACCELERANT_MODE_COUNT: %"B_PRIuSIZE"\n", MODE_COUNT);
	return MODE_COUNT;
}


status_t
GET_MODE_LIST(display_mode * dm)
{
	TRACE("GET_MODE_LIST\n");
	memcpy(dm, kModeList, sizeof(kModeList));
	return B_OK;
}


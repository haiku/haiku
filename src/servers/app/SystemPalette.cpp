/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stefano Ceccherini (burton666@libero.it)
 */

//! Methods to initialize and get the system color_map.	


#include "SystemPalette.h"

#include <stdio.h>
#include <string.h>


const static rgb_color kSystemPalette[] = {
	{   0,   0,   0, 255 }, {   8,   8,   8, 255 }, {  16,  16,  16, 255 },
	{  24,  24,  24, 255 }, {  32,  32,  32, 255 }, {  40,  40,  40, 255 },
	{  48,  48,  48, 255 }, {  56,  56,  56, 255 }, {  64,  64,  64, 255 },
	{  72,  72,  72, 255 }, {  80,  80,  80, 255 }, {  88,  88,  88, 255 },
	{  96,  96,  96, 255 }, { 104, 104, 104, 255 }, { 112, 112, 112, 255 },
	{ 120, 120, 120, 255 }, { 128, 128, 128, 255 }, { 136, 136, 136, 255 },
	{ 144, 144, 144, 255 }, { 152, 152, 152, 255 }, { 160, 160, 160, 255 },
	{ 168, 168, 168, 255 }, { 176, 176, 176, 255 }, { 184, 184, 184, 255 },
	{ 192, 192, 192, 255 }, { 200, 200, 200, 255 }, { 208, 208, 208, 255 },
	{ 216, 216, 216, 255 }, { 224, 224, 224, 255 }, { 232, 232, 232, 255 },
	{ 240, 240, 240, 255 }, { 248, 248, 248, 255 }, {   0,   0, 255, 255 },
	{   0,   0, 229, 255 }, {   0,   0, 204, 255 }, {   0,   0, 179, 255 },
	{   0,   0, 154, 255 }, {   0,   0, 129, 255 }, {   0,   0, 105, 255 },
	{   0,   0,  80, 255 }, {   0,   0,  55, 255 }, {   0,   0,  30, 255 },
	{ 255,   0,   0, 255 }, { 228,   0,   0, 255 }, { 203,   0,   0, 255 },
	{ 178,   0,   0, 255 }, { 153,   0,   0, 255 }, { 128,   0,   0, 255 },
	{ 105,   0,   0, 255 }, {  80,   0,   0, 255 }, {  55,   0,   0, 255 },
	{  30,   0,   0, 255 }, {   0, 255,   0, 255 }, {   0, 228,   0, 255 },
	{   0, 203,   0, 255 }, {   0, 178,   0, 255 }, {   0, 153,   0, 255 },
	{   0, 128,   0, 255 }, {   0, 105,   0, 255 }, {   0,  80,   0, 255 },
	{   0,  55,   0, 255 }, {   0,  30,   0, 255 }, {   0, 152,  51, 255 },
	{ 255, 255, 255, 255 }, { 203, 255, 255, 255 }, { 203, 255, 203, 255 },
	{ 203, 255, 152, 255 }, { 203, 255, 102, 255 }, { 203, 255,  51, 255 },
	{ 203, 255,   0, 255 }, { 152, 255, 255, 255 }, { 152, 255, 203, 255 },
	{ 152, 255, 152, 255 }, { 152, 255, 102, 255 }, { 152, 255,  51, 255 },
	{ 152, 255,   0, 255 }, { 102, 255, 255, 255 }, { 102, 255, 203, 255 },
	{ 102, 255, 152, 255 }, { 102, 255, 102, 255 }, { 102, 255,  51, 255 },
	{ 102, 255,   0, 255 }, {  51, 255, 255, 255 }, {  51, 255, 203, 255 },
	{  51, 255, 152, 255 }, {  51, 255, 102, 255 }, {  51, 255,  51, 255 },
	{  51, 255,   0, 255 }, { 255, 152, 255, 255 }, { 255, 152, 203, 255 },
	{ 255, 152, 152, 255 }, { 255, 152, 102, 255 }, { 255, 152,  51, 255 },
	{ 255, 152,   0, 255 }, {   0, 102, 255, 255 }, {   0, 102, 203, 255 },
	{ 203, 203, 255, 255 }, { 203, 203, 203, 255 }, { 203, 203, 152, 255 },
	{ 203, 203, 102, 255 }, { 203, 203,  51, 255 }, { 203, 203,   0, 255 },
	{ 152, 203, 255, 255 }, { 152, 203, 203, 255 }, { 152, 203, 152, 255 },
	{ 152, 203, 102, 255 }, { 152, 203,  51, 255 }, { 152, 203,   0, 255 },
	{ 102, 203, 255, 255 }, { 102, 203, 203, 255 }, { 102, 203, 152, 255 },
	{ 102, 203, 102, 255 }, { 102, 203,  51, 255 }, { 102, 203,   0, 255 },
	{  51, 203, 255, 255 }, {  51, 203, 203, 255 }, {  51, 203, 152, 255 },
	{  51, 203, 102, 255 }, {  51, 203,  51, 255 }, {  51, 203,   0, 255 },
	{ 255, 102, 255, 255 }, { 255, 102, 203, 255 }, { 255, 102, 152, 255 },
	{ 255, 102, 102, 255 }, { 255, 102,  51, 255 }, { 255, 102,   0, 255 },
	{   0, 102, 152, 255 }, {   0, 102, 102, 255 }, { 203, 152, 255, 255 },
	{ 203, 152, 203, 255 }, { 203, 152, 152, 255 }, { 203, 152, 102, 255 },
	{ 203, 152,  51, 255 }, { 203, 152,   0, 255 }, { 152, 152, 255, 255 },
	{ 152, 152, 203, 255 }, { 152, 152, 152, 255 }, { 152, 152, 102, 255 },
	{ 152, 152,  51, 255 }, { 152, 152,   0, 255 }, { 102, 152, 255, 255 },
	{ 102, 152, 203, 255 }, { 102, 152, 152, 255 }, { 102, 152, 102, 255 },
	{ 102, 152,  51, 255 }, { 102, 152,   0, 255 }, {  51, 152, 255, 255 },
	{  51, 152, 203, 255 }, {  51, 152, 152, 255 }, {  51, 152, 102, 255 },
	{  51, 152,  51, 255 }, {  51, 152,   0, 255 }, { 230, 134,   0, 255 },
	{ 255,  51, 203, 255 }, { 255,  51, 152, 255 }, { 255,  51, 102, 255 },
	{ 255,  51,  51, 255 }, { 255,  51,   0, 255 }, {   0, 102,  51, 255 },
	{   0, 102,   0, 255 }, { 203, 102, 255, 255 }, { 203, 102, 203, 255 },
	{ 203, 102, 152, 255 }, { 203, 102, 102, 255 }, { 203, 102,  51, 255 },
	{ 203, 102,   0, 255 }, { 152, 102, 255, 255 }, { 152, 102, 203, 255 },
	{ 152, 102, 152, 255 }, { 152, 102, 102, 255 }, { 152, 102,  51, 255 },
	{ 152, 102,   0, 255 }, { 102, 102, 255, 255 }, { 102, 102, 203, 255 },
	{ 102, 102, 152, 255 }, { 102, 102, 102, 255 }, { 102, 102,  51, 255 },
	{ 102, 102,   0, 255 }, {  51, 102, 255, 255 }, {  51, 102, 203, 255 },
	{  51, 102, 152, 255 }, {  51, 102, 102, 255 }, {  51, 102,  51, 255 },
	{  51, 102,   0, 255 }, { 255,   0, 255, 255 }, { 255,   0, 203, 255 },
	{ 255,   0, 152, 255 }, { 255,   0, 102, 255 }, { 255,   0,  51, 255 },
	{ 255, 175,  19, 255 }, {   0,  51, 255, 255 }, {   0,  51, 203, 255 },
	{ 203,  51, 255, 255 }, { 203,  51, 203, 255 }, { 203,  51, 152, 255 },
	{ 203,  51, 102, 255 }, { 203,  51,  51, 255 }, { 203,  51,   0, 255 },
	{ 152,  51, 255, 255 }, { 152,  51, 203, 255 }, { 152,  51, 152, 255 },
	{ 152,  51, 102, 255 }, { 152,  51,  51, 255 }, { 152,  51,   0, 255 },
	{ 102,  51, 255, 255 }, { 102,  51, 203, 255 }, { 102,  51, 152, 255 },
	{ 102,  51, 102, 255 }, { 102,  51,  51, 255 }, { 102,  51,   0, 255 },
	{  51,  51, 255, 255 }, {  51,  51, 203, 255 }, {  51,  51, 152, 255 },
	{  51,  51, 102, 255 }, {  51,  51,  51, 255 }, {  51,  51,   0, 255 },
	{ 255, 203, 102, 255 }, { 255, 203, 152, 255 }, { 255, 203, 203, 255 },
	{ 255, 203, 255, 255 }, {   0,  51, 152, 255 }, {   0,  51, 102, 255 },
	{   0,  51,  51, 255 }, {   0,  51,   0, 255 }, { 203,   0, 255, 255 },
	{ 203,   0, 203, 255 }, { 203,   0, 152, 255 }, { 203,   0, 102, 255 },
	{ 203,   0,  51, 255 }, { 255, 227,  70, 255 }, { 152,   0, 255, 255 },
	{ 152,   0, 203, 255 }, { 152,   0, 152, 255 }, { 152,   0, 102, 255 },
	{ 152,   0,  51, 255 }, { 152,   0,   0, 255 }, { 102,   0, 255, 255 },
	{ 102,   0, 203, 255 }, { 102,   0, 152, 255 }, { 102,   0, 102, 255 },
	{ 102,   0,  51, 255 }, { 102,   0,   0, 255 }, {  51,   0, 255, 255 },
	{  51,   0, 203, 255 }, {  51,   0, 152, 255 }, {  51,   0, 102, 255 },
	{  51,   0,  51, 255 }, {  51,   0,   0, 255 }, { 255, 203,  51, 255 },
	{ 255, 203,   0, 255 }, { 255, 255,   0, 255 }, { 255, 255,  51, 255 },
	{ 255, 255, 102, 255 }, { 255, 255, 152, 255 }, { 255, 255, 203, 255 },
	{ 255, 255, 255, 0 } // B_TRANSPARENT_MAGIC_CMAP8
};


// TODO: BWindowScreen has a method to set the palette.
// maybe we should have a lock to protect this variable.
static color_map sColorMap;


// color_distance
/*!	\brief Returns the "distance" between two RGB colors.

	This functions defines an metric on the RGB color space. The distance
	between two colors is 0, if and only if the colors are equal.

	\param red1 Red component of the first color.
	\param green1 Green component of the first color.
	\param blue1 Blue component of the first color.
	\param red2 Red component of the second color.
	\param green2 Green component of the second color.
	\param blue2 Blue component of the second color.
	\return The distance between the given colors.
*/
static inline uint32
color_distance(uint8 red1, uint8 green1, uint8 blue1,
			   uint8 red2, uint8 green2, uint8 blue2)
{
	int rd = (int)red1 - (int)red2;
	int gd = (int)green1 - (int)green2;
	int bd = (int)blue1 - (int)blue2;

	// distance according to psycho-visual tests
	// algorithm taken from here:
	// http://www.stud.uni-hannover.de/~michaelt/juggle/Algorithms.pdf
	int rmean = ((int)red1 + (int)red2) / 2;
	return (((512 + rmean) * rd * rd) >> 8)
			+ 4 * gd * gd
			+ (((767 - rmean) * bd * bd) >> 8); 
}


static inline uint8
FindClosestColor(const rgb_color &color, const rgb_color *palette)
{
	uint8 closestIndex = 0;
	unsigned closestDistance = UINT_MAX;
	for (int32 i = 0; i < 256; i++) {
		const rgb_color &c = palette[i];
		unsigned distance = color_distance(color.red, color.green, color.blue,
										   c.red, c.green, c.blue);
		if (distance < closestDistance) {
			closestIndex = (uint8)i;
			closestDistance = distance;
		}
	}
	return closestIndex;
}


static inline rgb_color
InvertColor(const rgb_color &color)
{
	// For some reason, Inverting (255, 255, 255) on beos
	// results in the same color.
	if (color.red == 255 && color.green == 255
		&& color.blue == 255)
		return color;
	
	rgb_color inverted;
	inverted.red = 255 - color.red;
	inverted.green = 255 - color.green;
	inverted.blue = 255 - color.blue;
	inverted.alpha = 255;
	
	return inverted;
}


static void
FillColorMap(const rgb_color *palette, color_map *map)
{
	memcpy(map->color_list, palette, sizeof(map->color_list));
	
	// init index map
	for (int32 color = 0; color < 32768; color++) {
		// get components
		rgb_color rgbColor;
		rgbColor.red = (color & 0x7c00) >> 7;
		rgbColor.green = (color & 0x3e0) >> 2;
		rgbColor.blue = (color & 0x1f) << 3;
		
		map->index_map[color] = FindClosestColor(rgbColor, palette);
	}
	
	// init inversion map
	for (int32 index = 0; index < 256; index++) {
		rgb_color inverted = InvertColor(map->color_list[index]);
		map->inversion_map[index] = FindClosestColor(inverted, palette);	
	}
}


/*!	\brief Initializes the system color_map.
*/
void
InitializeColorMap()
{
	FillColorMap(kSystemPalette, &sColorMap);
}


/*!	\brief Returns a pointer to the system palette.
	\return A pointer to the system palette.
*/
const rgb_color *
SystemPalette()
{
	return sColorMap.color_list;
}


/*!	\brief Returns a pointer to the system color_map structure.
	\return A pointer to the system color_map.
*/
const color_map *
SystemColorMap()
{
	return &sColorMap;
}

/*

	ChartRender.c

	by Pierre Raynaud-Richard.

	Copyright 1998 Be Incorporated, All Rights Reserved.

*/

/* This file has been designed to be easy to compile as a stand-alone
   piece of code, allowing you to use advanced intel compiler, even
   if they are compatible with the whole Be environment. To accomplish
   that purpose, all declarations were concentrated in ChartRender.h
   (see that header file for more infos). */
#include "ChartRender.h"

/* This table provide the horizontal and vertical offset of the matrix
   of pixel used for drawing stars. This matrix is designed as follow:

    --  [00] [01] [02] [03]  --
   [04] [05] [06] [07] [08] [09]
   [10] [11] [12] [13] [14] [15]
   [16] [17] [18] [19] [20] [21]
   [22] [23] [24] [25] [26] [27]
    --  [28] [29] [30] [31]  --

   The reference pixel is [12]. */
int8	pattern_dh[32] = {
		-1, 0, 1, 2,
	-2, -1, 0, 1, 2, 3,
	-2, -1, 0, 1, 2, 3,
	-2, -1, 0, 1, 2, 3,
	-2, -1, 0, 1, 2, 3,
		-1, 0, 1, 2
};

int8	pattern_dv[32] = {
		-2, -2, -2, -2,
	-1, -1, -1, -1, -1, -1,
	0,	0,	0,	0,	0,	0,
	1,	1,	1,	1,	1,	1,
	2,	2,	2,	2,	2,	2,
		3,	3,	3,	3
};

/* Those table contains a preprocessed version of the 32 size of star,
   represented in the 32 pixels matrix, by alpha-blending density [0 to 7].
   Those matrix are stored in packed format, as a the list of all pixel
   whose alpha-blending density is > 0. There is 4 cases because every
   star can be aligned at half a pixel in both direction (we implement
   sub-pixel precision and anti-aliasing to reduce the jittering). */
static	uint8	pattern_list[4*LEVEL_COUNT][32];
static	uint8	pattern_list_count[4*LEVEL_COUNT];
static	uint8	pattern_color_offset[4*LEVEL_COUNT][32];
/* this table store the alpha-blending level of the center pixel. This
   is used for size so small that only the center pixel is lighted. */
static	uint8	pixel_color_offset[LEVEL_COUNT];

/* Those mask are use for fast clipping, to determine which of the 32
   pixels of the standard star matrix are visible when coming closer
   from a left, right, top or bottom clipping border. */
static uint32	visible_mask_left[6] = {
	0xffffffff,
	0xffbefbef,
	0xef3cf3ce,
	0xce38e38c,
	0x8c30c308,
	0x08208200
};

static uint32	visible_mask_right[6] = {
	0xffffffff,
	0xf7df7dff,
	0x73cf3cf7,
	0x31c71c73,
	0x10c30c31,
	0x00410410,
};

static uint32	visible_mask_top[6] = {
	0xffffffff,
	0xfffffff0,
	0xfffffc00,
	0xffff0000,
	0xffc00000,
	0xf0000000
};

static uint32	visible_mask_bottom[6] = {
	0xffffffff,
	0x0fffffff,
	0x003fffff,
	0x0000ffff,
	0x000003ff,
	0x0000000f
};

/* Private functions used only internally. */
bool		ProjectStar(star *s, geometry *geo);
bool		CheckClipping(star *s, buffer *buf, bool reset_clipping);
void		DrawStar(star *s, buffer *buf);
void		EraseStar(star *s, buffer *buf);


/* This function initialise the 32 sizes of anti-aliased star, each one
   represented in 4 different half-pixel alignement :
   x : -0.25, y : -0.25
   x : +0.25, y : -0.25
   x : -0.25, y : +0.25
   x : +0.25, y : +0.25 */
void InitPatterns()
{
	int32		i, j, k, count;
	float		radius, x0, y0, x, y, dist, delta;
	uint8		color;
	uint8		*list, *color_offset;

	/* do the 4 half-pixel alignement */
	for (j=0; j<4; j++) {
		if (j&1) x0 = 1.25;
		else	 x0 = 0.75;
		if (j&2) y0 = 1.25;
		else	 y0 = 0.75;

		/* do the 32 sizes */
		for (i=0; i<LEVEL_COUNT; i++) {
			radius = (float)(i+1) * (2.8/(float)LEVEL_COUNT);
			count = 0;
			list = pattern_list[j*LEVEL_COUNT + i];
			color_offset = pattern_color_offset[j*LEVEL_COUNT + i];

			/* scan the 32 pixels of the matrix */
			for (k=0; k<32; k++) {
				x = ((float)pattern_dh[k] + ROUNDING) - x0;
				y = ((float)pattern_dv[k] + ROUNDING) - y0;

				dist = sqrt(x*x + y*y);
				/* process non source pixel */
				if (dist > 0.5) {
					delta = radius - dist + 0.5;
					if (delta >= 1.0) {
						*color_offset++ = 7;
						*list++ = k;
						count++;
					}
					else if (delta > 0.5) {
						*color_offset++ = (uint8)(7.499 - 16.0 * (1.0 - delta) * (1.0 - delta) + ROUNDING);
						*list++ = k;
						count++;
					}
					else if (delta > 0) {
						color = (uint8)(16.0 * delta * delta);
						if (color > 0) {
							*color_offset++ = color;
							*list++ = k;
							count++;
						}
					}
				}
				/* process source pixel (the one containing the center of the star) */
				else {
					if (radius < 0.25) {
						color = (uint8)(32.0 * radius * radius + ROUNDING);
						if (color == 0)
							color++;
					}
					else if (radius < 0.75) {
						delta = radius + 0.25;
						color = (uint8)(7.499 - 22.0 * (1.0 - delta) * (1.0 - delta) + ROUNDING);
					}
					else
						color = 7;
					*color_offset++ = color;
					*list++ = k;
					count++;
					pixel_color_offset[i] = color;
				}
			}
			pattern_list_count[j*LEVEL_COUNT + i] = count;
		}
	}
}

/* Project a star (s) in the view space of the camera, as described by (geo).
   Returns true if the star seems to be visible (in the pyramid of vision,
   closer than the rear plan, farther than the front plan), or false if it's
   clear that the star isnot visible. */
bool ProjectStar(star *s, geometry *geo)
{
	int32		h_double, v_double, level;
	float		x0, y0, z0, x, y, z, inv_z;

	/* Calculate the coordinate of the star after doing the cycling operation
	   that convert the cube of the starfield in a torus. This ensure that
	   get the copy of the star that is the only one likely to be visible from
	   the camera. */
	x0 = s->x;
	if (x0 < geo->cutx)
		x0 += 1.0;
	y0 = s->y;
	if (y0 < geo->cuty)
		y0 += 1.0;
	z0 = s->z;
	if (z0 < geo->cutz)
		z0 += 1.0;
	/* Translate the star relative to the position of the camera. */
	x0 -= geo->x;
	y0 -= geo->y;
	z0 -= geo->z;

	/* Calculate the z coordinate (depth) of the star in the camera referential. */
	z = geo->m[0][2]*x0 + geo->m[1][2]*y0 + geo->m[2][2]*z0;

	/* Do the rear and front plan clipping */
	if ((z < geo->z_min) || (z > geo->z_max))
		return false;

	/* Calculate the x coordinate (horizontal) of the star in the camera referential. */
	x = geo->m[0][0]*x0 + geo->m[1][0]*y0 + geo->m[2][0]*z0;

	/* Do the left and right clipping based on the pyramid of vision. */
	if ((x < geo->xz_min*z-BORDER_CLIPPING) || (x > geo->xz_max*z+BORDER_CLIPPING))
		return false;

	/* Calculate the y coordinate (vertical) of the star in the camera referential. */
	y = geo->m[0][1]*x0 + geo->m[1][1]*y0 + geo->m[2][1]*z0;

	/* Do the top and bottom clipping based on the pyramid of vision. */
	if ((y < geo->yz_min*z-BORDER_CLIPPING) || (y > geo->yz_max*z+BORDER_CLIPPING))
		return false;

	/* Calculate the invert of z, used to project both H and V coordinate. Apply
	   the zoom-factor at the same time. The zoom-factor was overscale by a factor
	   of two in advance, for the half-pixel precision processing */
	inv_z = geo->zoom_factor/z;

	/* Calculate the double pixel coordinate in the buffer (in half-pixel). */
	h_double = (int32)(x * inv_z + geo->offset_h);
	v_double = (int32)(y * inv_z + geo->offset_v);

	/* Calculate the light level of the star. We use that little weird function
	   to a get faster gradient to black near the rear plan. */
	level = (int32)(s->size * (inv_z * geo->z_max_square - z * geo->zoom_factor)) >> 8;
	/* The light level can go higher that our max (saturation). */
	if (level >= LEVEL_COUNT)
		level = LEVEL_COUNT-1;

	/* Get the real pixel coordinate in the buffer from the double coordinates */
	s->h = h_double >> 1;
	s->v = v_double >> 1;
	/* Save the light level (used to recognize single pixel star) */
	s->level = level;
	/* switch between the 4 pattern table use for the 4 half-aligned. */
	if ((h_double & 1) == 1) level += LEVEL_COUNT;
	if ((v_double & 1) == 1) level += 2*LEVEL_COUNT;
	s->pattern_level = level;
	return true;
}

/* Once a star has been projected (using ProjectStar), we need to determine
   which pixel of the star matrix are visible (if any). This depend of the
   clipping of the specific buffer you're using. This function will do that
   for the star (s), in the buffer (buf). It will return false if the star
   is fully invisible, true if not. The flag reset_clipping is used to
   reprocess the clipping from scratch, or to just cumulate the new clipping
   to the last drawing clipping (this is needed when updating the clipping
   of every stars after changing the clipping region of the buffer). */
bool CheckClipping(star *s, buffer *buf, bool reset_clipping)
{
	int32			delta;
	uint32			i, total_visible, tmp_visible;
	clipping_rect	box;
	clipping_rect	*r;

	/* Simple case : the star is represented by only one pixel. */
	if (pattern_list_count[s->pattern_level] == 1) {
		/* if the pixel is not in the bounding box of the clipping region,
		   the star is guarantee to be invisible. */
		if ((s->h < buf->clip_bounds.left) ||
			(s->h > buf->clip_bounds.right) ||
			(s->v < buf->clip_bounds.top) ||
			(s->v > buf->clip_bounds.bottom))
			goto invisible;
		/* if the clipping region contains only one rectangle, then it's
		   equal to its bounding box, so no further test are needed. */
		if (buf->clip_list_count == 1)
			goto visible;
		/* In the other case, we need to go through the list of rectangle
		   of the clipping region and check if the pixel is in any of those */
		r = buf->clip_list;
		for (i=0; i<buf->clip_list_count; r++, i++)
			if ((s->h >= r->left) &&
				(s->h <= r->right) &&
				(s->v >= r->top) &&
				(s->v <= r->bottom))
				goto visible;
		/* The pixel is not visible. The star is marked as not drawn. */
	invisible:
		s->last_draw_offset = INVALID;
		return false;
	visible:
		/* The pixel is visible. The offset at which the star should be draw is
		   calculated and store for using by drawing (and erasing later). */
		s->last_draw_offset = s->v * buf->bytes_per_row + s->h * buf->bytes_per_pixel;
		return true;
	}
	/* Complex case : the star is represented by more than one pixel. */
	else {
		/* Calculate the box the bounding box of the matrix of 32 pixels used
		   to represent the star, called box. */
		box.left = s->h - 2;
		box.right = s->h + 3;
		box.top = s->v - 2;
		box.bottom = s->v + 3;

		/* Check if the box is fully outside of the bounding box of the clipping
		   region. That woudl guarantee that the star is invisible. */
		if ((box.right < buf->clip_bounds.left) ||
			(box.left > buf->clip_bounds.right) ||
			(box.bottom < buf->clip_bounds.top) ||
			(box.top > buf->clip_bounds.bottom))
			goto invisible_pat;

		/* Now, we have to go through the list of rectangle of the clipping region
		   and cumulate the mask of the star matrix pixels that are visible in any
		   of those rectangle. At start time, the mask is empty. */
		total_visible = 0;
		r = buf->clip_list;
		for (i=0; i<buf->clip_list_count; r++, i++) {
			/* When reseting the clipping, all pixel of the matrix are tested. In
			   the other mode, only the pixel previously visible are tested (as we
			   want to know which one of the previously drawn pixel still need to
			   be erased. */
			if (reset_clipping)
				tmp_visible = 0xffffffff;
			else
				tmp_visible = s->last_draw_pattern;

			/* Calculate the clipping on the left side of the rectangle. */
			delta = r->left-box.left;
			if (delta > 5)
				continue;
			if (delta > 0)
				tmp_visible &= visible_mask_left[delta];

			/* Calculate the clipping on the right side of the rectangle. */
			delta = box.right-r->right;
			if (delta > 5)
				continue;
			if (delta > 0)
				tmp_visible &= visible_mask_right[delta];

			/* Calculate the clipping on the top side of the rectangle. */
			delta = r->top-box.top;
			if (delta > 5)
				continue;
			if (delta > 0)
				tmp_visible &= visible_mask_top[delta];

			/* Calculate the clipping on the bottom side of the rectangle. */
			delta = box.bottom-r->bottom;
			if (delta > 5)
				continue;
			if (delta > 0)
				tmp_visible &= visible_mask_bottom[delta];

			/* Pixel of the matrix not clipped out at that point are visible
			   inside this rectangle of the clipping region. We need to add
			   them to the mask of currently known visible pixel. */
			total_visible |= tmp_visible;
			/* If all pixel of the matrix are already visible, no need to continue
			   further. */
			if (total_visible == 0xffffffff)
				goto visible_pat;
		}
		/* If no pixel are visible, then we know... */
		if (total_visible != 0)
			goto visible_pat;

		/* The star is not visible. It's marked as not drawn. */
	invisible_pat:
		s->last_draw_offset = INVALID;
		return false;
	visible_pat:
		/* The star is partially visible. The offset at which the star should be
		   draw is calculated and store for using by drawing (and erasing later).
		   The mask of which pixel of the matrix are visible is store for use
		   at drawing and erasing time. */
		s->last_draw_offset = s->v * buf->bytes_per_row + s->h * buf->bytes_per_pixel;
		s->last_draw_pattern = total_visible;
		return true;
	}
}

/* After calling ProjectStar and CheckClipping, we're finally ready to
   draw the star in its destination buffer. So let's do it... */
void DrawStar(star *s, buffer *buf)
{
	int32		i, index, count;
	uint8		*draw8;
	uint16		*draw16;
	uint32		*draw32;
	uint32		*colors;
	uint8		*pat_list;
	uint8		*pat_color_offset;

	/* Simple case : the star is represented by only one pixel. */
	count = pattern_list_count[s->pattern_level];
	if (count == 1) {
		/* Depending the depth mode of the drawing buffer... */
		switch (buf->depth_mode) {
		case PIXEL_1_BYTE :
			/* Get the pointer to the address we want to draw to... */
			draw8 = (uint8*)((char*)buf->bits + s->last_draw_offset);
			/* ... and write the color pattern we want to use depending of
			   the lighting level and the color scheme of the star. */
			*draw8 = buf->colors[s->color_type][pixel_color_offset[s->level]];
			break;
		case PIXEL_2_BYTES :
			/* Same thing for 2 bytes mode */
			draw16 = (uint16*)((char*)buf->bits + s->last_draw_offset);
			*draw16 = buf->colors[s->color_type][pixel_color_offset[s->level]];
			break;
		case PIXEL_4_BYTES :
			/* Same thing for 4 bytes mode */
			draw32 = (uint32*)((char*)buf->bits + s->last_draw_offset);
			*draw32 = buf->colors[s->color_type][pixel_color_offset[s->level]];
			break;
		}
	}
	/* Complex case : the star is represented by a multiple pixels. */
	else {
		/* Pointer to the color table used depending the color scheme of
		   the star. */
		colors = buf->colors[s->color_type];
		pat_list = pattern_list[s->pattern_level];
		pat_color_offset = pattern_color_offset[s->pattern_level];

		/* Plot all pixel used to represent the star one after one... */
		for (i=0; i<count; i++) {
			/* This is the index of the pixel in the matrix */
			index = pat_list[i];
			/* Check if this pixel is visible (using the result of the clipping) */
			if (s->last_draw_pattern & (1<<index)) {
				switch (buf->depth_mode) {
				case PIXEL_1_BYTE :
					/* Get the pointer to the address we want to draw to... */
					draw8 = (uint8*)((char*)buf->pattern_bits[index] + s->last_draw_offset);
					/* ... and write the color pattern we want to use depending of
					   the lighting level and the color scheme of the star. */
					*draw8 = colors[pat_color_offset[i]];
					break;
				case PIXEL_2_BYTES :
					/* Same thing for 2 bytes mode */
					draw16 = (uint16*)((char*)buf->pattern_bits[index] + s->last_draw_offset);
					*draw16 = colors[pat_color_offset[i]];
					break;
				case PIXEL_4_BYTES :
					/* Same thing for 4 bytes mode */
					draw32 = (uint32*)((char*)buf->pattern_bits[index] + s->last_draw_offset);
					*draw32 = colors[pat_color_offset[i]];
					break;
				}
			}
		}
	}
}

/* Before redrawing a star at its new position, we need to erase what we draw
   at the previous frame... */
void EraseStar(star *s, buffer *buf)
{
	int32		i, index, count;
	uint8		*draw8;
	uint16		*draw16;
	uint32		*draw32;
	uint32		back_color;
	uint8		*pat_list;

	/* Color pattern we use to erase the buffer. */
	back_color = buf->back_color;

	/* Simple case : the star is represented by only one pixel. */
	count = pattern_list_count[s->pattern_level];
	if (count == 1) {
		/* Depending the depth mode of the drawing buffer... */
		switch (buf->depth_mode) {
		case PIXEL_1_BYTE :
			/* Get the pointer to the address we want to erase... */
			draw8 = (uint8*)((char*)buf->bits + s->last_draw_offset);
			/* ... and write the background color pattern. */
			*draw8 = back_color;
			break;
		case PIXEL_2_BYTES :
			/* Same thing for 2 bytes mode */
			draw16 = (uint16*)((char*)buf->bits + s->last_draw_offset);
			*draw16 = back_color;
			break;
		case PIXEL_4_BYTES :
			/* Same thing for 4 bytes mode */
			draw32 = (uint32*)((char*)buf->bits + s->last_draw_offset);
			*draw32 = back_color;
			break;
		}
	}
	/* Complex case : the star is represented by a multiple pixels. */
	else {
		pat_list = pattern_list[s->pattern_level];

		/* Erase all pixel used to represent the star one after one... */
		for (i=0; i<count; i++) {
			index = pat_list[i];
			/* Check if this pixel is visible (using the result of the clipping) */
			if (s->last_draw_pattern & (1<<index)) {
				switch (buf->depth_mode) {
				case PIXEL_1_BYTE :
					/* Get the pointer to the address we want to draw to... */
					draw8 = (uint8*)((char*)buf->pattern_bits[index] + s->last_draw_offset);
					/* ... and write the background color pattern. */
					*draw8 = back_color;
					break;
				case PIXEL_2_BYTES :
					/* Same thing for 2 bytes mode */
					draw16 = (uint16*)((char*)buf->pattern_bits[index] + s->last_draw_offset);
					*draw16 = back_color;
					break;
				case PIXEL_4_BYTES :
					/* Same thing for 4 bytes mode */
					draw32 = (uint32*)((char*)buf->pattern_bits[index] + s->last_draw_offset);
					*draw32 = back_color;
					break;
				}
			}
		}
	}
}

/* This function do the transition from previous state to the new state
   as described in (geo), in the buffer (buf), for the list of star (sp) */
void RefreshStarPacket(buffer *buf, star_packet *sp, geometry *geo)
{
	int32			i, min_count;
	star			*s;

	// TODO: For some reason, when selecting the "2 threads" option under vmware,
	// some weird timing calculations finish with setting the star packet count to
	// a negative number. This screws all the next calculations, and the animation
	// then comes to a stop.
	sp->count = max_c(sp->count, 0);

	/* Calculate the number of stars that were process during the
	   previous frame and still need to be process for that frame. */
	min_count = sp->erase_count;
	if (sp->count < min_count)
		min_count = sp->count;

	s = sp->list;

	/* For all those star... */
	for (i=0; i<min_count; s++, i++) {
		/* ... erase them if necessary, ... */
		if (s->last_draw_offset != INVALID)
			EraseStar(s, buf);
		/* ... project them at their new position, ... */
		if (ProjectStar(s, geo)) {
			/* ... check the clipping of the buffer if the star are in
			   the pyramid of vision, ... */
			if (CheckClipping(s, buf, true))
				/* ... and draw them if they're really visible. */
				DrawStar(s, buf);
		}
		/* ... or mark them as invisible if they're not in the pyramid
		   of vision. */
		else
			s->last_draw_offset = INVALID;
	}

	/* For star that were process at the previous frame but that we don't
	   want to process anymore, we just need to erase them. */
	for (; i<sp->erase_count; s++, i++) {
		if (s->last_draw_offset != INVALID)
			EraseStar(s, buf);
	}

	/* For star that were not process before, but are now, we just need to
	   go through the projection, clipping and drawing steps. */
	for (; i<sp->count; s++, i++) {
		if (ProjectStar(s, geo)) {
			if (CheckClipping(s, buf, true))
				DrawStar(s, buf);
		}
		else
			s->last_draw_offset = INVALID;
	}
}

/* Update the clipping visibility of all star of the list (sp) to
  respect the new clipping defined for the buffer (buf). */
void RefreshClipping(buffer *buf, star_packet *sp)
{
	star		*s;
	int32		i;

	s = sp->list;
	for (i=0; i<sp->erase_count; s++, i++) {
		if (s->last_draw_offset != INVALID)
			CheckClipping(s, buf, false);
	}
}



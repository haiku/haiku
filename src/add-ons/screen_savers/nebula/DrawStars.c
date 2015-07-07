// Hand-translated from x86 assembly.

/* this function needs a definition from outside (the width of the bitmap) */

// #define STAR_WIDTH 320
// #define STAR_FUNCTION draw_stars320

/* draw a star (5 pixels) */
void draw_stars(int STAR_WIDTH, char* dstParam, char incParam)
{
	unsigned char* dst;
	unsigned char inc;
	dst = (unsigned char*)dstParam;
	inc = (unsigned char)incParam;

	dst[STAR_WIDTH] += inc;
	if (dst[STAR_WIDTH] < inc) {
		// Overflowed.
		dst[STAR_WIDTH] = 255;
	}

	inc >>= 1;
	unsigned char al;
	al = *dst;
	unsigned char cl;
	cl = dst[STAR_WIDTH - 1];
	al += inc;
	if (al >= inc) {
		cl += inc;
		if (cl < inc) {
			*dst = 255;
			dst[STAR_WIDTH - 1] = 255;
		} else {
			*dst = al;
			dst[STAR_WIDTH - 1] = cl;
		}
	} else {
		cl += inc;
		if (cl < inc) {
			*dst = 255;
			dst[STAR_WIDTH - 1] = 255;
		} else {
			*dst = 255;
			dst[STAR_WIDTH - 1] = cl;
		}
	}
	al = dst[STAR_WIDTH * 2];
	cl = dst[STAR_WIDTH + 1];
	al += inc;
	if (al < inc) {
		cl += inc;
		if (cl >= inc) {
			dst[STAR_WIDTH * 2] = 255;
			dst[STAR_WIDTH + 1] = cl;
			return;
		}
	} else {
		cl += inc;
		if (cl >= inc) {
			dst[STAR_WIDTH * 2] = al;
			dst[STAR_WIDTH + 1] = cl;
			return;
		}
	}

	dst[STAR_WIDTH * 2] = 255;
	dst[STAR_WIDTH + 1] = 255;
}

// Hand-translated from x86 assembly.

/* draw a star (5 pixels) */
void draw_stars(int star_width, char* dstParam, char incParam)
{
	unsigned char* dst;
	unsigned char inc;
	dst = (unsigned char*)dstParam;
	inc = (unsigned char)incParam;

	dst[star_width] += inc;
	if (dst[star_width] < inc) {
		// Overflowed.
		dst[star_width] = 255;
	}

	inc >>= 1;
	unsigned char al;
	al = *dst;
	unsigned char cl;
	cl = dst[star_width - 1];
	al += inc;
	if (al >= inc) {
		cl += inc;
		if (cl < inc) {
			*dst = 255;
			dst[star_width - 1] = 255;
		} else {
			*dst = al;
			dst[star_width - 1] = cl;
		}
	} else {
		cl += inc;
		if (cl < inc) {
			*dst = 255;
			dst[star_width - 1] = 255;
		} else {
			*dst = 255;
			dst[star_width - 1] = cl;
		}
	}
	al = dst[star_width * 2];
	cl = dst[star_width + 1];
	al += inc;
	if (al < inc) {
		cl += inc;
		if (cl >= inc) {
			dst[star_width * 2] = 255;
			dst[star_width + 1] = cl;
			return;
		}
	} else {
		cl += inc;
		if (cl >= inc) {
			dst[star_width * 2] = al;
			dst[star_width + 1] = cl;
			return;
		}
	}

	dst[star_width * 2] = 255;
	dst[star_width + 1] = 255;
}

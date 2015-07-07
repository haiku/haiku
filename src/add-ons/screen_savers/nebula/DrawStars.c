// Hand-translated from x86 assembly.

/* this function needs a definition from outside (the width of the bitmap) */

// #define STAR_WIDTH 320
// #define STAR_FUNCTION draw_stars320

/* draw a star (5 pixels) */
void STAR_FUNCTION(char* dstParam, char incParam)
{
//	movl	8(%ebp),%edi      /* dst */
//	movl	12(%ebp),%edx     /* inc */
//
	unsigned char* dst;
	unsigned char inc;
	dst = (unsigned char*)dstParam;
	inc = (unsigned char)incParam;

	dst[STAR_WIDTH] += inc;
	if (dst[STAR_WIDTH] < inc) {
	//	addb	%dl,(STAR_WIDTH)(%edi)
		// Overflowed.
		dst[STAR_WIDTH] = 255;
		//	movb	$255,(STAR_WIDTH)(%edi)
	}
//	jnc		3f
//
//3:
	inc >>= 1;
//	shrb	$1,%dl
	unsigned char al;
	al = *dst;
//	movb	(%edi),%al
	unsigned char cl;
	cl = dst[STAR_WIDTH - 1];
//	movb	(STAR_WIDTH-1)(%edi),%cl
	al += inc;
//	addb	%dl,%al
	if (al >= inc) {
		cl += inc;
		//	addb	%dl,%cl
		if (cl < inc) {
		//	jc		2f
		// 2:
			*dst = 255;
			// movb	$255,(%edi)
			dst[STAR_WIDTH - 1] = 255;
			// movb	$255,(STAR_WIDTH-1)(%edi)
		} else {
			*dst = al;
			// movb	%al,(%edi)
			dst[STAR_WIDTH - 1] = cl;
			// movb	%cl,(STAR_WIDTH-1)(%edi)
		}
	} else {
	// 1:
		cl += inc;
		//	addb	%dl,%cl
		if (cl < inc) {
		//	jc		2f
			*dst = 255;
			// movb	$255,(%edi)
			dst[STAR_WIDTH - 1] = 255;
			// movb	$255,(STAR_WIDTH-1)(%edi)
		} else {
			*dst = 255;
			// movb	$255,(%edi)
			dst[STAR_WIDTH - 1] = cl;
			// movb	%cl,(STAR_WIDTH-1)(%edi)
		}
	}
	al = dst[STAR_WIDTH * 2];
//	movb	2*STAR_WIDTH(%edi),%al
	cl = dst[STAR_WIDTH + 1];
//	movb	(STAR_WIDTH+1)(%edi),%cl
	al += inc;
//	addb	%dl,%al
	if (al < inc) {
	//	jc		11f
		cl += inc;
		//	addb	%dl,%cl
		if (cl >= inc) {
			dst[STAR_WIDTH * 2] = 255;
			//	movb	$255,(2*STAR_WIDTH)(%edi)
			dst[STAR_WIDTH + 1] = cl;
			//	movb	%cl,(STAR_WIDTH+1)(%edi)
			return;
		}
	} else {
		cl += inc;
		//	addb	%dl,%cl
		if (cl >= inc) {
		//	jc		21f
			dst[STAR_WIDTH * 2] = al;
			//	movb	%al,2*STAR_WIDTH(%edi)
			dst[STAR_WIDTH + 1] = cl;
			//	movb	%cl,(STAR_WIDTH+1)(%edi)
			return;
		}
	}
//
//21:
	dst[STAR_WIDTH * 2] = 255;
//	movb	$255,2*STAR_WIDTH(%edi)
	dst[STAR_WIDTH + 1] = 255;
//	movb	$255,(STAR_WIDTH+1)(%edi)
//	popl	%edi
//	leave
//	ret
}

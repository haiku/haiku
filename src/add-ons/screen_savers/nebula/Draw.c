// Hand-translated from x86 assembly.

/* memshset (char *dst, int center_shade,int fixed_shade, int length_2) */
/* dst is the *center* of the dst segment */
/* center shade is the initial color (at center) (!! 8:8 also) */
/* fixed_shade is the fixed point increment (8:8) */
/* length_2 is the 1/2 length to set (to the left, and to the right) */

/* a memaddshadedset function (or something like that ) */

void memshset(char* dstParam, int center_shade, int fixed_shade, int half_length)
{
	unsigned char* dst;
	unsigned char* source;
	int offset;

	if (half_length == 0)
		return;

//	movl	8(%ebp),%edi      /* dst */
//	movl	12(%ebp),%eax	/* center_shade*/
//	movl	16(%ebp),%ebx	  /* fixed_shade */
//	movl	20(%ebp),%ecx	  /* half_length */
//
//@	orl		%ecx,%ecx
//@	jz		2f
//

	dst = (unsigned char*)dstParam;
	source = dst;
//	movl	%edi,%esi
	dst -= half_length; // Left segment
//	subl	%ecx,%edi		/* %edi  left segment */
	source += half_length; // Right segment
//	addl	%ecx,%esi		/* %esi  right segment */
	source++; // Don't overlap at middle
//	incl	%esi			/* dont overlap at middle */
//
	offset = half_length;
//	movl	%ecx,%edx
	offset = -offset;
//	negl	%edx
	offset--; // For in-advance incl in loop
//	decl	%edx			/* for in-advance incl in loop */

	do {
		int tmp;
//1:
		center_shade += fixed_shade; // Increment color.
		center_shade &= 0xFFFF; // Mask so we can do overflow tests.
//	addl	%ebx,%eax		/* increment color */
		offset++;
//	incl	%edx

		tmp = dst[half_length] + (center_shade >> 8); // Increment left pixel.
		//	addb	%ah,(%edi,%ecx)	/* increment  left pixel */
		// Did it overflow?
		if (tmp & 0xFF00) {
		//	jc		3f				/* did overflow ? */
			dst[half_length] = 255;
//3:
//	movb	$255,(%edi,%ecx)
		} else {
			dst[half_length] = tmp;
		}


		tmp = source[offset] + (center_shade >> 8); // Increment right pixel.
		// Did it overflow?
		if (tmp & 0xFF00) {
			source[offset] = 255;
//4:
//	movb	$255,(%esi,%edx)
//	decl	%ecx
//	jnz		1b
//
//	popl	%ebx
//	popl	%esi
//	popl	%edi
//
//	leave
//	ret
		} else {
			source[offset] = tmp;
		}
//	addb	%ah,(%esi,%edx) /* increment right pixel */
//	jc		4f
//
	} while (--half_length != 0);
//	decl	%ecx
//	jnz		1b
//
//2:
//	popl	%ebx
//	popl	%esi
//	popl	%edi
//
//	leave
//	ret
}


/* do the "motion blur" (actually the pixel fading) */
void mblur(char* srcParam, int nbpixels)
{
//	movl	8(%ebp),%edi      /* dst */
//	movl	12(%ebp),%ecx     /* number */
	unsigned int clear1UpperBit = 0x7f7f7f7f;
	unsigned int clear3UpperBits = 0x1f1f1f1f;
	unsigned long* src = (unsigned long*)srcParam;
//	movl	$0x7f7f7f7f,%edx  /* clear 1 upper bits */
//	movl	$0x1f1f1f1f,%esi  /* clear 3 upper bits */

	if ((nbpixels >>= 2) == 0)
	//	shrl	$2,%ecx
		return;
		//	jz	2f
//
	do {
//1:
		long eax, ebx;
		eax = *src;
	//	movl	(%edi),%eax
		src++;
	//	addl	$4,%edi
	//
		ebx = eax;
	//	movl	%eax,%ebx
		eax >>= 1;
	//	shrl	$1,%eax
	//
	//
		ebx >>= 3;
	//	shrl	$3,%ebx
		eax &= clear1UpperBit;
	//	andl	%edx,%eax
	//
		ebx &= clear3UpperBits;
	//	andl	%esi,%ebx
		eax += ebx;
	//	addl	%ebx,%eax
	//
		src[-1] = eax;
	//	movl	%eax,-4(%edi)
	} while (--nbpixels != 0);
	//	decl	%ecx
	//	jnz	1b
}

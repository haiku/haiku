
.text

/* this function needs a definition from outside (the width of the bitmap) */

// #define STAR_WIDTH 320
// #define STAR_FUNCTION draw_stars320

/* draw a star (5 pixels) */
.globl	STAR_FUNCTION
.type   STAR_FUNCTION,@function

STAR_FUNCTION:
	pushl	%ebp
	movl	%esp,%ebp

	pushl	%edi

	movl	8(%ebp),%edi      /* dst */
	movl	12(%ebp),%edx     /* inc */

	addb	%dl,(STAR_WIDTH)(%edi)
	jnc		3f
	movb	$255,(STAR_WIDTH)(%edi)

3:
	shrb	$1,%dl
	movb	(%edi),%al
	movb	(STAR_WIDTH-1)(%edi),%cl
	addb	%dl,%al
	jc		1f
	addb	%dl,%cl
	jc		2f
	movb	%al,(%edi)
	movb	%cl,(STAR_WIDTH-1)(%edi)
	jmp		31f
1:
	addb	%dl,%cl
	jc		2f
	movb	$255,(%edi)
	movb	%cl,(STAR_WIDTH-1)(%edi)

	jmp 	31f

2:	
	movb	$255,(%edi)
	movb	$255,(STAR_WIDTH-1)(%edi)


31:		
	movb	2*STAR_WIDTH(%edi),%al
	movb	(STAR_WIDTH+1)(%edi),%cl
	addb	%dl,%al
	jc		11f
	addb	%dl,%cl
	jc		21f
	movb	%al,2*STAR_WIDTH(%edi)
	movb	%cl,(STAR_WIDTH+1)(%edi)
	popl	%edi
	leave
	ret

	
11:	
	addb	%dl,%cl
	jc		21f
	movb	$255,(2*STAR_WIDTH)(%edi)
	movb	%cl,(STAR_WIDTH+1)(%edi)
	popl	%edi
	leave
	ret
	
21:	
	movb	$255,2*STAR_WIDTH(%edi)
	movb	$255,(STAR_WIDTH+1)(%edi)
	popl	%edi
	leave
	ret

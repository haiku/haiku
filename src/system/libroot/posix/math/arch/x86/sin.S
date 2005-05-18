/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#define FUNCTION(x) .global x; .type x,@function; x
#define FUNCTION_END(x)
#define WEAK_ALIAS(original, alias) .weak original; original = alias

FUNCTION(__sin):
	fldl		4(%esp)
	fsin
	fnstsw		%ax
	testl		$0x400,%eax
	jnz			1f
	ret
	.align 4
1:	fldpi
	fadd		%st(0)
	fxch		%st(1)
2:	fprem1
	fnstsw		%ax
	testl		$0x400,%eax
	jnz			2b
	fstp		%st(1)
	fsin
	ret
FUNCTION_END(__sin)

WEAK_ALIAS(__sin, sin)

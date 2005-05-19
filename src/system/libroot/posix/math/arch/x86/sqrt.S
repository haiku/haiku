/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#define FUNCTION(x) .global x; .type x,@function; x
#define FUNCTION_END(x)
#define WEAK_ALIAS(original, alias) .weak original; original = alias

FUNCTION(__sqrt):
	fldl		4(%esp)
	fsqrt
	ret
FUNCTION_END(__sqrt)

WEAK_ALIAS(__sqrt, sqrt)

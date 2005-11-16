#include <sysdep.h>

	.text
ENTRY(__fabsl)
	fldt	4(%esp)
	fabs
	ret
END(__fabsl)
weak_alias (__fabsl, fabsl)

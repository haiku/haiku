#include <sysdep.h>

	.text
ENTRY(__fabsf)
	flds	4(%esp)
	fabs
	ret
END(__fabsf)
weak_alias (__fabsf, fabsf)


#include <asm_defs.h>

	.text

FUNCTION(_arch_exception_loop):
arch_loop:
    b arch_loop
FUNCTION_END(_arch_exception_loop)

/* The intention in this function in contrast with the previous is that by
 * prototype, the compiler sets x0 and x1 to its parameters
 * so when attaching we can track where it came from */
FUNCTION(_arch_exception_panic):
arch_panic_loop:
    b arch_panic_loop
FUNCTION_END(_arch_exception_panic)

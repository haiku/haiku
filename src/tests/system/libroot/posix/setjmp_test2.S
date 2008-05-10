#define FUNCTION(x) .global x; .type x,@function; x

FUNCTION(main):
	mov		$jmpBuffer, %eax
	push	%eax
	call	setjmp
	add		$4, %esp

	mov		$0, %eax
	ret

.comm	jmpBuffer, 1024, 32

	;; Return with exit code 47.

	.global _start
_start:
	ldi32 #47,r4
	ldi32 #1,r0
	int   #10

	.global _start
_start:

; write (hello world)
	ldi32 #14,r6
	ldi32 #hello,r5
	ldi32 #1,r4
	ldi32 #5,r0
	int   #10
; exit (0)
	ldi32 #0,r4
	ldi32 #1,r0
	int   #10

hello:	.ascii "Hello World!\r\n"

/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

.section .init, "ax", @progbits
	ldr		lr, [sp]
	add		sp, sp, #16
	ret

.section .fini, "ax", @progbits
	ldr		lr, [sp]
	add		sp, sp, #16
	ret

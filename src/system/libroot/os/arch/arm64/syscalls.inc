/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#define _AARCH64_SYSCALL_ENCODE(name, code, nparams) \
.globl name; \
.type name,%function; \
.p2align 2; \
name: \
	svc #(((code) << 5) | (nparams)); \
	ret

#define SYSCALL0(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 0)
#define SYSCALL1(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 1)
#define SYSCALL2(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 2)
#define SYSCALL3(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 3)
#define SYSCALL4(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 4)
#define SYSCALL5(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 5)
#define SYSCALL6(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 6)
#define SYSCALL7(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 7)
#define SYSCALL8(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 8)
#define SYSCALL9(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 9)
#define SYSCALL10(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 10)
#define SYSCALL11(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 11)
#define SYSCALL12(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 12)
#define SYSCALL13(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 13)
#define SYSCALL14(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 14)
#define SYSCALL15(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 15)
#define SYSCALL16(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 16)
#define SYSCALL17(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 17)
#define SYSCALL18(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 18)
#define SYSCALL19(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 19)
#define SYSCALL20(name, n) _AARCH64_SYSCALL_ENCODE(name, n, 20)

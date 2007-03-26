/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Hugo Santos <hugosantos@gmail.com>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef STRACE_H
#define STRACE_H

class Syscall;

Syscall *get_syscall(const char *name);

#endif	// STRACE_H

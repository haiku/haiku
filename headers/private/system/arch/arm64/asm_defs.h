/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_ARCH_ARM64_ASM_DEFS_H
#define SYSTEM_ARCH_ARM64_ASM_DEFS_H

#define SYMBOL(name)			.global name; name
#define SYMBOL_END(name)		.size name, . - name
#define STATIC_FUNCTION(name)	.type name, %function; name
#define FUNCTION(name)			.global name; .type name, %function; name
#define FUNCTION_END(name)		.size name, . - name

#endif	/* SYSTEM_ARCH_ARM64_ASM_DEFS_H */

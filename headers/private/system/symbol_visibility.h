/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#ifndef SYMBOL_VISIBILITY_H
#define SYMBOL_VISIBILITY_H


#if __GNUC__ >= 4
#	define HIDDEN_FUNCTION(function)	do {} while (0)
#	define HIDDEN_FUNCTION_ATTRIBUTE	__attribute__((visibility("hidden")))
#else
#	define HIDDEN_FUNCTION(function)	asm volatile(".hidden " #function)
#	define HIDDEN_FUNCTION_ATTRIBUTE
#endif


#endif /* !SYMBOL_VISIBILITY_H */

/*
 * Debug macros for the Midi Kit
 *
 * Copyright (c) 2002-2003 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef MIDI_DEBUG_H
#define MIDI_DEBUG_H

#include <OS.h>
#include <stdio.h>

#define WARN(args) { fprintf(stderr, "[midi] WARNING: %s\n", args); } 
#define UNIMPLEMENTED { fprintf(stderr, "[midi] UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__); }

#ifdef DEBUG

#define TRACE(args) { printf("[midi] "); printf args; printf("\n"); } 

#define ASSERT(expr) \
	if (!(expr))\
	{\
		char buf[1024];\
		sprintf(\
			buf, "%s(%d) : Assertion \"%s\" failed!",\
			__FILE__, __LINE__, #expr);\
		debugger(buf);\
	}\

#else

#define TRACE(args)
#define ASSERT(expr)

#endif

#endif // MIDI_DEBUG_H

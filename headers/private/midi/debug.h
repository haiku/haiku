/**
 * @file debug.h
 *
 * Debug macros for the Midi Kit
 *
 * @author Matthijs Hollemans
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

/**
 * @file debug.h
 *
 * Debug macros for the Midi Kit
 *
 * @author Matthijs Hollemans
 * @author Marcus Overhagen
 */

#ifndef MIDI_DEBUG_H
#define MIDI_DEBUG_H

#include <stdio.h>

#ifndef NDEBUG

#define TRACE(args)    { printf args; } 
#define UNIMPLEMENTED  TRACE(("[midi] UNIMPLEMENTED %s\n",__PRETTY_FUNCTION__))

#else

#define TRACE(args)
#define UNIMPLEMENTED

#endif

#endif // MIDI_DEBUG_H


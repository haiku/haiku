
#ifndef _MIDI2_DEFS_H
#define _MIDI2_DEFS_H

#include <OS.h>
#include <Errors.h>

#ifndef _MIDI_CONSTANTS_
#define _MIDI_CONSTANTS_

/* Channel Message Masks */
const uchar B_NOTE_OFF          = 0x80;
const uchar B_NOTE_ON           = 0x90;
const uchar B_KEY_PRESSURE      = 0xa0;
const uchar B_CONTROL_CHANGE    = 0xb0;
const uchar B_PROGRAM_CHANGE    = 0xc0;
const uchar B_CHANNEL_PRESSURE  = 0xd0;
const uchar B_PITCH_BEND        = 0xe0;

/* System Messages */
const uchar B_SYS_EX_START      = 0xf0;
const uchar B_MIDI_TIME_CODE    = 0xf1;
const uchar B_SONG_POSITION     = 0xf2;
const uchar B_SONG_SELECT       = 0xf3;
const uchar B_CABLE_MESSAGE     = 0xf5;
const uchar B_TUNE_REQUEST      = 0xf6;
const uchar B_SYS_EX_END        = 0xf7;
const uchar B_TIMING_CLOCK      = 0xf8;
const uchar B_START             = 0xfa;
const uchar B_CONTINUE          = 0xfb;
const uchar B_STOP              = 0xfc;
const uchar B_ACTIVE_SENSING    = 0xfe;
const uchar B_SYSTEM_RESET      = 0xff;

/* Controller Numbers */
const uchar B_MODULATION            = 0x01;
const uchar B_BREATH_CONTROLLER     = 0x02;
const uchar B_FOOT_CONTROLLER       = 0x04;
const uchar B_PORTAMENTO_TIME       = 0x05;
const uchar B_DATA_ENTRY            = 0x06;
const uchar B_MAIN_VOLUME           = 0x07;
const uchar B_MIDI_BALANCE          = 0x08;
const uchar B_PAN                   = 0x0a;
const uchar B_EXPRESSION_CTRL       = 0x0b;
const uchar B_GENERAL_CTRL_1        = 0x10;
const uchar B_GENERAL_CTRL_2        = 0x11;
const uchar B_GENERAL_CTRL_3        = 0x12;
const uchar B_GENERAL_CTRL_4        = 0x13;
const uchar B_SUSTAIN_PEDAL         = 0x40;
const uchar B_PORTAMENTO            = 0x41;
const uchar B_SOSTENUTO             = 0x42;
const uchar B_SOFT_PEDAL            = 0x43;
const uchar B_HOLD_2                = 0x45;
const uchar B_GENERAL_CTRL_5        = 0x50;
const uchar B_GENERAL_CTRL_6        = 0x51;
const uchar B_TEMPO_CHANGE          = 0x51;
const uchar B_GENERAL_CTRL_7        = 0x52;
const uchar B_GENERAL_CTRL_8        = 0x53;
const uchar B_EFFECTS_DEPTH         = 0x5b;
const uchar B_TREMOLO_DEPTH         = 0x5c;
const uchar B_CHORUS_DEPTH          = 0x5d;
const uchar B_CELESTE_DEPTH         = 0x5e;
const uchar B_PHASER_DEPTH          = 0x5f;
const uchar B_DATA_INCREMENT        = 0x60;
const uchar B_DATA_DECREMENT        = 0x61;
const uchar B_RESET_ALL_CONTROLLERS = 0x79;
const uchar B_LOCAL_CONTROL         = 0x7a;
const uchar B_ALL_NOTES_OFF         = 0x7b;
const uchar B_OMNI_MODE_OFF         = 0x7c;
const uchar B_OMNI_MODE_ON          = 0x7d;
const uchar B_MONO_MODE_ON          = 0x7e;
const uchar B_POLY_MODE_ON          = 0x7f;

#endif // _MIDI_CONSTANTS_

#endif // _MIDI2_DEFS_H

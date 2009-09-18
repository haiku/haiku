
#ifndef _MIDI_DEFS_H
#define _MIDI_DEFS_H

#include <OS.h>
#include <Errors.h>

//------------------------------------------------------------------------------

/* System time converted to int milliseconds */
#define B_NOW  ((uint32)(system_time()/1000))

//------------------------------------------------------------------------------

/* Synthesizer things */

#define B_SYNTH_DIRECTORY    B_SYSTEM_DATA_DIRECTORY
#define B_BIG_SYNTH_FILE     "synth/big_synth.sy"
#define B_LITTLE_SYNTH_FILE  "synth/little_synth.sy"

typedef enum synth_mode 
{
	B_NO_SYNTH,
	B_BIG_SYNTH,
	B_LITTLE_SYNTH,
	B_DEFAULT_SYNTH,
	B_SAMPLES_ONLY
} 
synth_mode;

//------------------------------------------------------------------------------

/* Need to move these into Errors.h */

enum
{
	B_BAD_INSTRUMENT = B_MIDI_ERROR_BASE + 0x100,
	B_BAD_MIDI_DATA,
	B_ALREADY_PAUSED,
	B_ALREADY_RESUMED,
	B_NO_SONG_PLAYING,
	B_TOO_MANY_SONGS_PLAYING
};

//------------------------------------------------------------------------------

#ifndef uchar
typedef unsigned char uchar;
#endif

#ifndef _MIDI_CONSTANTS_
#define _MIDI_CONSTANTS_

/* Channel Message Masks*/
const uchar B_NOTE_OFF          = 0x80;
const uchar B_NOTE_ON           = 0x90;
const uchar B_KEY_PRESSURE      = 0xa0;
const uchar B_CONTROL_CHANGE    = 0xb0;
const uchar B_PROGRAM_CHANGE    = 0xc0;
const uchar B_CHANNEL_PRESSURE  = 0xd0;
const uchar B_PITCH_BEND        = 0xe0;

/* System Messages*/
const uchar B_SYS_EX_START		= 0xf0;
const uchar B_MIDI_TIME_CODE	= 0xf1;
const uchar B_SONG_POSITION		= 0xf2;
const uchar B_SONG_SELECT		= 0xf3;
const uchar B_CABLE_MESSAGE		= 0xf5;
const uchar B_TUNE_REQUEST		= 0xf6;
const uchar B_SYS_EX_END		= 0xf7;
const uchar B_TIMING_CLOCK		= 0xf8;
const uchar B_START				= 0xfa;
const uchar B_CONTINUE			= 0xfb;
const uchar B_STOP				= 0xfc;
const uchar B_ACTIVE_SENSING	= 0xfe;
const uchar B_SYSTEM_RESET		= 0xff;

/* Controller Numbers*/
const uchar B_MODULATION            = 0x01;
const uchar B_BREATH_CONTROLLER     = 0x02;
const uchar B_FOOT_CONTROLLER       = 0x04;
const uchar B_PORTAMENTO_TIME       = 0x05;
const uchar B_DATA_ENTRY            = 0x06;
const uchar B_MAIN_VOLUME           = 0x07;
const uchar B_MIDI_BALANCE          = 0x08;  /* used to be B_BALANCE */
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
 
const uchar B_TEMPO_CHANGE          = 0x51;

#endif // _MIDI_CONSTANTS_

//------------------------------------------------------------------------------

typedef enum midi_axe 
{
	/* Pianos */
	B_ACOUSTIC_GRAND=0,
	B_BRIGHT_GRAND,
	B_ELECTRIC_GRAND,
	B_HONKY_TONK,
	B_ELECTRIC_PIANO_1,
	B_ELECTRIC_PIANO_2,
	B_HARPSICHORD,
	B_CLAVICHORD,

	/* Tuned Idiophones */
	B_CELESTA,
	B_GLOCKENSPIEL,
	B_MUSIC_BOX,
	B_VIBRAPHONE,
	B_MARIMBA,
	B_XYLOPHONE,
	B_TUBULAR_BELLS,
	B_DULCIMER,

	/* Organs */
	B_DRAWBAR_ORGAN,
	B_PERCUSSIVE_ORGAN,
	B_ROCK_ORGAN,
	B_CHURCH_ORGAN,
	B_REED_ORGAN,
	B_ACCORDION,
	B_HARMONICA,
	B_TANGO_ACCORDION,

	/* Guitars */
	B_ACOUSTIC_GUITAR_NYLON,
	B_ACOUSTIC_GUITAR_STEEL,
	B_ELECTRIC_GUITAR_JAZZ,
	B_ELECTRIC_GUITAR_CLEAN,
	B_ELECTRIC_GUITAR_MUTED,
	B_OVERDRIVEN_GUITAR,
	B_DISTORTION_GUITAR,
	B_GUITAR_HARMONICS,

	/* Basses */
	B_ACOUSTIC_BASS,
	B_ELECTRIC_BASS_FINGER,
	B_ELECTRIC_BASS_PICK,
	B_FRETLESS_BASS,
	B_SLAP_BASS_1,
	B_SLAP_BASS_2,
	B_SYNTH_BASS_1,
	B_SYNTH_BASS_2,

	/* Strings */
	B_VIOLIN,
	B_VIOLA,
	B_CELLO,
	B_CONTRABASS,
	B_TREMOLO_STRINGS,
	B_PIZZICATO_STRINGS,
	B_ORCHESTRAL_STRINGS,
	B_TIMPANI,

	/* Ensemble strings and voices */
	B_STRING_ENSEMBLE_1,
	B_STRING_ENSEMBLE_2,
	B_SYNTH_STRINGS_1,
	B_SYNTH_STRINGS_2,
	B_VOICE_AAH,
	B_VOICE_OOH,
	B_SYNTH_VOICE,
	B_ORCHESTRA_HIT,

	/* Brass */
	B_TRUMPET,
	B_TROMBONE,
	B_TUBA,
	B_MUTED_TRUMPET,
	B_FRENCH_HORN,
	B_BRASS_SECTION,
	B_SYNTH_BRASS_1,
	B_SYNTH_BRASS_2,

	/* Reeds */
	B_SOPRANO_SAX,
	B_ALTO_SAX,
	B_TENOR_SAX,
	B_BARITONE_SAX,
	B_OBOE,
	B_ENGLISH_HORN,
	B_BASSOON,
	B_CLARINET,

	/* Pipes */
	B_PICCOLO,
	B_FLUTE,
	B_RECORDER,
	B_PAN_FLUTE,
	B_BLOWN_BOTTLE,
	B_SHAKUHACHI,
	B_WHISTLE,
	B_OCARINA,

	/* Synth Leads*/
	B_LEAD_1,
	B_SQUARE_WAVE = B_LEAD_1,
	B_LEAD_2,
	B_SAWTOOTH_WAVE = B_LEAD_2,
	B_LEAD_3,
	B_CALLIOPE = B_LEAD_3,
	B_LEAD_4,
	B_CHIFF = B_LEAD_4,
	B_LEAD_5,
	B_CHARANG = B_LEAD_5,
	B_LEAD_6,
	B_VOICE = B_LEAD_6,
	B_LEAD_7,
	B_FIFTHS = B_LEAD_7,
	B_LEAD_8,
	B_BASS_LEAD = B_LEAD_8,

	/* Synth Pads */
	B_PAD_1,
	B_NEW_AGE = B_PAD_1,
	B_PAD_2,
	B_WARM = B_PAD_2,
	B_PAD_3,
	B_POLYSYNTH = B_PAD_3,
	B_PAD_4,
	B_CHOIR = B_PAD_4,
	B_PAD_5,
	B_BOWED = B_PAD_5,
	B_PAD_6,
	B_METALLIC = B_PAD_6,
	B_PAD_7,
	B_HALO = B_PAD_7,
	B_PAD_8,  
	B_SWEEP = B_PAD_8,

	/* Effects */
	B_FX_1,
	B_FX_2,
	B_FX_3,
	B_FX_4,
	B_FX_5,
	B_FX_6,
	B_FX_7,
	B_FX_8,

	/* Ethnic */
	B_SITAR,
	B_BANJO,
	B_SHAMISEN,
	B_KOTO,
	B_KALIMBA,
	B_BAGPIPE,
	B_FIDDLE,
	B_SHANAI,

	/* Percussion */
	B_TINKLE_BELL,
	B_AGOGO,
	B_STEEL_DRUMS,
	B_WOODBLOCK,
	B_TAIKO_DRUMS,
	B_MELODIC_TOM,
	B_SYNTH_DRUM,
	B_REVERSE_CYMBAL,

	/* Sound Effects */
	B_FRET_NOISE,
	B_BREATH_NOISE,
	B_SEASHORE,
	B_BIRD_TWEET,
	B_TELEPHONE,
	B_HELICOPTER,
	B_APPLAUSE,
	B_GUNSHOT
} 
midi_axe;

#endif // _MIDI_DEFS_H

// ****************************************************************************
//
//		EchoGalsXface.H
//
//		Include file for interfacing with the CEchoGals generic driver class.
//
//		Set editor tabs to 3 for your viewing pleasure.
//
// ----------------------------------------------------------------------------
//
// This file is part of Echo Digital Audio's generic driver library.
// Copyright Echo Digital Audio Corporation (c) 1998 - 2005
// All rights reserved
// www.echoaudio.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// ****************************************************************************

/*

	
	
	Here's a block diagram of how most of the cards work:

                  +-----------+
         record   |           |<-------------------- Inputs
          <-------|           |        |
  PCI             | Transport |        |
  bus             |  engine   |       \|/
          ------->|           |    +-------+
           play   |           |--->|monitor|-------> Outputs
                  +-----------+    | mixer |
                                   +-------+

	The lines going to and from the PCI bus represent "pipes".  A pipe performs
	audio transport - moving audio data to and from buffers on the host via
	bus mastering.
	
	The inputs and outputs on the right represent input and output "busses."
	A bus is a physical, real connection to the outside world.  An example
	of a bus would be the 1/4" analog connectors on the back of Layla or 
	an RCA S/PDIF connector.
  
	For most cards, there is a one-to-one correspondence between outputs
	and busses; that is, each individual pipe is hard-wired to a single bus.
	
	Cards that work this way are Darla20, Gina20, Layla20, Darla24, Gina24,
	Layla24, Mona, Indigo, Indigo io, and Indigo dj.
	

	Mia has a feature called "virtual outputs." 


                  +-----------+
           record |           |<----------------------------- Inputs
          <-------|           |                  |
     PCI          | Transport |                  |
     bus          |  engine   |                 \|/
          ------->|           |   +------+   +-------+
            play  |           |-->|vmixer|-->|monitor|-------> Outputs
                  +-----------+   +------+   | mixer |
                                             +-------+


	Obviously, the difference here is the box labeled "vmixer."  Vmixer is 
	short for "virtual output mixer."  For Mia, pipes are *not* hard-wired
	to a single bus; the vmixer lets you mix any pipe to any bus in any
	combination.
	
	Note, however, that the left-hand side of the diagram is unchanged.  
	Transport works exactly the same way - the difference is in the mixer stage.
	
	
	Pipes and busses are numbered starting at zero.
	
	

	Pipe index
	==========	

	A number of calls in CEchoGals refer to a "pipe index".  A pipe index is
	a unique number for a pipe that unambiguously refers to a playback or record
	pipe.  Pipe indices are numbered starting with analog outputs, followed by 
	digital outputs, then analog inputs, then digital inputs.
	
	Take Gina24 as an example:
	
	Pipe index
	
	0-7				Analog outputs
	8-15				Digital outputs
	16-17				Analog inputs
	18-25				Digital inputs
	
	
	You get the pipe index by calling CEchoGals::OpenAudio; the other transport
	functions take the pipe index as a parameter.  If you need a pipe index for
	some other reason, use the handy MakePipeIndex method.
	

	Some calls take a CChannelMask parameter; CChannelMask is a handy way to group
	pipe indices.
				


	Digital mode switch
	===================

	Some cards (right now, Gina24, Layla24, and Mona) have a Digital Mode Switch
	or DMS.  Cards with a DMS can be set to one of three mutually exclusive 
	digital modes: S/PDIF RCA, S/PDIF optical, or ADAT optical.

	This may create some confusion since ADAT optical is 8 channels wide and
	S/PDIF is only two channels wide.  Gina24, Layla24, and Mona handle this
	by acting as if they always have 8 digital outs and ins.  If you are in 
	either S/PDIF mode, the last 6 channels don't do anything - data sent
	out these channels is thrown away and you will always record zeros.
	
	Note that with Gina24, Layla24, and Mona, sample rates above 50 kHz are
	only available if	you have the card configured for S/PDIF optical or S/PDIF RCA.
	
	
	
	Double speed mode
	=================
	
	Some of the cards support 88.2 kHz and 96 kHz sampling (Darla24, Gina24, Layla24,
	Mona, Mia, and Indigo).  For these cards, the driver sometimes has to worry about
	"double speed mode"; double speed mode applies whenever the sampling rate is above
	50 kHz.  
	
	For instance, Mona and Layla24 support word clock sync.  However, they actually
	support two different word clock modes - single speed (below 50 kHz) and double 
	speed (above 50 kHz).  The hardware detects if a single or double speed word clock
	signal is present; the generic code uses that information to determine which mode
	to use.  
	
	The generic code takes care of all this for you.

   For hardware that supports 176.4 and 192 kHz, there is a corresponding quad speed 
	mode.
	
*/


//	Prevent problems with multiple includes
#ifndef _ECHOGALSXFACE_
#define _ECHOGALSXFACE_


//***********************************************************************
//
//	PCI configuration space 
//
//***********************************************************************	

//
// PCI vendor ID and device IDs for the hardware
//
#define VENDOR_ID						 0x1057
#define DEVICE_ID_56301				 0x1801
#define DEVICE_ID_56361				 0x3410
#define SUBVENDOR_ID					 ((ULONG)0xECC0)


//
//	Valid Echo PCI subsystem card IDs
//
#define DARLA							0x0010
#define GINA							0x0020
#define LAYLA							0x0030
#define DARLA24						0x0040
#define GINA24					 		0x0050
#define LAYLA24						0x0060
#define MONA							0x0070
#define MIA								0x0080
#define INDIGO							0x0090
#define INDIGO_IO						0x00a0
#define INDIGO_DJ						0x00b0
#define DC8								0x00c0
#define ECHO3G							0x0100
#define BASEPCI						0xfff0


//***********************************************************************
//
//	Array sizes and so forth
//
//***********************************************************************	

//
// Sizes
//
#define ECHO_MAXNAMELEN				128			// Max card name length
															// (includes 0 terminator)
#define ECHO_MAXAUDIOINPUTS		32				// Max audio input channels
#define ECHO_MAXAUDIOOUTPUTS		32				// Max audio output channels
#define ECHO_MAXAUDIOPIPES			32				// Max number of input and output pipes
#define ECHO_MAXMIDIJACKS			1				// Max MIDI ports
#define ECHO_MTC_QUEUE_SZ			32				// Max MIDI time code input queue entries

//
// MIDI activity indicator timeout
//
#define MIDI_ACTIVITY_TIMEOUT_USEC	200000


//*****************************************************************************
//
// Clocks
//
//*****************************************************************************

//
// Clock numbers
//
#define ECHO_CLOCK_INTERNAL				0
#define ECHO_CLOCK_WORD						1
#define ECHO_CLOCK_SUPER					2
#define ECHO_CLOCK_SPDIF					3
#define ECHO_CLOCK_ADAT						4
#define ECHO_CLOCK_ESYNC					5
#define ECHO_CLOCK_ESYNC96					6
#define ECHO_CLOCK_MTC						7
#define ECHO_CLOCK_NONE						0xffff

//
// Clock bit numbers - used to report capabilities and whatever clocks
// are being detected dynamically.
//
#define ECHO_CLOCK_BIT_INTERNAL			(1<<ECHO_CLOCK_INTERNAL)
#define ECHO_CLOCK_BIT_WORD				(1<<ECHO_CLOCK_WORD)
#define ECHO_CLOCK_BIT_SUPER				(1<<ECHO_CLOCK_SUPER)
#define ECHO_CLOCK_BIT_SPDIF				(1<<ECHO_CLOCK_SPDIF)
#define ECHO_CLOCK_BIT_ADAT				(1<<ECHO_CLOCK_ADAT)
#define ECHO_CLOCK_BIT_ESYNC				(1<<ECHO_CLOCK_ESYNC)
#define ECHO_CLOCK_BIT_ESYNC96			(1<<ECHO_CLOCK_ESYNC96)
#define ECHO_CLOCK_BIT_MTC					(1<<ECHO_CLOCK_MTC)


//*****************************************************************************
//
// Digital modes
//
//*****************************************************************************

//
// Digital modes for Mona, Layla24, and Gina24
//
#define DIGITAL_MODE_NONE					0xFF
#define DIGITAL_MODE_SPDIF_RCA			0
#define DIGITAL_MODE_SPDIF_OPTICAL		1
#define DIGITAL_MODE_ADAT					2

//
//	Digital mode capability masks
//
#define ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_RCA		(1<<DIGITAL_MODE_SPDIF_RCA)
#define ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_OPTICAL	(1<<DIGITAL_MODE_SPDIF_OPTICAL)
#define ECHOCAPS_HAS_DIGITAL_MODE_ADAT				(1<<DIGITAL_MODE_ADAT)


//*****************************************************************************
//
// Driver flags
//
//*****************************************************************************

//
//	Note that some flags are read-only (look for ROFLAG as part of the name).  
// The read-only flags are used to tell you what the card can do; you can't 
// set or clear them.
//
// Some cards can only handle mono & stereo interleaved data in host audio buffers; that
// is, you may only specify 1 or 2 in the wPipeWidth field when you open a pipe for 
// playing or recording.
// 
// If the card has the super interleave flag set, that means that you can interleave 
// by more than 2.  For super interleave-capable cards, you can interleave by 1, 2, 
// 4, 6, 8, 10, 12, 14, or 16.  Note that you cannot interleave beyond the actual 
// number of pipes on the card - if the card has 16 pipes and you open pipe 6,
// the maximum interleave would be 10.
//
//
// Gina24, Layla24, and Mona support digital input auto-mute.  If the digital input
// auto-mute is enabled, the DSP will only enable the digital inputs if the card
// is syncing to a valid clock on the ADAT or S/PDIF inputs.
//
// If the auto-mute is disabled, the digital inputs are enabled regardless of
// what the input clock is set or what is connected.
//
//

#define ECHOGALS_FLAG_BADBOARD					0x0001	// Board could not be init or
																		// stopped responding
																		// (i.e. DSP crashed)

#define ECHOGALS_FLAG_SYNCH_WAVE					0x0004	// Used to enable wave device
																		// synchronization.  When set,
																		// all open wave devices for
																		// a single task start when the
																		// start for the last channel
																		// is received.

#define ECHOGALS_FLAG_SAMPLE_RATE_LOCKED		0x0010	// Set true if the rate has been locked

#define ECHOGALS_ROFLAG_DIGITAL_IN_AUTOMUTE	0x4000	// Set if the console needs to handle
																		// the digital input auto-mute

#define ECHOGALS_ROFLAG_SUPER_INTERLEAVE_OK	0x8000	// Set if this card can handle super
																		// interleave

#define ECHOGALS_FLAG_WRITABLE_MASK				0x3fff
																		
																	

//*****************************************************************************
//
// ECHOSTATUS return type - most of the generic driver functions
// return an ECHOSTATUS value
//
//*****************************************************************************

typedef unsigned long	ECHOSTATUS;

//
// Return status values
//
//	Changes here require a change in pStatusStrs in CEchoGals.cpp
//
#define ECHOSTATUS_OK									0x00
#define ECHOSTATUS_FIRST								0x00
#define ECHOSTATUS_BAD_FORMAT							0x01
#define ECHOSTATUS_BAD_BUFFER_SIZE					0x02
#define ECHOSTATUS_CANT_OPEN							0x03
#define ECHOSTATUS_CANT_CLOSE							0x04
#define ECHOSTATUS_CHANNEL_NOT_OPEN					0x05
#define ECHOSTATUS_BUSY									0x06
#define ECHOSTATUS_BAD_LEVEL							0x07
#define ECHOSTATUS_NO_MIDI								0x08
#define ECHOSTATUS_CLOCK_NOT_SUPPORTED				0x09
#define ECHOSTATUS_CLOCK_NOT_AVAILABLE				0x0A
#define ECHOSTATUS_BAD_CARDID							0x0B
#define ECHOSTATUS_NOT_SUPPORTED						0x0C
#define ECHOSTATUS_BAD_NOTIFY_SIZE					0x0D
#define ECHOSTATUS_INVALID_PARAM						0x0E
#define ECHOSTATUS_NO_MEM								0x0F
#define ECHOSTATUS_NOT_SHAREABLE						0x10
#define ECHOSTATUS_FIRMWARE_LOADED					0x11
#define ECHOSTATUS_DSP_DEAD							0x12
#define ECHOSTATUS_DSP_TIMEOUT						0x13
#define ECHOSTATUS_INVALID_CHANNEL					0x14
#define ECHOSTATUS_CHANNEL_ALREADY_OPEN			0x15
#define ECHOSTATUS_DUCK_FULL							0x16
#define ECHOSTATUS_INVALID_INDEX						0x17
#define ECHOSTATUS_BAD_CARD_NAME 					0x18
#define ECHOSTATUS_IRQ_NOT_OURS						0x19
#define ECHOSTATUS_NO_DATA								0x1E
#define ECHOSTATUS_BUFFER_OVERFLOW					0x1F
#define ECHOSTATUS_OPERATION_CANCELED				0x20
#define ECHOSTATUS_EVENT_NOT_OPEN					0x21
#define ECHOSTATUS_ASIC_NOT_LOADED					0x22
#define ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED	0x23
#define ECHOSTATUS_RESERVED							0x24
#define ECHOSTATUS_BAD_COOKIE							0x25
#define ECHOSTATUS_MIXER_DISABLED					0x26
#define ECHOSTATUS_NO_SUPER_INTERLEAVE				0x27
#define ECHOSTATUS_DUCK_NOT_WRAPPED					0x28
#define ECHOSTATUS_NO_3G_BOX							0x29
#define ECHOSTATUS_LAST									0x2a


//*****************************************************************************
//
// ECHOGALS_AUDIOFORMAT describes the audio buffer format for a pipe
//
//
// byMonoToStereo is used to resolve an ambiguity when wDataInterleave is set
// to one.  Say you're writing a Windows driver and someone tells you to play mono data;
// what they really mean is that they want the same signal sent out of both left
// and right channels.
// 
// Now, say you're writing an ASIO driver and the ASIO host wants mono data
// sent out of a single output only.  byMonoToStereo is a flag used to resolve
// this; if byMonoToStereo is non-zero, then the same signal is sent out both channels.
// If byMonoToStereo is zero, then only one output channel is used.
//
//*****************************************************************************

typedef struct tECHOGALS_AUDIOFORMAT
{
	WORD		wDataInterleave;				// How the data is arranged in memory
													// Mono = 1, stereo = 2
	WORD		wBitsPerSample;				// 8, 16, 32
	BYTE		byMonoToStereo;				// Only used if wDataInterleave is 1 and
													// if this is an output pipe.
	BYTE		byDataAreBigEndian;			// 1 = Apple, 0 = Intel
} ECHOGALS_AUDIOFORMAT, * PECHOGALS_AUDIOFORMAT;


//
//	All kinds of peak and VU meters.  Only cards with virtual outputs will
// have output pipe meters.  
//
//	All data are scaled integers in units of dB X 256
//
typedef struct tECHOGALS_METERS
{
	INT32	iNumPipesOut;	// These numbers only apply in the context of this structure;
	INT32	iNumPipesIn;	// they indicate the number of entries in each of the arrays.
	INT32	iNumBussesOut;
	INT32	iNumBussesIn;

	INT32	iPipeOutVU[ECHO_MAXAUDIOOUTPUTS];
	INT32	iPipeOutPeak[ECHO_MAXAUDIOOUTPUTS];

	INT32	iPipeInVU[ECHO_MAXAUDIOINPUTS];
	INT32	iPipeInPeak[ECHO_MAXAUDIOINPUTS];

	INT32	iBusOutVU[ECHO_MAXAUDIOOUTPUTS];
	INT32	iBusOutPeak[ECHO_MAXAUDIOOUTPUTS];
	
	INT32	iBusInVU[ECHO_MAXAUDIOINPUTS];
	INT32	iBusInPeak[ECHO_MAXAUDIOINPUTS];
} ECHOGALS_METERS, * PECHOGALS_METERS;

//
// ECHOGALS_AUDIO_PIPE describes a single pipe.
//
// Note that nPipe is *not* a pipe index; it's just the number for the pipe.  
// This is meant to make life easier for you, the driver developer; the code 
// is meant to be set up that you don't have to go around calculating the 
// pipe index.
//
// In other words, if you want to specify the first input pipe, do this:
// 
// nPipe = 0
// bIsInput = TRUE
//
// Don't set nPipe equal to the number of output pipes, since it's not a pipe
// index.
//

typedef struct tECHOGALS_AUDIO_PIPE
{
	WORD		nPipe;					// Pipe number (not a pipe index!)
	BOOL		bIsInput;				// Set TRUE for input pipe, FALSE for output
	WORD		wInterleave;			// Interleave factor for this pipe
											// Mono = 1, stereo = 2, 5.1 surround = 6
} ECHOGALS_AUDIO_PIPE, * PECHOGALS_AUDIO_PIPE;


//
// This structure is used to open a pipe and reserve it for your exclusive use.
//
// Note that if you set wInterleave in the ECHOGALS_AUDIO_PIPE sub-struct
// to greater than one, you are actually reserving more than one pipe; in fact
// you are reserving the pipe specified by the nPipe field as well as
// the next wInterleave-1 pipes.
//
// ProcessId is used in specific situations in the Windows driver where
// several pipes are opened as different devices, but you want them all
// to start synchronously.  The driver attempts to start the pipes in groups
// based on the ProcessId field.
//
// If you don't want to hassle with this, just set ProcessId to NULL; the
// generic code will then ignore the ProcessId.
// 
// ProcessId is also ignored if you clear the ECHOGALS_FLAG_SYNCH_WAVE flag.
//

typedef struct tECHOGALS_OPENAUDIOPARAMETERS
{
	ECHOGALS_AUDIO_PIPE		Pipe;			// Pipe descriptor
	BOOL							bIsCyclic;	// Set TRUE if using circular buffers
													// Set FALSE for linked list of use
													// once and discard buffers
	PVOID							ProcessId;	// Caller process ID for implementing
													// synchronous wave start
} ECHOGALS_OPENAUDIOPARAMETERS, * PECHOGALS_OPENAUDIOPARAMETERS;


//
// Yes, it's the world's simplest structure.
//
typedef struct tECHOGALS_CLOSEAUDIOPARAMETERS
{
	WORD		wPipeIndex;
} ECHOGALS_CLOSEAUDIOPARAMETERS, * PECHOGALS_CLOSEAUDIOPARAMETERS;

//
// Context info for MIDI input clients
//
typedef struct tECHOGALS_MIDI_IN_CONTEXT
{
	BOOL	fOpen;
	DWORD dwDrain;
} ECHOGALS_MIDI_IN_CONTEXT;

//
// One nifty feature is that the DSP is constantly writing the DMA position
// for each pipe to the comm page without the driver having to do anything.  
// You can therefore obtain a pointer to this location and directly read
// the position from anywhere.
//
// The pointer returned should be treated as a read-only pointer and is
// kind of dangerous, since it points directly into the driver.
//
// Note that the value pointed to by pdwPosition will be in little-endian
// format.
//
// The value pointed to by pdwPosition is the number of bytes transported
// by the DSP; it will constantly increase until it hits the 32-bit boundary
// and then wraps around.  It does *not* represent the offset into a circular
// DMA buffer; this value is treated identically for cyclic and non-cyclic 
// buffer schemes.
//
typedef struct tECHOGALS_POSITION
{
	PDWORD	pdwPosition;
} ECHOGALS_POSITION, * PECHOGALS_POSITION;


/*

	ECHOGALS_CAPS

	This structure is used to differentiate one card from another.
	
	szName					Zero-terinated char string; unique name for 
								this card.

	wCardType				One of the DARLA, GINA, etc. card IDs at the
								top of this header file

	dwOutClockTypes		This is a bit mask describing all the different
								output clock types that can be set.  If the bit mask
								is zero, then the output clock cannot be set on this
								card.  The bits will be one of the ECHO_CLOCK_BIT_???
								defines above.

	dwInClockTypes			Again, a bit mask describing what this card can
								sync to.  This mask is fixed, regardless of what is
								or is not currently connected to the hardware.  Again,
								this uses the ECHO_CLOCK_BIT_??? defines.

	dwDigitalModes			Some cards (right now, Gina24, Layla24 and Mona) have
								what we call a Digital Mode Switch or DMS.  
								Use the ECHOCAPS_HAS_DIGITAL_MODE_??? defines to determine
								which digital modes this card supports.

	fHasVmixer				Boolean flag- if this field is non-zero, then this card
								supports virtual outputs.

	wNumPipesOut			Should be self-explanatory, given the definitions of
	wNumPipesIn				pipes and busses above.  Note that these are all in terms
	wNumBussesOut			of mono pipes and busses.
	wNumBussesIn

	wFirstDigitalBusOut	Gives you the number of, well, the first bus that
								goes out an S/PDIF or ADAT output.  If this card has
								no digital outputs, wFirstDigitalBusOut will be equal
								to wNumBussesOut.  This may seem strange, but remember
								that the busses are numbered starting at zero and that
								valid bus numbers therefore range from zero to 
								(wNumBussesOut - 1).  Doing it this way actually
								turned out to simplify writing consoles and so forth.

	wFirstDigitalBusIn	Just like the last one, but for inputs

	wFirstDigitalPipeOut	Gives you the number of the first pipe that is connected
								to a digital output bus.  If this card has no digital output
								busses, this will be equal to wNumPipesOut.  Cards with
								virtual outputs will always have this member set equal
								to wNumPipesOut.

	wFirstDigitalPipeIn	Again, just like wFirstDigitalPipeOut, but for inputs.

	dwPipeOutCaps[]		Array of bit fields with information about each output
								pipe.  This tells you what controls are available for
								each output pipe, as well as whether or not this pipe
								is connected to a digital output.  Use the ECHOCAPS_???
								bits above to parse this bit field.

	dwPipeInCaps[]			More arrays for the other busses and pipes; works
	dwBusOutCaps[]			just like dwPipeOutCaps.
	dwBusInCaps[]

	wNumMidiOut				Number of MIDI inputs and outputs on this card.
	wNumMidiIn				

 */

enum
{
	ECHOCAPS_GAIN 				= 0x00000001,
	ECHOCAPS_MUTE				= 0x00000004,
	ECHOCAPS_PAN				= 0x00000010,
	ECHOCAPS_PEAK_METER		= 0x00000020,
	ECHOCAPS_VU_METER 		= 0x00000040,
	ECHOCAPS_NOMINAL_LEVEL	= 0x00000080,	// aka +4/-10
	ECHOCAPS_PHANTOM_POWER	= 0x20000000,
	ECHOCAPS_DUMMY				= 0x40000000,	// Used for Indigo; indicates
														// a dummy input or output
	ECHOCAPS_DIGITAL			= 0x80000000 	// S/PDIF or ADAT connection
};


typedef struct tECHOGALS_CAPS
{
	CHAR 		szName[ ECHO_MAXNAMELEN ];	// Name of this card
	WORD		wCardType;						// Type of card
	DWORD		dwOutClockTypes;				// Types of output clocks
	DWORD		dwInClockTypes;				// Types of input clocks
	DWORD		dwDigitalModes;				// Digital modes supported
	BOOL		fHasVmixer;
	
	WORD		wNumPipesOut;
	WORD		wNumPipesIn;
	WORD		wNumBussesOut;
	WORD		wNumBussesIn;
	
	WORD		wFirstDigitalBusOut;			// Equal to wNumBussesOut if this card has 
													// no digital output busses

	WORD		wFirstDigitalBusIn;			// Equal to wNumBussesIn if this card has 
													// no digital input busses													
													
	WORD		wFirstDigitalPipeOut;		// Works just like wFirstDigitalBus... above
	WORD		wFirstDigitalPipeIn;													
	
	DWORD		dwPipeOutCaps[ ECHO_MAXAUDIOOUTPUTS ];
	DWORD		dwPipeInCaps[ ECHO_MAXAUDIOINPUTS ];
	DWORD		dwBusOutCaps[ ECHO_MAXAUDIOOUTPUTS ];
	DWORD		dwBusInCaps[ ECHO_MAXAUDIOINPUTS ];

	WORD		wNumMidiOut;					// Number of MIDI out jacks
	WORD		wNumMidiIn;						// Number of MIDI in jacks
} ECHOGALS_CAPS, *PECHOGALS_CAPS;


/*

	Echo3G box types

*/

//
// 3G external box types
//
#define GINA3G									0x00
#define LAYLA3G								0x10
#define NO3GBOX								0xffff


#endif // _ECHOGALSXFACE_


// end file EchoGalsXface.h

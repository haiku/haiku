// ****************************************************************************
//
//		CDspCommObject.H
//
//		Include file for EchoGals generic driver DSP interface base class.
//
// ----------------------------------------------------------------------------
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

#ifndef _DSPCOMMOBJECT_
#define _DSPCOMMOBJECT_

#ifdef _DEBUG
#ifdef ECHO_WDM
#pragma optimize("",off)
#endif
#endif

#ifdef _WIN32

//	Must match structure alignment w/DSP
#pragma pack( push, 2 )

#endif

#include "OsSupport.h"
#include "CDaffyDuck.h"

/****************************************************************************

	Lots of different defines for the different cards

 ****************************************************************************/


//==================================================================================
//
// Macros to convert to and from generic driver mixer values.  Since it can be tough
// to do floating point math in a driver, the generic driver uses fixed-point values.
// The numbers are in a 24.8 format; that is, the upper 24 bits are the integer part
// of the number and the lower 8 bits represent the fractional part.  In this scheme,
// a value of 0x180 would be equal to 1.5.
// 
// Since the DSP usually wants 8 bit integer gains, the following macros are useful.
//
//==================================================================================

#define GENERIC_TO_DSP(iValue) 	((iValue + 0x80) >> 8)	
#define DSP_TO_GENERIC(iValue)	(iValue << 8)


//==================================================================================
//
//	Max inputs and outputs
//
//==================================================================================

#define DSP_MAXAUDIOINPUTS			16				// Max audio input channels
#define DSP_MAXAUDIOOUTPUTS		16				// Max audio output channels
#define DSP_MAXPIPES					32				// Max total pipes (input + output)


//==================================================================================
//
//	These are the offsets for the memory-mapped DSP registers; the DSP base
// address is treated as the start of a DWORD array.
//
//==================================================================================

#define	CHI32_CONTROL_REG					4
#define	CHI32_STATUS_REG					5
#define	CHI32_VECTOR_REG					6
#define	CHI32_DATA_REG						7


//==================================================================================
//
//	Interesting bits within the DSP registers
//
//==================================================================================

#define	CHI32_VECTOR_BUSY					0x00000001
#define	CHI32_STATUS_REG_HF3				0x00000008
#define	CHI32_STATUS_REG_HF4				0x00000010
#define	CHI32_STATUS_REG_HF5				0x00000020
#define	CHI32_STATUS_HOST_READ_FULL	0x00000004
#define	CHI32_STATUS_HOST_WRITE_EMPTY	0x00000002
#define 	CHI32_STATUS_IRQ			      0x00000040


//==================================================================================
//
// DSP commands sent via slave mode; these are sent to the DSP by 
// CDspCommObject::Write_DSP
//
//==================================================================================

#define  DSP_FNC_SET_COMMPAGE_ADDR				0x02
#define	DSP_FNC_LOAD_LAYLA_ASIC					0xa0
#define	DSP_FNC_LOAD_GINA24_ASIC				0xa0
#define 	DSP_FNC_LOAD_MONA_PCI_CARD_ASIC		0xa0
#define 	DSP_FNC_LOAD_LAYLA24_PCI_CARD_ASIC	0xa0
#define 	DSP_FNC_LOAD_MONA_EXTERNAL_ASIC		0xa1
#define 	DSP_FNC_LOAD_LAYLA24_EXTERNAL_ASIC	0xa1
#define  DSP_FNC_LOAD_3G_ASIC						0xa0


//==================================================================================
//
// Defines to handle the MIDI input state engine; these are used to properly
// extract MIDI time code bytes and their timestamps from the MIDI input stream.
//
//==================================================================================

#define	MIDI_IN_STATE_NORMAL				0
#define	MIDI_IN_STATE_TS_HIGH			1
#define	MIDI_IN_STATE_TS_LOW				2
#define	MIDI_IN_STATE_F1_DATA			3

#define	MIDI_IN_SKIP_DATA					((DWORD)-1)


/*------------------------------------------------------------------------------------

Setting the sample rates on Layla24 is somewhat schizophrenic.

For standard rates, it works exactly like Mona and Gina24.  That is, for
8, 11.025, 16, 22.05, 32, 44.1, 48, 88.2, and 96 kHz, you just set the 
appropriate bits in the control register and write the control register.

In order to support MIDI time code sync (and possibly SMPTE LTC sync in 
the future), Layla24 also has "continuous sample rate mode".  In this mode, 
Layla24 can generate any sample rate between 25 and 50 kHz inclusive, or 
50 to 100 kHz inclusive for double speed mode.

To use continuous mode:

-Set the clock select bits in the control register to 0xe (see the #define 
 below)

-Set double-speed mode if you want to use sample rates above 50 kHz

-Write the control register as you would normally

-Now, you need to set the frequency register. First, you need to determine the
 value for the frequency register.  This is given by the following formula:

frequency_reg = (LAYLA24_MAGIC_NUMBER / sample_rate) - 2

Note the #define below for the magic number

-Wait for the DSP handshake
-Write the frequency_reg value to the dwSampleRate field of the comm page
-Send the vector command SET_LAYLA24_FREQUENCY_REG (see vmonkey.h)

Once you have set the control register up for continuous mode, you can just 
write the frequency register to change the sample rate.  This could be
used for MIDI time code sync. For MTC sync, the control register is set for 
continuous mode.  The driver then just keeps writing the 
SET_LAYLA24_FREQUENCY_REG command.

----------------------------------------------------------------------------------*/

#define	LAYLA24_MAGIC_NUMBER						677376000
#define	LAYLA24_CONTINUOUS_CLOCK				0x000e


//==================================================================================
//
// DSP vector commands
//
//==================================================================================

#define	DSP_VC_RESET							0x80ff

#ifndef DSP_56361

//
// Vector commands for families that only use the 56301
// Only used for Darla20, Gina20, Layla20, and Darla24
//
#define	DSP_VC_ACK_INT							0x8073
#define	DSP_VC_SET_VMIXER_GAIN				0x0000	// Not used, only for compile
#define	DSP_VC_START_TRANSFER				0x0075	// Handshke rqd.
#define	DSP_VC_METERS_ON						0x0079
#define	DSP_VC_METERS_OFF						0x007b
#define	DSP_VC_UPDATE_OUTVOL					0x007d	// Handshke rqd.
#define	DSP_VC_UPDATE_INGAIN					0x007f	// Handshke rqd.
#define	DSP_VC_ADD_AUDIO_BUFFER				0x0081	// Handshke rqd.
#define	DSP_VC_TEST_ASIC						0x00eb
#define	DSP_VC_UPDATE_CLOCKS					0x00ef	// Handshke rqd.
#define 	DSP_VC_SET_LAYLA_SAMPLE_RATE		0x00f1	// Handshke rqd.
#define	DSP_VC_SET_GD_AUDIO_STATE			0x00f1	// Handshke rqd.
#define	DSP_VC_WRITE_CONTROL_REG			0x00f1	// Handshke rqd.
#define 	DSP_VC_MIDI_WRITE						0x00f5	// Handshke rqd.
#define 	DSP_VC_STOP_TRANSFER					0x00f7	// Handshke rqd.
#define	DSP_VC_UPDATE_FLAGS					0x00fd	// Handshke rqd.
#define 	DSP_VC_GO_COMATOSE					0x00f9

#else

//
// Vector commands for families that use either the 56301 or 56361
//
#define	DSP_VC_ACK_INT							0x80F5
#define	DSP_VC_SET_VMIXER_GAIN				0x00DB	// Handshke rqd.
#define	DSP_VC_START_TRANSFER				0x00DD	// Handshke rqd.
#define	DSP_VC_METERS_ON						0x00EF
#define	DSP_VC_METERS_OFF						0x00F1
#define	DSP_VC_UPDATE_OUTVOL					0x00E3	// Handshke rqd.
#define	DSP_VC_UPDATE_INGAIN					0x00E5	// Handshke rqd.
#define	DSP_VC_ADD_AUDIO_BUFFER				0x00E1	// Handshke rqd.
#define	DSP_VC_TEST_ASIC						0x00ED
#define	DSP_VC_UPDATE_CLOCKS					0x00E9	// Handshke rqd.
#define	DSP_VC_SET_LAYLA24_FREQUENCY_REG	0x00E9	// Handshke rqd.
#define 	DSP_VC_SET_LAYLA_SAMPLE_RATE		0x00EB	// Handshke rqd.
#define	DSP_VC_SET_GD_AUDIO_STATE			0x00EB	// Handshke rqd.
#define	DSP_VC_WRITE_CONTROL_REG			0x00EB	// Handshke rqd.
#define 	DSP_VC_MIDI_WRITE						0x00E7	// Handshke rqd.
#define 	DSP_VC_STOP_TRANSFER					0x00DF	// Handshke rqd.
#define	DSP_VC_UPDATE_FLAGS					0x00FB	// Handshke rqd.
#define 	DSP_VC_GO_COMATOSE					0x00d9

#ifdef SUPERMIX
#define	DSP_VC_TRIGGER							0x00f3
#endif

#endif


//==================================================================================
//
//	Timeouts
//
//==================================================================================

#define HANDSHAKE_TIMEOUT		20000		//	SendVector command timeout in microseconds
#define MIDI_OUT_DELAY_USEC	2000		// How long to wait after MIDI fills up


//==================================================================================
//
//	Flags for dwFlags field in the comm page
//
//==================================================================================

#define	DSP_FLAG_MIDI_INPUT				0x0001	// Enable MIDI input
#define	DSP_FLAG_SPDIF_NONAUDIO			0x0002	// Sets the "non-audio" bit in the S/PDIF out
																// status bits.  Clear this flag for audio data;
																// set it for AC3 or WMA or some such
#define	DSP_FLAG_PROFESSIONAL_SPDIF	0x0008	// 1 Professional, 0 Consumer

#ifdef SUPERMIX
#define	DSP_FLAG_SUPERMIX_SRC			0x0080	// Turn on sample rate conversion for supermixer
#endif



//==================================================================================
//
//	Clock detect bits reported by the DSP for Gina20, Layla20, Darla24, and Mia
//
//==================================================================================

#define GLDM_CLOCK_DETECT_BIT_WORD		0x0002
#define GLDM_CLOCK_DETECT_BIT_SUPER		0x0004
#define GLDM_CLOCK_DETECT_BIT_SPDIF		0x0008
#define GLDM_CLOCK_DETECT_BIT_ESYNC		0x0010


//==================================================================================
//
//	Clock detect bits reported by the DSP for Gina24, Mona, and Layla24
//
//==================================================================================

#define GML_CLOCK_DETECT_BIT_WORD96		0x0002
#define GML_CLOCK_DETECT_BIT_WORD48		0x0004
#define GML_CLOCK_DETECT_BIT_SPDIF48	0x0008
#define GML_CLOCK_DETECT_BIT_SPDIF96	0x0010
#define GML_CLOCK_DETECT_BIT_WORD		(GML_CLOCK_DETECT_BIT_WORD96|GML_CLOCK_DETECT_BIT_WORD48)
#define GML_CLOCK_DETECT_BIT_SPDIF		(GML_CLOCK_DETECT_BIT_SPDIF48|GML_CLOCK_DETECT_BIT_SPDIF96)
#define GML_CLOCK_DETECT_BIT_ESYNC		0x0020
#define GML_CLOCK_DETECT_BIT_ADAT		0x0040


//==================================================================================
//
//	Gina/Darla clock states
//
//==================================================================================

#define GD_CLOCK_NOCHANGE			0
#define GD_CLOCK_44					1
#define GD_CLOCK_48					2
#define GD_CLOCK_SPDIFIN			3
#define GD_CLOCK_UNDEF				0xff


//==================================================================================
//
//	Gina/Darla S/PDIF status bits
//
//==================================================================================

#define GD_SPDIF_STATUS_NOCHANGE	0
#define GD_SPDIF_STATUS_44			1
#define GD_SPDIF_STATUS_48			2
#define GD_SPDIF_STATUS_UNDEF		0xff


//==================================================================================
//
//	Layla20 output clocks
//
//==================================================================================

#define LAYLA20_OUTPUT_CLOCK_SUPER	0
#define LAYLA20_OUTPUT_CLOCK_WORD	1


//==================================================================================
//
//	Return values from the DSP when ASIC is loaded
//
//==================================================================================

#define ASIC_LOADED					0x1
#define ASIC_NOT_LOADED				0x0


//==================================================================================
//
////	DSP Audio formats
//
// These are the audio formats that the DSP can transfer
// via input and output pipes.  LE means little-endian,
// BE means big-endian.
//
// DSP_AUDIOFORM_MS_8	
//
// 	8-bit mono unsigned samples.  For playback,
//		mono data is duplicated out the left and right channels
// 	of the output bus.  The "MS" part of the name
//		means mono->stereo.
//
//	DSP_AUDIOFORM_MS_16LE
//
//		16-bit signed little-endian mono samples.  Playback works
//		like the previous code.
//
//	DSP_AUDIOFORM_MS_24LE
//
//		24-bit signed little-endian mono samples.  Data is packed
//    three bytes per sample; if you had two samples 0x112233 and 0x445566
//    they would be stored in memory like this: 33 22 11 66 55 44.
//
//	DSP_AUDIOFORM_MS_32LE
//	
//		24-bit signed little-endian mono samples in a 32-bit 
//		container.  In other words, each sample is a 32-bit signed 
//		integer, where the actual audio data is left-justified 
//		in the 32 bits and only the 24 most significant bits are valid.
//
//	DSP_AUDIOFORM_SS_8
//	DSP_AUDIOFORM_SS_16LE
// DSP_AUDIOFORM_SS_24LE
//	DSP_AUDIOFORM_SS_32LE
//
//		Like the previous ones, except now with stereo interleaved
//		data.  "SS" means stereo->stereo.
//
// DSP_AUDIOFORM_MM_32LE
//
//		Similar to DSP_AUDIOFORM_MS_32LE, except that the mono
//		data is not duplicated out both the left and right outputs.
//		This mode is used by the ASIO driver.  Here, "MM" means
//		mono->mono.
//
//	DSP_AUDIOFORM_MM_32BE
//
//		Just like DSP_AUDIOFORM_MM_32LE, but now the data is
//		in big-endian format.
//
//==================================================================================

#define DSP_AUDIOFORM_MS_8			0		// 8 bit mono
#define DSP_AUDIOFORM_MS_16LE		1		// 16 bit mono
#define DSP_AUDIOFORM_MS_24LE		2		// 24 bit mono
#define DSP_AUDIOFORM_MS_32LE		3		// 32 bit mono
#define DSP_AUDIOFORM_SS_8			4		// 8 bit stereo
#define DSP_AUDIOFORM_SS_16LE		5		// 16 bit stereo
#define DSP_AUDIOFORM_SS_24LE		6		// 24 bit stereo
#define DSP_AUDIOFORM_SS_32LE		7		// 32 bit stereo
#define DSP_AUDIOFORM_MM_32LE		8		// 32 bit mono->mono little-endian
#define DSP_AUDIOFORM_MM_32BE		9		// 32 bit mono->mono big-endian
#define DSP_AUDIOFORM_SS_32BE		10		// 32 bit stereo big endian
#define DSP_AUDIOFORM_INVALID		0xFF	// Invalid audio format


//==================================================================================
//
// Super-interleave is defined as interleaving by 4 or more.  Darla20 and Gina20
// do not support super interleave.
//
// 16 bit, 24 bit, and 32 bit little endian samples are supported for super 
// interleave.  The interleave factor must be even.  16 - way interleave is the 
// current maximum, so you can interleave by 4, 6, 8, 10, 12, 14, and 16.
//
// The actual format code is derived by taking the define below and or-ing with
// the interleave factor.  So, 32 bit interleave by 6 is 0x86 and 
// 16 bit interleave by 16 is (0x40 | 0x10) = 0x50.
//
//==================================================================================

#define DSP_AUDIOFORM_SUPER_INTERLEAVE_16LE	0x40
#define DSP_AUDIOFORM_SUPER_INTERLEAVE_24LE	0xc0
#define DSP_AUDIOFORM_SUPER_INTERLEAVE_32LE	0x80


//==================================================================================
//
//	Gina24, Mona, and Layla24 control register defines
//
//==================================================================================

#define GML_CONVERTER_ENABLE		0x0010
#define GML_SPDIF_PRO_MODE			0x0020		// Professional S/PDIF == 1, consumer == 0
#define GML_SPDIF_SAMPLE_RATE0	0x0040
#define GML_SPDIF_SAMPLE_RATE1	0x0080
#define GML_SPDIF_TWO_CHANNEL		0x0100		// 1 == two channels, 0 == one channel
#define GML_SPDIF_NOT_AUDIO		0x0200
#define GML_SPDIF_COPY_PERMIT		0x0400
#define GML_SPDIF_24_BIT			0x0800		// 1 == 24 bit, 0 == 20 bit
#define GML_ADAT_MODE				0x1000		// 1 == ADAT mode, 0 == S/PDIF mode
#define GML_SPDIF_OPTICAL_MODE	0x2000		// 1 == optical mode, 0 == RCA mode
#define GML_DOUBLE_SPEED_MODE		0x4000		// 1 == double speed, 0 == single speed			

#define GML_DIGITAL_IN_AUTO_MUTE	0x800000

#define GML_96KHZ						(0x0 | GML_DOUBLE_SPEED_MODE)
#define GML_88KHZ						(0x1 | GML_DOUBLE_SPEED_MODE)
#define GML_48KHZ						0x2
#define GML_44KHZ						0x3
#define GML_32KHZ						0x4
#define GML_22KHZ						0x5
#define GML_16KHZ						0x6
#define GML_11KHZ						0x7
#define GML_8KHZ						0x8
#define GML_SPDIF_CLOCK				0x9
#define GML_ADAT_CLOCK				0xA
#define GML_WORD_CLOCK				0xB
#define GML_ESYNC_CLOCK				0xC
#define GML_ESYNCx2_CLOCK			0xD

#define GML_CLOCK_CLEAR_MASK			0xffffbff0
#define GML_SPDIF_RATE_CLEAR_MASK   (~(GML_SPDIF_SAMPLE_RATE0|GML_SPDIF_SAMPLE_RATE1))
#define GML_DIGITAL_MODE_CLEAR_MASK	0xffffcfff
#define GML_SPDIF_FORMAT_CLEAR_MASK	0xfffff01f


//==================================================================================
//
//	Mia sample rate and clock setting constants
// 
//==================================================================================

#define MIA_32000		0x0040
#define MIA_44100		0x0042
#define MIA_48000		0x0041						   
#define MIA_88200		0x0142
#define MIA_96000		0x0141

#define MIA_SPDIF		0x00000044
#define MIA_SPDIF96	0x00000144

#define MIA_MIDI_REV	1				// Must be Mia rev 1 for MIDI support


//==================================================================================
//
// Gina20 & Layla20 have input gain controls for the analog inputs;
// this is the magic number for the hardware that gives you 0 dB at -10.
//
//==================================================================================

#define GL20_INPUT_GAIN_MAGIC_NUMBER	0xC8


//==================================================================================
//
//	Defines how much time must pass between DSP load attempts
//
//==================================================================================

#define DSP_LOAD_ATTEMPT_PERIOD	1000000L	// One million microseconds == one second


//==================================================================================
//
// Size of arrays for the comm page.  MAX_PLAY_TAPS and MAX_REC_TAPS are no longer 
// used, but the sizes must still be right for the DSP to see the comm page correctly.
//
//==================================================================================

#define MONITOR_ARRAY_SIZE			0x180
#define VMIXER_ARRAY_SIZE			0x40
#define CP_MIDI_OUT_BUFFER_SIZE	32
#define CP_MIDI_IN_BUFFER_SIZE 	256
#define MAX_PLAY_TAPS				168
#define MAX_REC_TAPS					192

#define DSP_MIDI_OUT_FIFO_SIZE	64


//==================================================================================
//
//	Macros for reading and writing DSP registers
//
//==================================================================================

#ifndef READ_REGISTER_ULONG
#define READ_REGISTER_ULONG(ptr)		( *(ptr) )
#endif

#ifndef WRITE_REGISTER_ULONG
#define WRITE_REGISTER_ULONG(ptr,val)	*(ptr) = val
#endif


/****************************************************************************

	The comm page.  This structure is read and written by the DSP; the
	DSP code is a firm believer in the byte offsets written in the comments
	at the end of each line.  This structure should not be changed.
	
	Any reads from or writes to this structure should be in little-endian
	format.

 ****************************************************************************/

typedef struct
{
	DWORD				dwCommSize;				// size of this object							0x000	4

	DWORD				dwFlags;					// See Appendix A below							0x004	4
	DWORD				dwUnused;				// Unused entry									0x008	4

	DWORD				dwSampleRate;			// Card sample rate in Hz						0x00c	4
	DWORD				dwHandshake;			// DSP command handshake						0x010	4
	CChMaskDsp		cmdStart;				// Chs. to start mask							0x014	4
	CChMaskDsp		cmdStop;					// Chs. to stop mask								0x018	4
	CChMaskDsp		cmdReset;				// Chs. to reset mask							0x01c	4
	WORD				wAudioFormat[ DSP_MAXPIPES ];
              									// Chs. audio format								0x020	16*2*2
	DUCKENTRY		DuckListPhys[ DSP_MAXPIPES ];
     												// Chs. Physical duck addrs					0x060	16*2*8
	DWORD				dwPosition[ DSP_MAXPIPES ];
													// Positions for ea. ch.						0x160	16*2*4
	BYTE				VUMeter[ DSP_MAXPIPES ];
													// VU meters										0x1e0	16*2*1
	BYTE				PeakMeter[ DSP_MAXPIPES ];
													// Peak meters										0x200	16*2*1
	BYTE				OutLineLevel[ DSP_MAXAUDIOOUTPUTS ];
													// Output gain										0x220	16*1
	BYTE				InLineLevel[ DSP_MAXAUDIOINPUTS ];
													// Input gain										0x230	16*1
	BYTE				byMonitors[ MONITOR_ARRAY_SIZE ];
													// Monitor map										0x240	0x180
	DWORD				dwPlayCoeff[ MAX_PLAY_TAPS ];
													// Gina/Darla play filters - obsolete		0x3c0	168*4
	DWORD				dwRecCoeff [ MAX_REC_TAPS ];
													// Gina/Darla record filters - obsolete	0x660	192*4
	WORD				wMidiInData[ CP_MIDI_IN_BUFFER_SIZE ];
													// MIDI input data transfer buffer			0x960	256*2
	BYTE				byGDClockState;		// Chg Gina/Darla clock state					0xb60	4
	BYTE				byGDSpdifStatus;		// Chg. Gina/Darla S/PDIF state
	BYTE				byGDResamplerState;	// Should always be 3
	BYTE				byFiller2;
	CChMaskDsp		cmdNominalLevel;
													// -10 level enable mask						0xb64	4
	WORD				wInputClock; 			// Chg. Input clock state
	WORD				wOutputClock; 			// Chg. Output clock state						0xb68
	DWORD				dwStatusClocks;	 	// Current Input clock state					0xb6c	4

	DWORD				dwExtBoxStatus;		// External box connected or not				0xb70 4
	DWORD				dwUnused2;				// filler											0xb74	4
	DWORD				dwMidiOutFreeCount;	// # of bytes free in MIDI output FIFO		0xb78	4
	DWORD 			dwUnused3;				//                                        0xb7c	4
	DWORD				dwControlReg;			// Mona, Gina24, Layla24 and 3G control 	0xb80 4
	DWORD				dw3gFreqReg;			// 3G frequency register						0xb84	4
	BYTE				byFiller[24];			// filler											0xb88
	BYTE				byVmixerLevel[ VMIXER_ARRAY_SIZE ];
													// Vmixer levels									0xba0 64
	BYTE				byMidiOutData[ CP_MIDI_OUT_BUFFER_SIZE ];
													// MIDI output data								0xbe0	32
} DspCommPage, *PDspCommPage;


/****************************************************************************

	CDspCommObject is the class which wraps both the comm page and the
	DSP registers.  CDspCommObject talks directly to the hardware; anyone
	who wants to do something to the hardware goes through CDspCommObject or
	one of the derived classes.
	
	Note that an instance of CDspCommObject is never actually created; it
	is treated as an abstract base class.

 ****************************************************************************/

class CDspCommObject
{
protected:
	volatile PDspCommPage m_pDspCommPage;		// Physical memory seen by DSP
	PPAGE_BLOCK		m_pDspCommPageBlock;	// Physical memory info for COsSupport

 	//
 	//	These members are not seen by the DSP; they are used internally by
	// this class.
 	//
	WORD				m_wNumPipesOut;
	WORD				m_wNumPipesIn;
	WORD				m_wNumBussesOut;
	WORD				m_wNumBussesIn;		
	WORD				m_wFirstDigitalBusOut;
	WORD				m_wFirstDigitalBusIn;
	
	BOOL				m_fHasVmixer;
	
	WORD				m_wNumMidiOut;			// # MIDI out channels
	WORD				m_wNumMidiIn;			// # MIDI in  channels
	PWORD 			m_pwDspCode;			// Current DSP code loaded, NULL if nothing loaded
	PWORD 			m_pwDspCodeToLoad;	// DSP code to load
	BOOL				m_bHasASIC;				// Set TRUE if card has an ASIC
	BOOL				m_bASICLoaded;			// Set TRUE when ASIC loaded
	DWORD				m_dwCommPagePhys;		// Physical addr of this object
	volatile PDWORD m_pdwDspRegBase;		// DSP's register base
	CChannelMask	m_cmActive;				// Chs. active mask
	BOOL				m_bBadBoard;			// Set TRUE if DSP won't load
													// or punks out
	WORD				m_wMeterOnCount;		// How many times meters have been
													// enabled
	PCOsSupport		m_pOsSupport;			// Ptr to OS specific methods & data
	CHAR				m_szCardName[ 20 ];
	BYTE				m_byDigitalMode;		// Digital mode (see DIGITAL_MODE_?? 
													//	defines in EchoGalsXface.h
	WORD				m_wInputClock;			// Currently selected input clock
	WORD				m_wOutputClock;		// Currently selected output clock
	
	ULONGLONG		m_ullLastLoadAttemptTime;	// Last system time that the driver
															// attempted to load the DSP & ASIC
#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT
	BOOL				m_fDigitalInAutoMute;
#endif

#ifdef MIDI_SUPPORT
	WORD				m_wMidiOnCount;		// Count MIDI enabled cmds
	ULONGLONG		m_ullMidiInTime;		// Last time MIDI in occured
	ULONGLONG		m_ullMidiOutTime;		// Last time MIDI out occured
	ULONGLONG		m_ullNextMidiWriteTime;	// Next time to try MIDI output
	
	WORD				m_wMtcState;			// State for MIDI input parsing state machine
#endif

protected :

	virtual WORD ComputeAudioMonitorIndex
	(
		WORD	wBusOut,
		WORD	wBusIn
	)
	{ 
		return( wBusOut * m_wNumBussesIn + wBusIn ); 
	}

	//
	//	Load code into DSP
	//
#ifdef DSP_56361
	virtual ECHOSTATUS InstallResidentLoader();
#endif
	virtual ECHOSTATUS LoadDSP( PWORD pCode );

	//
	//	Read the serial number from DSP
	//
	virtual ECHOSTATUS	ReadSn();

	//
	//	Load code into ASIC
	//
	virtual BOOL LoadASIC( DWORD dwCmd, PBYTE pCode, DWORD dwSize );
	virtual BOOL LoadASIC() { return TRUE; }

	//
	//	Check status of ASIC - loaded or not loaded
	//
	virtual BOOL CheckAsicStatus();

	//
	//	Write to DSP
	//
	ECHOSTATUS	Write_DSP( DWORD dwData );

	//
	//	Read from DSP
	//
	ECHOSTATUS	Read_DSP( DWORD *pdwData );

	//
	//	Get/Set handshake Flag
	//
	DWORD GetHandshakeFlag()
		{ ECHO_ASSERT( NULL != m_pDspCommPage );
		  return( SWAP( m_pDspCommPage->dwHandshake ) ); }
	void ClearHandshake()
		{ ECHO_ASSERT( NULL != m_pDspCommPage );
		  m_pDspCommPage->dwHandshake = 0; }

	//
	//	Get/set DSP registers
	//
	DWORD GetDspRegister( DWORD dwIndex )
	{ 
		ECHO_ASSERT( NULL != m_pdwDspRegBase );
		
		return READ_REGISTER_ULONG( m_pdwDspRegBase + dwIndex); 
	}
	
	void SetDspRegister( DWORD dwIndex, DWORD dwValue )
	{ 
		ECHO_ASSERT( NULL != m_pdwDspRegBase );
	
		WRITE_REGISTER_ULONG( m_pdwDspRegBase + dwIndex, dwValue);
	}

	//
	//	Set control register in CommPage
	//
	void SetControlRegister( DWORD dwControlRegister )
		{ ECHO_ASSERT( NULL != m_pDspCommPage );
		  m_pDspCommPage->dwControlReg = SWAP( dwControlRegister ); }

	//
	//	Called after load firmware to restore old gains, meters on, monitors, etc.
	//
	virtual void RestoreDspSettings();

	//
	// Send a vector command to the DSP
	//
	ECHOSTATUS SendVector( DWORD dwCommand );

	//
	//	Wait for DSP to finish the last vector command
	//
	BOOL WaitForHandshake();

	//
	// Send new input line setting to DSP
	//
	ECHOSTATUS UpdateAudioInLineLevel();

public:

	//
	//	Construction/destruction
	//
	CDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CDspCommObject();

	//
	//	Card information
	//
	virtual WORD GetCardType() = NULL;
										// Undefined, must be done in derived class
	const PCHAR GetCardName() { return( m_szCardName ); }
										// Must be init in derived class

	//
	// Get mask with active pipes
	//
	void GetActivePipes
	(
		PCChannelMask	pChannelMask
	);

	//
	// Basic info methods
	//
	WORD GetNumPipesOut()
	{
		return m_wNumPipesOut;
	}
	
	WORD GetNumPipesIn()
	{
		return m_wNumPipesIn;
	}
	
	WORD GetNumBussesOut()
	{
		return m_wNumBussesOut;
	}
	
	WORD GetNumBussesIn()
	{
		return m_wNumBussesIn;
	}
	
	WORD GetNumPipes()
	{
		return m_wNumPipesOut + m_wNumPipesIn;
	}
	
	WORD GetNumBusses()
	{
		return m_wNumBussesOut + m_wNumBussesIn;
	}
	
	WORD GetFirstDigitalBusOut()
	{
		return m_wFirstDigitalBusOut;
	}
	
	WORD GetFirstDigitalBusIn()
	{
		return m_wFirstDigitalBusIn;
	}
	
	BOOL HasVmixer()
	{
		return m_fHasVmixer;
	}

	WORD GetNumMidiOutChannels()
		{ return( m_wNumMidiOut ); }
	WORD GetNumMidiInChannels()
		{ return( m_wNumMidiIn ); }
	WORD GetNumMidiChannels()
		{ return( m_wNumMidiIn + m_wNumMidiOut ); }

	//
	// Get, set, and clear comm page flags
	//
	DWORD GetFlags()
		{ 
			return( SWAP( m_pDspCommPage->dwFlags ) );  }
	DWORD SetFlags( DWORD dwFlags )
		{	
			DWORD dwCpFlags = SWAP( m_pDspCommPage->dwFlags );
			dwCpFlags |= dwFlags;
			m_pDspCommPage->dwFlags = SWAP( dwCpFlags );
		
			if ( m_bASICLoaded && WaitForHandshake() )
				UpdateFlags();
		  return( GetFlags() );
		}
	DWORD ClearFlags( DWORD dwFlags )
		{	
			DWORD dwCpFlags = SWAP( m_pDspCommPage->dwFlags );
			dwCpFlags &= ~dwFlags;
			m_pDspCommPage->dwFlags = SWAP( dwCpFlags );
		
			if ( m_bASICLoaded && WaitForHandshake() )
				UpdateFlags();
			return( GetFlags() );
		}

	//
	//	Returns currently selected input clock
	//
	WORD GetInputClock()
	{
		return m_wInputClock;
	}

	//
	//	Returns what input clocks are currently detected
	//
	DWORD GetInputClockDetect()
		{ return( SWAP( m_pDspCommPage->dwStatusClocks ) ); }

	//
	//	Returns currently selected output clock
	//
	WORD GetOutputClock()
	{
		return m_wOutputClock;
	}

	//
	//	Returns control register
	//
	DWORD GetControlRegister()
		{ ECHO_ASSERT( NULL != m_pDspCommPage );
		  return SWAP( m_pDspCommPage->dwControlReg ); }

	//
	//	Set input and output clocks
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);
	virtual ECHOSTATUS SetOutputClock(WORD wClock);
	
	#ifdef COURT8_FAMILY
	virtual ECHOSTATUS SetPhoneBits(DWORD 	dwPhoneBits)
	{
		return ECHOSTATUS_NOT_SUPPORTED;
	}
	#endif

	//
	//	Set digital mode
	//
	virtual ECHOSTATUS SetDigitalMode( BYTE byNewMode )
		{ return ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED; }

	//
	//	Get digital mode
	//
	virtual BYTE GetDigitalMode()
		{ return( m_byDigitalMode ); }

	//
	//	Get mask of all supported digital modes.
	//	(See ECHOCAPS_HAS_DIGITAL_MODE_??? defines in EchoGalsXface.h)
	//
	//	Note: If the card does not have a digital mode switch
	//			then return 0 (no digital modes supported).
	//			Some legacy cards support S/PDIF as their only
	//			digital mode.  We still return 0 here because it
	//			is not switchable.
	//
	virtual DWORD GetDigitalModes()
		{ return( 0 ); }

	//
	//	Return audio channel position in bytes
	//
	DWORD GetAudioPosition( WORD wPipeIndex )
		{ ECHO_ASSERT( wPipeIndex < ECHO_MAXAUDIOPIPES );

		  return( ( wPipeIndex < ECHO_MAXAUDIOPIPES )
							? SWAP( m_pDspCommPage->dwPosition[ wPipeIndex ] )
							: 0  );  }

	//
	// Reset the pipe position for a single pipe
	//							
	void ResetPipePosition(WORD wPipeIndex)
	{
		if (wPipeIndex < ECHO_MAXAUDIOPIPES)
		{
			m_pDspCommPage->dwPosition[ wPipeIndex ] = 0;
		}
	}

	//
	// Warning: Never write to the pointer returned by this
	// function!!!
	//
	//	The data pointed to by this pointer is in little-
	// endian format.
	//
	PDWORD GetAudioPositionPtr()
		{ return( m_pDspCommPage->dwPosition ); }

	//
	// Get the current sample rate
	//
	virtual DWORD GetSampleRate()
	    { return( SWAP( m_pDspCommPage->dwSampleRate ) ); }

	//
	//	Set the sample rate.
	//	Return rate that was set, 0xffffffff if error
	//
	virtual DWORD SetSampleRate( DWORD dwNewSampleRate ) = NULL;

	//
	//	Send current setting to DSP & return what it is
	//
	virtual DWORD SetSampleRate() = NULL;

	//
	// Start a group of pipes
	//
	ECHOSTATUS StartTransport
	(
		PCChannelMask	pChannelMask		// Pipes to start
	);

	//
	// Stop a group of pipes
	//
	ECHOSTATUS StopTransport
	(
		PCChannelMask	pChannelMask
	);

	//
	// Reset a group of pipes
	//
	ECHOSTATUS ResetTransport
	(
		PCChannelMask	pChannelMask
	);

#ifdef SUPERMIX	
	//
	// Trigger this card for synced transport; used for Supermix mode
	//
	ECHOSTATUS TriggerTransport();
#endif

	//
	//	See if any pipes are playing or recording
	// 
	BOOL IsTransportActive()
	{
		return (FALSE == m_cmActive.IsEmpty());
	}

	//
	// Tell DSP we added a buffer to a channel
	//
	ECHOSTATUS AddBuffer( WORD wPipeIndex );

	//
	// Add start of duck list for one channel to commpage so DSP can read it.
	//
	void SetAudioDuckListPhys( WORD wPipeIndex, DWORD dwNewPhysAdr );
	
	//
	// Read extended status register from the DSP
	//
	DWORD GetStatusReg()
	{ 
		return READ_REGISTER_ULONG( m_pdwDspRegBase + CHI32_STATUS_REG ); 
	}

	//
	// Tell DSP to release the hardware interrupt
	//
	void AckInt()
	{ 
		m_pDspCommPage->wMidiInData[ 0 ] = 0;
		SendVector( DSP_VC_ACK_INT ); 
	}

	//
	// Overload new & delete so memory for this object is allocated
	// from contiguous non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 

	//
	//	Get status of board
	//
	BOOL IsBoardBad()
		{ return( m_bBadBoard ); }

	//
	//	Tell DSP flags have been updated
	//
	ECHOSTATUS UpdateFlags()
	{ 
		ECHO_DEBUGPRINTF(("CDspCommObject::UpdateFlags\n"));
		ClearHandshake();
		return( SendVector( DSP_VC_UPDATE_FLAGS ) ); 
	}
	
	//
	//	Get/Set professional or consumer S/PDIF status
	//
	virtual BOOL IsProfessionalSpdif()
		{ 
			ECHO_DEBUGPRINTF(("CDspCommObject::IsProfessionalSpdif - flags are 0x%lx\n",
									GetFlags()));
			return( ( GetFlags() & DSP_FLAG_PROFESSIONAL_SPDIF ) ? TRUE : FALSE ); 
		}

	virtual void SetProfessionalSpdif( BOOL bNewStatus )
		{	
			ECHO_DEBUGPRINTF(("CDspCommObject::SetProfessionalSpdif %d\n",bNewStatus));
			if ( 0 != bNewStatus )
				SetFlags( DSP_FLAG_PROFESSIONAL_SPDIF );
			else
				ClearFlags( DSP_FLAG_PROFESSIONAL_SPDIF );
			
			ECHO_DEBUGPRINTF(("CDspCommObject::SetProfessionalSpdif - flags are now 0x%lx\n",
									GetFlags()));	
		}
		
	//
	// Get/Set S/PDIF out non-audio status bit
	//
	virtual BOOL IsSpdifOutNonAudio()
	{
			return( ( GetFlags() & DSP_FLAG_SPDIF_NONAUDIO ) ? TRUE : FALSE ); 
	}
	
	virtual void SetSpdifOutNonAudio( BOOL bNonAudio)
	{
			if ( 0 != bNonAudio )
				SetFlags( DSP_FLAG_SPDIF_NONAUDIO );
			else
				ClearFlags( DSP_FLAG_SPDIF_NONAUDIO );
	}

	//
	// Mixer functions
	//
	virtual ECHOSTATUS SetNominalLevel( WORD wBus, BOOL bState );
	virtual ECHOSTATUS GetNominalLevel( WORD wBus, PBYTE pbyState );

	ECHOSTATUS SetAudioMonitor
	(
		WORD	wOutCh,
		WORD	wInCh,
		INT32	iGain,
		BOOL 	fImmediate = TRUE
	);

	//
	// SetBusOutGain - empty function on non-vmixer cards
	//
	virtual ECHOSTATUS SetBusOutGain(WORD wBusOut,INT32 iGain)
	{
		return ECHOSTATUS_OK;
	}
	
	// Send volume to DSP
	ECHOSTATUS UpdateAudioOutLineLevel();

	// Send vmixer volume to DSP
	virtual ECHOSTATUS UpdateVmixerLevel();

	virtual ECHOSTATUS SetPipeOutGain
	( 
		WORD 	wPipeOut, 
		WORD 	wBusOut,
		INT32	iGain,
		BOOL 	fImmediate = TRUE
	);
	
	virtual ECHOSTATUS GetPipeOutGain
	( 
		WORD 	wPipeOut, 
		WORD 	wBusOut,
		INT32	&iGain
	);
	
	virtual ECHOSTATUS SetBusInGain
	( 
		WORD 	wBusIn, 
		INT32 iGain
	);

	virtual ECHOSTATUS GetBusInGain( WORD wBusIn, INT32 &iGain);
	
	//
	//	See description of ECHOGALS_METERS above for
	//	data format information.
	//
	virtual ECHOSTATUS GetAudioMeters
	(
		PECHOGALS_METERS	pMeters
	);

	ECHOSTATUS GetMetersOn
	(
		BOOL & bOn
	)
	{	bOn = ( 0 != m_wMeterOnCount ); return ECHOSTATUS_OK; }

	ECHOSTATUS SetMetersOn( BOOL bOn );

	//
	//	Set/get Audio Format
	//
	ECHOSTATUS SetAudioFormat
	(
		WORD 							wPipeIndex,
		PECHOGALS_AUDIOFORMAT	pFormat
	);

	ECHOSTATUS GetAudioFormat
	( 
		WORD 							wPipeIndex,
		PECHOGALS_AUDIOFORMAT	pFormat
	);

#ifdef MIDI_SUPPORT

	//
	//	MIDI output activity
	//
	virtual BOOL IsMidiOutActive();
		
	//
	// Set MIDI I/O on or off
	//
	ECHOSTATUS SetMidiOn( BOOL bOn );
	
	//
	// Read and write MIDI data
	//	
	ECHOSTATUS WriteMidi
	(
		PBYTE		pData,
		DWORD		dwLength,
		PDWORD	pdwActualCt
	);
	
	ECHOSTATUS ReadMidi
	(
		WORD 		wIndex,				// Buffer index
		DWORD &	dwData				// Return data
	);
	
#endif // MIDI_SUPPORT

	//
	//	Reset the DSP and load new firmware.
	//
	virtual ECHOSTATUS LoadFirmware();
	
	//
	// Put the hardware to sleep
	//
	virtual ECHOSTATUS GoComatose();
	

#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT
	//
	// Get and set the digital input auto-mute flag
	//
	virtual ECHOSTATUS GetDigitalInputAutoMute(BOOL &fAutoMute);
	virtual ECHOSTATUS SetDigitalInputAutoMute(BOOL fAutoMute);
	
#endif // DIGITAL_INPUT_AUTO_MUTE_SUPPORT
	
#ifdef SUPERMIX
	//
	// Set the input gain boost
	//
	virtual ECHOSTATUS SetInputGainBoost(WORD wBusIn,BYTE bBoostDb)
	{ return ECHOSTATUS_NOT_SUPPORTED; }
#endif	
	
};		// class CDspCommObject

typedef CDspCommObject * PCDspCommObject;

#ifdef _WIN32
#pragma pack( pop )
#endif

#endif

// **** DspCommObject.h ****

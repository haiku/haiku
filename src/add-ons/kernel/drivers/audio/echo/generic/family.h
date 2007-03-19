//********************************************************************************
//
// family.h
//
// This header file deals with variations between the different families of
// products.  Current families:
//
//  Echogals - Darla20, Gina20, Layla20, and Darla24
//  Echo24 - 	Gina24, Layla24, Mona, Mia, and Mia MIDI
//  Indigo - 	Indigo, Indigo io, and Indigo dj
//  3G -			Gina3G, Layla3G
//
//----------------------------------------------------------------------------
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
//********************************************************************************

#ifndef _FAMILY_H_
#define _FAMILY_H_

//===========================================================================
// 
// Echogals
//
// To build an Echogals driver, make sure and #define ECHOGALS_FAMILY
//
//===========================================================================

#ifdef ECHOGALS_FAMILY

#define MIDI_SUPPORT

#define READ_DSP_TIMEOUT		1000000L		// one second
#define MIN_MTC_1X_RATE			8000

#endif // ECHOGALS_FAMILY


//===========================================================================
// 
// Echo24
//
// To build an Echo24 driver, make sure and #define ECHO24_FAMILY
//
//===========================================================================

#ifdef ECHO24_FAMILY

#define MIDI_SUPPORT

#define DSP_56361									// Some Echo24 cards use the 56361 DSP

#define READ_DSP_TIMEOUT						100000L	// .1 second

#define STEREO_BIG_ENDIAN32_SUPPORT

#define LAYLA24_CARD_NAME			"Layla24"
#define LAYLA24_DSP_CODE			pwLayla24DSP
#define LAYLA24_HAS_VMIXER			FALSE
#define LAYLA24_2ASIC_FILENAME	"Layla24_2S_ASIC.c"

#define LAYLA24_DSP_FILENAME		"Layla24DSP.c"

#define MIN_MTC_1X_RATE							8000

#endif // ECHO24_FAMILY




//===========================================================================
// 
// Indigo, Indigo IO, and Indigo DJ
//
// To build an Indigo driver, make sure and #define INDIGO_FAMILY
//
//===========================================================================

#ifdef INDIGO_FAMILY

#define DSP_56361								// Indigo only uses the 56361

#define READ_DSP_TIMEOUT		100000L	// .1 second

#define STEREO_BIG_ENDIAN32_SUPPORT

#endif // INDIGO_FAMILY


//===========================================================================
// 
// 3G
//
//===========================================================================

#ifdef ECHO3G_FAMILY

#define MIDI_SUPPORT

#define DSP_56361									// Some Echo24 cards use the 56361 DSP

#define READ_DSP_TIMEOUT						100000L	// .1 second

#define STEREO_BIG_ENDIAN32_SUPPORT
#define PHANTOM_POWER_CONTROL	

#define MIN_MTC_1X_RATE							32000

#endif // ECHO3G_FAMILY


#endif // _FAMILY_H_

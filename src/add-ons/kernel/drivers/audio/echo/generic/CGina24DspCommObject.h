// ****************************************************************************
//
//		CGina24DspCommObject.H
//
//		Include file for EchoGals generic driver Gina24 DSP interface class.
//
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
//
//   Copyright Echo Digital Audio Corporation (c) 1998 - 2004
//   All rights reserved
//   www.echoaudio.com
//   
//   This file is part of Echo Digital Audio's generic driver library.
//   
//   Echo Digital Audio's generic driver library is free software; 
//   you can redistribute it and/or modify it under the terms of 
//   the GNU General Public License as published by the Free Software Foundation.
//   
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//   
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
//   MA  02111-1307, USA.
//
// ****************************************************************************

#ifndef	_GINA24DSPCOMMOBJECT_
#define	_GINA24DSPCOMMOBJECT_

#include "CGMLDspCommObject.h"

class CGina24DspCommObject : public CGMLDspCommObject
{
public:
	//
	//	Construction/destruction
	//
	CGina24DspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CGina24DspCommObject();

	//
	//	Set the DSP sample rate.
	//	Return rate that was set, -1 if error
	//
	virtual DWORD SetSampleRate( DWORD dwNewSampleRate );
	//
	//	Send current setting to DSP & return what it is
	//
	virtual DWORD SetSampleRate()
		{ return( SetSampleRate( GetSampleRate() ) ); }
	//
	//	Set digital mode
	//
	virtual ECHOSTATUS SetDigitalMode
	(
		BYTE	byNewMode
	);
	//
	//	Get mask of all supported digital modes
	//	(See ECHOCAPS_HAS_DIGITAL_MODE_??? defines in EchoGalsXface.h)
	//
	virtual DWORD GetDigitalModes()
		{ return( ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_RCA	  |
					 ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_OPTICAL |
					 ECHOCAPS_HAS_DIGITAL_MODE_ADAT			  |
					 ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_CDROM	); }

	//
	//	Set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);

	//
	//	Card information
	//
	virtual WORD GetCardType()
		{ return( GINA24 ); }

protected:

	virtual BOOL LoadASIC();

	BYTE *	m_pbyAsic;					// Current ASIC code

};		// class CGina24DspCommObject

typedef CGina24DspCommObject * PCGina24DspCommObject;

#endif

// **** Gina24DspCommObject.h ****

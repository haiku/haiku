// ****************************************************************************
//
//		CMonaDspCommObject.H
//
//		Include file for EchoGals generic driver Mona DSP interface class.
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

#ifndef	_MONADSPCOMMOBJECT_
#define	_MONADSPCOMMOBJECT_

#include "CGMLDspCommObject.h"

class CMonaDspCommObject : public CGMLDspCommObject
{
public:
	//
	//	Construction/destruction
	//
	CMonaDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CMonaDspCommObject();

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
					 ECHOCAPS_HAS_DIGITAL_MODE_ADAT ); }

	//
	//	Card information
	//
	virtual WORD GetCardType()
		{ return( MONA ); }

protected:

	virtual BOOL LoadASIC();
	BOOL SwitchAsic( DWORD dwMask96 );

	//
	//	Set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);

	BYTE *	m_pbyAsic;					// Current ASIC code

};		// class CMonaDspCommObject

typedef CMonaDspCommObject * PCMonaDspCommObject;

#endif

// **** MonaDspCommObject.h ****

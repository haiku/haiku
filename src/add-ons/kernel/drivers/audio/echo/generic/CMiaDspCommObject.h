// ****************************************************************************
//
//		CMiaDspCommObject.H
//
//		Include file for EchoGals generic driver Mia DSP interface class.
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

#ifndef	_MIADSPCOMMOBJECT_
#define	_MIADSPCOMMOBJECT_

#include "CDspCommObjectVmixer.h"

class CMiaDspCommObject : public CDspCommObjectVmixer
{
public:
	//
	//	Construction/destruction
	//
	CMiaDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CMiaDspCommObject();

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
	//	Card information
	//
	virtual WORD GetCardType()
		{ return( MIA ); }

	//
	//	Set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);

protected:

	BOOL		m_bProfessionalSpdif;	// S/PDIF pro/consumer setting

};		// class CMiaDspCommObject

typedef CMiaDspCommObject * PCMiaDspCommObject;

#endif

// **** CMiaDspCommObject.h ****

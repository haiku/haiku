// ****************************************************************************
//
//		CGina24DspCommObject.H
//
//		Include file for EchoGals generic driver Gina24 DSP interface class.
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

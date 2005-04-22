// ****************************************************************************
//
//		CLaylaDspCommObject.H
//
//		Include file for EchoGals generic driver Layla DSP interface class.
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

#ifndef	_LAYLADSPCOMMOBJECT_
#define	_LAYLADSPCOMMOBJECT_

#include "CDspCommObject.h"

//*****************************************************************************
//
// Layla clock numbers to send to DSP
//
//*****************************************************************************

#define LAYLA20_CLOCK_INTERNAL			0
#define LAYLA20_CLOCK_SPDIF				1
#define LAYLA20_CLOCK_WORD					2
#define LAYLA20_CLOCK_SUPER				3


//*****************************************************************************
//
// CLaylaDspCommObject
//
//*****************************************************************************


class CLaylaDspCommObject : public CDspCommObject
{
protected:

	enum
	{
		LAYLA20_INPUT_TRIMS	= 8
	};

	BYTE	m_byInputTrims[LAYLA20_INPUT_TRIMS];	// Input trims for Layla20's 8 analog
																// inputs

public:
	//
	//	Construction/destruction
	//
	CLaylaDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CLaylaDspCommObject();

	//
	//	Set the DSP sample rate
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
		{ return( LAYLA ); }

	//
	//	Set clocks
	//
	virtual ECHOSTATUS SetOutputClock(WORD wClock);
	virtual ECHOSTATUS SetInputClock(WORD wClock);

	//
	//	Input gain
	// 
	virtual ECHOSTATUS SetBusInGain(WORD wBusIn,INT32 iGain);
	virtual ECHOSTATUS GetBusInGain(WORD wBusIn,INT32 &iGain);
	virtual ECHOSTATUS SetNominalLevel( WORD wBus, BOOL bState );	

protected:

	virtual BOOL LoadASIC();

	//
	//	Check status of ASIC - loaded or not loaded
	//
	enum
	{
		NUM_ASIC_ATTEMPTS	= 5,
		NUM_ASIC_WINS = 3
	};

	virtual BOOL CheckAsicStatus();

};		// class CLaylaDspCommObject

typedef CLaylaDspCommObject * PCLaylaDspCommObject;

#endif

// **** LaylaDspCommObject.h ****

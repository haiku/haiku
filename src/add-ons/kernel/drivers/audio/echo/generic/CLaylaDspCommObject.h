// ****************************************************************************
//
//		CLaylaDspCommObject.H
//
//		Include file for EchoGals generic driver Layla DSP interface class.
//
// ----------------------------------------------------------------------------
//
//		Copyright Echo Digital Audio Corporation (c) 1998 - 2002
//		All rights reserved
//		www.echoaudio.com
//		
//		Permission is hereby granted, free of charge, to any person obtaining a
//		copy of this software and associated documentation files (the
//		"Software"), to deal with the Software without restriction, including
//		without limitation the rights to use, copy, modify, merge, publish,
//		distribute, sublicense, and/or sell copies of the Software, and to
//		permit persons to whom the Software is furnished to do so, subject to
//		the following conditions:
//		
//		- Redistributions of source code must retain the above copyright
//		notice, this list of conditions and the following disclaimers.
//		
//		- Redistributions in binary form must reproduce the above copyright
//		notice, this list of conditions and the following disclaimers in the
//		documentation and/or other materials provided with the distribution.
//		
//		- Neither the name of Echo Digital Audio, nor the names of its
//		contributors may be used to endorse or promote products derived from
//		this Software without specific prior written permission.
//
//		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//		IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
//		ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//		TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//		SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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

	BYTE			m_byInputTrims[8];	// Input trims for Layla20's 8 analog
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
	virtual ECHOSTATUS SetBusInGain(WORD wBusIn,int iGain);
	virtual ECHOSTATUS GetBusInGain(WORD wBusIn,int &iGain);
	virtual ECHOSTATUS SetNominalLevel( WORD wBus, BOOL bState );	

	virtual BOOL IsMidiOutActive();

protected:

	virtual BOOL LoadASIC();

};		// class CLaylaDspCommObject

typedef CLaylaDspCommObject * PCLaylaDspCommObject;

#endif

// **** LaylaDspCommObject.h ****

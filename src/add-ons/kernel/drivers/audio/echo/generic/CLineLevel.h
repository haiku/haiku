// ****************************************************************************
//
//		CLineLevel.h
//
//		Include file for EchoGals generic driver line level state machine.
//
//		Class for setting and getting mixer values for input and output busses.
//
//		Implemented as a base class with 3 derived classes, one for
//		each type of line.
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

#ifndef _CLineLevel_
#define _CLineLevel_


class CEchoGals;


/****************************************************************************

	CLineLevel - Base class

 ****************************************************************************/

class CLineLevel
{
protected :

	INT32				m_iGain;				// Current gain in dB X 256
	BOOL				m_fMuted;
	CEchoGals *		m_pEchoGals;		// Ptr to our creator object
	WORD				m_wChannelIndex;	// pipe index for this line

public:

	//
	//	Construction/destruction
	//
	CLineLevel();
	virtual ~CLineLevel();

	//
	//	Initialization function for initializing arrays of this class
	//
	void Init
	(
		WORD				wChannelIndex,	// Which channel we represent
		CEchoGals *		pEchoGals,		// For setting line levels
		INT32				iGain = 0		// Initial gain setting
	);

	//
	//	Mute 
	//
	virtual ECHOSTATUS SetMute( BOOL bOn );
	BOOL IsMuteOn() { return m_fMuted; }


	//
	//	Gain functions
	//
	INT32 GetGain() { return( m_iGain ); }
	virtual ECHOSTATUS SetGain( 
		INT32 	iGain, 
		BOOL 	fImmediate = TRUE 
	) = NULL;

};		// class CLineLevel

typedef CLineLevel * PCLineLevel;



/****************************************************************************

	CBusInLineLevel - Derived class for managing input bus gains

 ****************************************************************************/

class CBusInLineLevel : public CLineLevel
{
protected :

public:

	//
	//	Construction/destruction
	//
	CBusInLineLevel();
	virtual ~CBusInLineLevel();

	//
	//	Mute 
	//
	virtual ECHOSTATUS SetMute( BOOL bOn );

	//
	//	Gain functions
	//
	virtual ECHOSTATUS SetGain
	( 
		INT32 	iGain, 
		BOOL 	fImmediate = TRUE 
	);


};		// class CBusInLineLevel

typedef CBusInLineLevel * PCBusInLineLevel;




/****************************************************************************

	CBusOutLineLevel - Derived class for managing output bus gains

 ****************************************************************************/

class CBusOutLineLevel : public CLineLevel
{
protected :

public:

	//
	//	Construction/destruction
	//
	CBusOutLineLevel();
	virtual ~CBusOutLineLevel();

	//
	//	Mute 
	//
	virtual ECHOSTATUS SetMute( BOOL bOn );

	//
	//	Gain functions
	//
	virtual ECHOSTATUS SetGain
	( 
		INT32 	iGain, 
		BOOL 	fImmediate = TRUE 
	);


};		// class CBusOutLineLevel

typedef CBusOutLineLevel * PCBusOutLineLevel;

#endif

// **** CLineLevel.h ****

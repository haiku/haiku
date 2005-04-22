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

// ****************************************************************************
//
//  	CIndigoDJDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Indigo DJ DSP
//		interface class.
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

#include "CEchoGals.h"
#include "CIndigoDJDspCommObject.h"

#ifdef ECHO_WDM
#pragma optimize("",off)
#endif
#include "IndigoDJDSP.c"


//
//	Construction/destruction
//
CIndigoDJDspCommObject::CIndigoDJDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CIndigoDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Indigo dj" );

	m_wNumBussesOut = 4;
	m_wFirstDigitalBusOut = m_wNumBussesOut;

	m_pwDspCodeToLoad = pwIndigodjDSP;
	
}	// CIndigoDJDspCommObject::CIndigoDJDspCommObject( DWORD dwPhysRegBase )


CIndigoDJDspCommObject::~CIndigoDJDspCommObject()
{
}	// CIndigoDJDspCommObject::~CIndigoDJDspCommObject()


// **** CIndigoDJDspCommObject.cpp ****

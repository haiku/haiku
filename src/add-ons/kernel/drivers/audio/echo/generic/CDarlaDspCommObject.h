// ****************************************************************************
//
//		CDarlaDspCommObject.H
//
//		Include file for Darla20 DSP interface class.
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

#ifndef	_DARLADSPCOMMOBJECT_
#define	_DARLADSPCOMMOBJECT_

#include "CGdDspCommObject.h"


class CDarlaDspCommObject : public CGdDspCommObject
{
public:
	//
	//	Construction/destruction
	//
	CDarlaDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CDarlaDspCommObject();
	//
	//	Card information
	//
	virtual WORD GetCardType()
		{ return DARLA; }

	//
	//	Set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock)
	{
		return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	}
protected:

};		// class CDarlaDspCommObject

typedef CDarlaDspCommObject * PCDarlaDspCommObject;

#endif

// **** DarlaDspCommObject.h ****

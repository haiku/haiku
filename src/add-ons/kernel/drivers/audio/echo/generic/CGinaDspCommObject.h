// ****************************************************************************
//
//		CGinaDspCommObject.H
//
//		Include file for EchoGals generic driver Gina DSP interface class.
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

#ifndef	_GINADSPCOMMOBJECT_
#define	_GINADSPCOMMOBJECT_

#include "CGdDspCommObject.h"


class CGinaDspCommObject : public CGdDspCommObject
{
public:
	//
	//	Construction/destruction
	//
	CGinaDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~CGinaDspCommObject();

	//
	//	Card information
	//
	virtual WORD GetCardType()
		{ return( GINA ); }

	//
	//	Set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);

protected:

};		// class CGinaDspCommObject

typedef CGinaDspCommObject * PCGinaDspCommObject;

#endif

// **** GinaDspCommObject.h ****

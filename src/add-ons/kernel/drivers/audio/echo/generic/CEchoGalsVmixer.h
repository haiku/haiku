// ****************************************************************************
//
//		CEchoGalsVmixer.cpp
//
//		CEchoGalsMTC is used to add virtual output mixing to the base
//		CEchoGals class.
//
//		Set editor tabs to 3 for your viewing pleasure.
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

//	Prevent problems with multiple includes
#ifndef _CECHOGALSVMIXER_H_
#define _CECHOGALSVMIXER_H_

#include "CEchoGals.h"

class CEchoGalsVmixer : public CEchoGals
{
public:
	//
	//	Construction/destruction
	//
	CEchoGalsVmixer( PCOsSupport pOsSupport );
	virtual ~CEchoGalsVmixer();

	//
	// Adjust all the output pipe levels for a particular output bus
	//
	virtual ECHOSTATUS AdjustPipesOutForBusOut(WORD wBusOut,INT32 iBusOutGain);

protected:	
	virtual ECHOSTATUS GetBaseCapabilities(PECHOGALS_CAPS pCapabilities);

	// 
	// Adjust all the monitor levels for a particular output bus; this is
	// used to implement the master output level.
	//
	virtual ECHOSTATUS AdjustMonitorsForBusOut(WORD wBusOut);

};		// class CEchoGalsVmixer


#endif // _CECHOGALSVMIXER_H_

// *** CEchoGalsVmixer.h ***

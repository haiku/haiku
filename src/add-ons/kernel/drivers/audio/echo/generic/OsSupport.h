// ****************************************************************************
//
//		OsSupport.H
//
//		Wrapper include file for OS-specific header files
//		Set editor tabs to 3 for your viewing pleasure.
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

//	Prevent problems with multiple includes
#ifndef _ECHOOSSUPPORT_
#define _ECHOOSSUPPORT_

//===========================================================================
//
// BeOS
//
//===========================================================================

#ifdef ECHO_BEOS

#include "OsSupportBeOS.h"

#endif


//===========================================================================
//
// WDM driver
//
//===========================================================================

#ifdef ECHO_WDM

//
// WDM driver for Windows Me/2000/XP
//
#include "OsSupportWDM.h"

#endif


//===========================================================================
//
// MacOS 8 and 9
//
//===========================================================================

#ifdef ECHO_OS9

#include "OsSupportMac.h"

#endif


//===========================================================================
//
// Mac OS X
//
//===========================================================================

#ifdef ECHO_OSX

#include "OsSupportOsX.h"

#endif


#endif	// _ECHOOSSUPPORT_

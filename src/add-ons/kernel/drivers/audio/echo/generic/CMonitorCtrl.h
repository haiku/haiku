// ****************************************************************************
//
//		CMonitorCtrl.h
//
//		Class to control input monitors on cards with or without vmixers.
//		Input monitors are used to route audio data from input busses to
//		output busses through the DSP with very low latency.
//
//		Any input bus may be routed to any output bus.
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

#ifndef _CMonitorCtrl_H_
#define _CMonitorCtrl_H_

class CEchoGals;

class CMonitorCtrl
{

protected:

	typedef struct 
	{
		INT8	iLeft;
		INT8	iRight;
	} PAN_DB;

	CEchoGals	*m_pEG;
	
	WORD			m_wNumBussesIn;
	WORD			m_wNumBussesOut;
	
	INT8			*m_Gains;
	BYTE			*m_Mutes;
	WORD			*m_Pans;
	PAN_DB		*m_PanDbs;
	
	WORD GetIndex(WORD wBusIn,WORD wBusOut)
	{
		return (wBusOut >> 1) * m_wNumBussesIn + wBusIn;
	}
	
public:

	ECHOSTATUS Init(CEchoGals *m_pEG);
	void Cleanup();

	ECHOSTATUS SetGain
	(
		WORD 	wBusIn, 
		WORD 	wBusOut, 
		INT32 	iGain,
		BOOL 	fImmediate = TRUE
	);
	ECHOSTATUS GetGain(WORD wBusIn, WORD wBusOut, INT32 &iGain);
	
	ECHOSTATUS SetMute
	(	
		WORD 	wBusIn, 
		WORD 	wBusOut, 
		BOOL 	bMute,
		BOOL 	fImmediate = TRUE
	);
	ECHOSTATUS GetMute(WORD wBusIn, WORD wBusOut, BOOL &bMute);
	
	ECHOSTATUS SetPan(WORD wBusIn, WORD wBusOut, INT32 iPan);
	ECHOSTATUS GetPan(WORD wBusIn, WORD wBusOut, INT32 &iPan);

};

#endif // _CMonitorCtrl_H_

// ****************************************************************************
//
//		CPipeOutCtrl.h
//
//		Class to control output pipes on cards with or without vmixers.
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

#ifndef _CPIPEOUTCTRL_H_
#define _CPIPEOUTCTRL_H_

class CEchoGals;

class CPipeOutCtrl
{

protected:

	typedef struct 
	{
		INT8	iLeft;
		INT8	iRight;
	} PAN_DB;

	CEchoGals	*m_pEG;
		
	WORD			m_wNumPipesOut;
	WORD			m_wNumBussesOut;
	BOOL			m_fHasVmixer;
	
	INT8			*m_Gains;
	BYTE			*m_Mutes;
	WORD			*m_Pans;
	PAN_DB		*m_PanDbs;
	
	WORD GetIndex(WORD wPipe,WORD wBus)
	{
		if (!m_fHasVmixer)
			return wPipe;
			
		return (wBus >> 1) * m_wNumPipesOut + wPipe;
	}
	
public:

	ECHOSTATUS Init(CEchoGals *m_pEG);
	void Cleanup();

	ECHOSTATUS SetGain
	(
		WORD 	wPipeOut, 
		WORD 	wBusOut, 
		INT32 	iGain,
		BOOL 	fImmediate = TRUE
	);
	ECHOSTATUS GetGain(WORD wPipeOut, WORD wBusOut, INT32 &iGain);
	
	ECHOSTATUS SetMute
	(	
		WORD 	wPipeOut, 
		WORD 	wBusOut, 
		BOOL 	bMute,
		BOOL 	fImmediate = TRUE
	);
	ECHOSTATUS GetMute(WORD wPipeOut, WORD wBusOut, BOOL &bMute);
	
	ECHOSTATUS SetPan(WORD wPipeOut, WORD wBusOut, INT32 iPan);
	ECHOSTATUS GetPan(WORD wPipeOut, WORD wBusOut, INT32 &iPan);

};

#endif // _CPIPEOUTCTRL_H_

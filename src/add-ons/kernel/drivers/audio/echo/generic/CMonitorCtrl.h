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

	~CMonitorCtrl();

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

// ****************************************************************************
//
//		CPipeOutCtrl.h
//
//		Class to control output pipes on cards with or without vmixers.
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

	~CPipeOutCtrl();

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

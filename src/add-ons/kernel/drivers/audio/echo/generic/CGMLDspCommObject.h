// ****************************************************************************
//
//		CGMLDspCommObject.H
//
//		Common DSP interface class for Gina24, Layla24, and Mona
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

#ifndef	_GMLDSPCOMMOBJECT_
#define	_GMLDSPCOMMOBJECT_

class CGMLDspCommObject : public CDspCommObject
{
public:
	CGMLDspCommObject( PDWORD pdwRegBase, PCOsSupport pOsSupport )
		: CDspCommObject(pdwRegBase, pOsSupport)
	{
	}
		
	virtual ~CGMLDspCommObject() {}

	//
	// Set get S/PDIF output format
	//
	virtual void SetProfessionalSpdif( BOOL bNewStatus );

	virtual BOOL IsProfessionalSpdif()
		{ return( m_bProfessionalSpdif ); }
		
	//
	// Get/Set S/PDIF out non-audio status bit
	//
	virtual BOOL IsSpdifOutNonAudio()
	{
			return m_bNonAudio;
	}
	
	virtual void SetSpdifOutNonAudio(BOOL bNonAudio);
		
protected:
	//
	// Write the Gina24/Mona/Layla24 control reg
	//
	virtual ECHOSTATUS WriteControlReg( DWORD dwControlReg, BOOL fForceWrite = FALSE );
	
	//
	// Member variables
	//
	BOOL m_bProfessionalSpdif;
	BOOL m_bNonAudio;

};		// class CGMLDspCommObject

#endif // _GMLDSPCOMMOBJECT_

// **** GMLDspCommObject.h ****

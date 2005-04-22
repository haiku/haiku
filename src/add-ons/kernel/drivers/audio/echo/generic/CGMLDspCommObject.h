// ****************************************************************************
//
//		CGMLDspCommObject.H
//
//		Common DSP interface class for Gina24, Layla24, and Mona
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
	//	Get mask of all supported digital modes
	//	(See ECHOCAPS_HAS_DIGITAL_MODE_??? defines in EchoGalsXface.h)
	//
	virtual DWORD GetDigitalModes()
		{ return( ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_RCA	  |
					 ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_OPTICAL |
					 ECHOCAPS_HAS_DIGITAL_MODE_ADAT ); }

	//
	//	Set digital mode
	//
	virtual ECHOSTATUS SetDigitalMode
	(
		BYTE	byNewMode
	);

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

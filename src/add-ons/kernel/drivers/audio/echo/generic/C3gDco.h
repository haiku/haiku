// ****************************************************************************
//
//		C3gDco.H
//
//		Include file for EchoGals generic driver 3g DSP interface class.
//
// ----------------------------------------------------------------------------
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

#ifndef	_3GDSPCOMMOBJECT_
#define	_3GDSPCOMMOBJECT_

#include "CDspCommObject.h"

class C3gDco : public CDspCommObject
{
public:
	//
	//	Construction/destruction
	//
	C3gDco( PDWORD pdwRegBase, PCOsSupport pOsSupport );
	virtual ~C3gDco();

	//
	//	Set the DSP sample rate.
	//	Return rate that was set, -1 if error
	//
	virtual DWORD SetSampleRate( DWORD dwNewSampleRate );
	//
	//	Send current setting to DSP & return what it is
	//
	virtual DWORD SetSampleRate()
		{ return( SetSampleRate( GetSampleRate() ) ); }

	//
	//	Card information
	//
	virtual WORD GetCardType()
		{ return( ECHO3G ); }
		
	virtual void Get3gBoxType(DWORD *pOriginalBoxType,DWORD *pCurrentBoxType);

	//
	//	Get mask of all supported digital modes
	//	(See ECHOCAPS_HAS_DIGITAL_MODE_??? defines in EchoGalsXface.h)
	//
	virtual DWORD GetDigitalModes();

	//
	//	Set input clock
	//
	virtual ECHOSTATUS SetInputClock(WORD wClock);
	
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
	
	void SetPhantomPower( BOOL fPhantom );
	
	virtual ECHOSTATUS GetAudioMeters
	(
		PECHOGALS_METERS	pMeters
	);
		
	BOOL DoubleSpeedMode(DWORD *pdwNewCtrlReg = NULL);

	CChannelMask m_Adat38Mask;

protected:

	//
	// ASIC loader
	//
	virtual BOOL LoadASIC();

	//
	//	Check status of external box
	//
	enum
	{
		E3G_ASIC_NOT_LOADED	= 0xffff,
		E3G_BOX_TYPE_MASK			= 0xf0
	};
	virtual BOOL CheckAsicStatus();
	void SetChannelCounts();

	//
	//	Returns 3G frequency register
	//
	DWORD Get3gFreqReg()
		{ ECHO_ASSERT(NULL != m_pDspCommPage );
		  return SWAP( m_pDspCommPage->dw3gFreqReg ); }

	//
	// Write the control reg
	//
	ECHOSTATUS WriteControlReg
	( 
		DWORD dwControlReg,
		DWORD	dwFreqReg,
		BOOL 	fForceWrite = FALSE
	);
	
	//
	// Use this to check if a new control reg setting may be 
	// applied
	//
	ECHOSTATUS ValidateCtrlReg(DWORD dwNewControlReg );

	//
	// Set the various S/PDIF status bits
	//
	void SetSpdifBits(DWORD *pdwCtrlReg,DWORD dwSampleRate);
	
	//
	// Member variables
	//
	BOOL m_bProfessionalSpdif;
	BOOL m_bNonAudio;
	DWORD m_dwOriginalBoxType;
	DWORD m_dwCurrentBoxType;
	BOOL	m_bBoxTypeSet;
	
};		// class C3gDco

typedef C3gDco* PC3gDco;

//
// 3G register bits
//
#define E3G_CONVERTER_ENABLE		0x0010
#define E3G_SPDIF_PRO_MODE			0x0020		// Professional S/PDIF == 1, consumer == 0
#define E3G_SPDIF_SAMPLE_RATE0	0x0040
#define E3G_SPDIF_SAMPLE_RATE1	0x0080
#define E3G_SPDIF_TWO_CHANNEL		0x0100		// 1 == two channels, 0 == one channel
#define E3G_SPDIF_NOT_AUDIO		0x0200
#define E3G_SPDIF_COPY_PERMIT		0x0400
#define E3G_SPDIF_24_BIT			0x0800		// 1 == 24 bit, 0 == 20 bit
#define E3G_DOUBLE_SPEED_MODE		0x4000		// 1 == double speed, 0 == single speed			
#define E3G_PHANTOM_POWER			0x8000		// 1 == phantom power on, 0 == phantom power off

#define E3G_96KHZ						(0x0 | E3G_DOUBLE_SPEED_MODE)
#define E3G_88KHZ						(0x1 | E3G_DOUBLE_SPEED_MODE)
#define E3G_48KHZ						0x2
#define E3G_44KHZ						0x3
#define E3G_32KHZ						0x4
#define E3G_22KHZ						0x5
#define E3G_16KHZ						0x6
#define E3G_11KHZ						0x7
#define E3G_8KHZ						0x8
#define E3G_SPDIF_CLOCK				0x9
#define E3G_ADAT_CLOCK				0xA
#define E3G_WORD_CLOCK				0xB
#define E3G_CONTINUOUS_CLOCK		0xE

#define E3G_ADAT_MODE				0x1000
#define E3G_SPDIF_OPTICAL_MODE	0x2000

#define E3G_CLOCK_CLEAR_MASK			0xbfffbff0
#define E3G_DIGITAL_MODE_CLEAR_MASK	0xffffcfff
#define E3G_SPDIF_FORMAT_CLEAR_MASK	0xfffff01f

//
//	Clock detect bits reported by the DSP
//
#define E3G_CLOCK_DETECT_BIT_WORD96		0x0001
#define E3G_CLOCK_DETECT_BIT_WORD48		0x0002
#define E3G_CLOCK_DETECT_BIT_SPDIF48	0x0004
#define E3G_CLOCK_DETECT_BIT_ADAT		0x0004
#define E3G_CLOCK_DETECT_BIT_SPDIF96	0x0008
#define E3G_CLOCK_DETECT_BIT_WORD		(E3G_CLOCK_DETECT_BIT_WORD96|E3G_CLOCK_DETECT_BIT_WORD48)
#define E3G_CLOCK_DETECT_BIT_SPDIF		(E3G_CLOCK_DETECT_BIT_SPDIF48|E3G_CLOCK_DETECT_BIT_SPDIF96)


//
// Frequency control register
//
#define E3G_MAGIC_NUMBER				   677376000
#define E3G_FREQ_REG_DEFAULT				(E3G_MAGIC_NUMBER / 48000 - 2)
#define E3G_FREQ_REG_MAX					0xffff


//
// Other stuff
//
#define E3G_MAX_OUTPUTS						16

#endif // _3GDSPCOMMOBJECT_

// **** C3gDco.h ****

/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This file is part of libebml.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: EbmlCrc32.h 1155 2005-05-06 11:43:38Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Jory Stone       <jcsston @ toughguy.net>
*/
#ifndef LIBEBML_CRC32_H
#define LIBEBML_CRC32_H

#include "EbmlTypes.h"
#include "EbmlBinary.h"

START_LIBEBML_NAMESPACE

const uint32 CRC32_NEGL = 0xffffffffL;

#ifdef WORDS_BIGENDIAN
# define CRC32_INDEX(c) (c >> 24)
# define CRC32_SHIFTED(c) (c << 8)
#else
# define CRC32_INDEX(c) (c & 0xff)
# define CRC32_SHIFTED(c) (c >> 8)
#endif

class EBML_DLL_API EbmlCrc32 : public EbmlBinary {
	public:
		EbmlCrc32();
		EbmlCrc32(const EbmlCrc32 & ElementToClone);
		static EbmlElement & Create() {return *(new EbmlCrc32);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		bool ValidateSize() const {return (Size == 4);}
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);
//		uint64 UpdateSize(bool bKeepIntact = false);
		
		static const EbmlCallbacks ClassInfos;
		bool IsDefaultValue() const {
			return false;
		}

		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		bool IsYourId(const EbmlId & TestId) const;
	
		void AddElementCRC32(EbmlElement &ElementToCRC);
		bool CheckElementCRC32(EbmlElement &ElementToCRC);
		
		/*!
			CRC Checksum Calculation
		*/
		enum {DIGESTSIZE = 4};
		
		/*!
			Use this to quickly check a CRC32 with some data
			\return True if inputCRC matches CRC32 generated from input data
		*/
		static bool CheckCRC(uint32 inputCRC, const binary *input, uint32 length);
		/*!
			Calls Update() and Finalize(), use to create a CRC32 in one go
		*/
		void FillCRC32(const binary *input, uint32 length);
		/*!
			Add data to the CRC table, in other words process some data bit by bit
		*/
		void Update(const binary *input, uint32 length);
		/*!
			Use this with Update() to Finalize() or Complete the CRC32
		*/
		void Finalize();
		/*!
			Returns a uint32 that has the value of the CRC32
		*/
		uint32 GetCrc32() const {
			return m_crc_final;
		};
	
		void ForceCrc32(uint32 NewValue) { m_crc_final = NewValue; bValueIsSet = true;}

		EbmlElement * Clone() const;

	protected:
		void ResetCRC() {m_crc = CRC32_NEGL;}
		void UpdateByte(binary b) {m_crc = m_tab[CRC32_INDEX(m_crc) ^ b] ^ CRC32_SHIFTED(m_crc);}

		static const uint32 m_tab[256];
		uint32 m_crc;
		uint32 m_crc_final;
};

template <class T>
inline unsigned int GetAlignment(T *dummy=NULL) // VC60 workaround
{
#if (_MSC_VER >= 1300)
	return __alignof(T);
#elif defined(__GNUC__)
	return __alignof__(T);
#else
	return sizeof(T);
#endif
}

template <class T>
inline bool IsPowerOf2(T n)
{
	return n > 0 && (n & (n-1)) == 0;
}

template <class T1, class T2>
inline T2 ModPowerOf2(T1 a, T2 b)
{
	assert(IsPowerOf2(b));
	return T2(a) & (b-1);
}

inline bool IsAlignedOn(const void *p, unsigned int alignment)
{
	return IsPowerOf2(alignment) ? ModPowerOf2((unsigned long)p, alignment) == 0 : (unsigned long)p % alignment == 0;
}

template <class T>
inline bool IsAligned(const void *p, T *dummy=NULL)	// VC60 workaround
{
	return IsAlignedOn(p, GetAlignment<T>());
}

END_LIBEBML_NAMESPACE

#endif // LIBEBML_CRC32_H

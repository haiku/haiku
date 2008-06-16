/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
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
    \version \$Id: EbmlEndian.h 1298 2008-02-21 22:14:18Z mosu $
    \author Ingo Ralf Blum   <ingoralfblum @ users.sf.net>
    \author Lasse Kärkkäinen <tronic @ users.sf.net>
    \author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_ENDIAN_H
#define LIBEBML_ENDIAN_H

#include <algorithm>
#include <cstring>

#include "EbmlConfig.h" // contains _ENDIANESS_

START_LIBEBML_NAMESPACE

enum endianess {
    big_endian,   ///< PowerPC, Alpha, 68000
    little_endian ///< Intel x86 platforms
};

/*!
    \class Endian
    \brief general class to handle endian-specific buffers
    \note don't forget to define/undefine _ENDIANESS_ to BIG_ENDIAN depending on your machine
*/
template<class TYPE, endianess ENDIAN> class Endian
{
    public:
	Endian() {}

	Endian(const TYPE value)
	{
		memcpy(&platform_value, &value, sizeof(TYPE));
		process_endian();
	}

	inline Endian & Eval(const binary *endian_buffer)
	{
	    //endian_value = *(TYPE *)(endian_buffer);
	    memcpy(&endian_value, endian_buffer, sizeof(TYPE));	// Some (all?) RISC processors do not allow reading objects bigger than 1 byte from non-aligned addresses, and endian_buffer may point to a non-aligned address.
	    process_platform();
	    return *this;
	}

	inline void Fill(binary *endian_buffer) const
	{
	    //*(TYPE*)endian_buffer = endian_value;
	    memcpy(endian_buffer, &endian_value, sizeof(TYPE)); // See above.
	}

	inline operator const TYPE&() const { return platform_value; }
//	inline TYPE endian() const   { return endian_value; }
	inline const TYPE &endian() const       { return endian_value; }
	inline size_t size() const   { return sizeof(TYPE); }
	inline bool operator!=(const binary *buffer) const {return *((TYPE*)buffer) == platform_value;}

    protected:
	TYPE platform_value;
	TYPE endian_value;

	inline void process_endian()
	{
	    endian_value = platform_value;
#ifdef WORDS_BIGENDIAN
	    if (ENDIAN == little_endian)
		std::reverse(reinterpret_cast<uint8*>(&endian_value),reinterpret_cast<uint8*>(&endian_value+1));
#else  // _ENDIANESS_
	    if (ENDIAN == big_endian)
		std::reverse(reinterpret_cast<uint8*>(&endian_value),reinterpret_cast<uint8*>(&endian_value+1));
#endif // _ENDIANESS_
	}

	inline void process_platform()
	{
	    platform_value = endian_value;
#ifdef WORDS_BIGENDIAN
	    if (ENDIAN == little_endian)
		std::reverse(reinterpret_cast<uint8*>(&platform_value),reinterpret_cast<uint8*>(&platform_value+1));
#else  // _ENDIANESS_
	    if (ENDIAN == big_endian)
		std::reverse(reinterpret_cast<uint8*>(&platform_value),reinterpret_cast<uint8*>(&platform_value+1));
#endif // _ENDIANESS_
	}
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_ENDIAN_H

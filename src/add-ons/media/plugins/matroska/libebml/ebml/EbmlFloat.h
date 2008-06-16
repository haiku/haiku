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
	\version \$Id: EbmlFloat.h 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_FLOAT_H
#define LIBEBML_FLOAT_H

#include "EbmlTypes.h"
#include "EbmlElement.h"

START_LIBEBML_NAMESPACE

/*!
    \class EbmlFloat
    \brief Handle all operations on a float EBML element
*/
class EBML_DLL_API EbmlFloat : public EbmlElement {
	public:
		enum Precision {
			 FLOAT_32
			,FLOAT_64
		};

		EbmlFloat(const Precision prec = FLOAT_32);
		EbmlFloat(const double DefaultValue, const Precision prec = FLOAT_32);
		EbmlFloat(const EbmlFloat & ElementToClone);

		bool ValidateSize() const 
		{
			return (Size == 4 || Size == 8);
		}
	
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);
		uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false);

		void SetPrecision(const EbmlFloat::Precision prec = FLOAT_32) 
		{
			if (prec == FLOAT_64)
				Size = 8;
			else
				Size = 4; // default size
		}

	
//		EbmlFloat & operator=(const float NewValue) { Value = NewValue; return *this;}
		EbmlFloat & operator=(const double NewValue) { Value = NewValue; bValueIsSet = true; return *this;}

		bool operator<(const EbmlFloat & EltCmp) const {return Value < EltCmp.Value;}
		
		operator const float() const {return float(Value);}
		operator const double() const {return double(Value);}

		void SetDefaultValue(double aValue) {assert(!DefaultIsSet); DefaultValue = aValue; DefaultIsSet = true;}
    
		const double DefaultVal() const {assert(DefaultIsSet); return DefaultValue;}

		bool IsDefaultValue() const {
			return (DefaultISset() && Value == DefaultValue);
		}
		
	protected:
		double Value; /// The actual value of the element
		double DefaultValue;
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_FLOAT_H

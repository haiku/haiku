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
	\version \$Id: EbmlFloat.cpp 710 2004-08-10 12:27:34Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#include <cassert>

#include "ebml/EbmlFloat.h"

START_LIBEBML_NAMESPACE

EbmlFloat::EbmlFloat(const EbmlFloat::Precision prec)
 :EbmlElement(0, false)
{
	SetPrecision(prec);
}

EbmlFloat::EbmlFloat(const double aDefaultValue, const EbmlFloat::Precision prec)
 :EbmlElement(0, true), Value(aDefaultValue), DefaultValue(aDefaultValue)
{
	DefaultIsSet = true;
	SetPrecision(prec);
}

EbmlFloat::EbmlFloat(const EbmlFloat & ElementToClone)
 :EbmlElement(ElementToClone)
 ,Value(ElementToClone.Value)
 ,DefaultValue(ElementToClone.DefaultValue)
{
}

/*!
	\todo handle exception on errors
	\todo handle 10 bits precision
*/
uint32 EbmlFloat::RenderData(IOCallback & output, bool bForceRender, bool bSaveDefault)
{
	assert(Size == 4 || Size == 8);

	if (Size == 4) {
		float val = Value;
		big_int32 TmpToWrite(*((int32 *) &val));
		output.writeFully(&TmpToWrite.endian(), Size);
	} else if (Size == 8) {
		double val = Value;
		big_int64 TmpToWrite(*((int64 *) &val));
		output.writeFully(&TmpToWrite.endian(), Size);
	} 

	return Size;
}

uint64 EbmlFloat::UpdateSize(bool bSaveDefault, bool bForceRender)
{
	if (!bSaveDefault && IsDefaultValue())
		return 0;
	return Size;
}

/*!
	\todo remove the hack for possible endianess pb (test on little & big endian)
*/
uint64 EbmlFloat::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	if (ReadFully != SCOPE_NO_DATA)
	{
		binary Buffer[20];
		assert(Size <= 20);
		input.readFully(Buffer, Size);
		
		if (Size == 4) {
			big_int32 TmpRead;
			TmpRead.Eval(Buffer);
			float val = *((float *)&(int32(TmpRead)));
			Value = val;
			bValueIsSet = true;
		} else if (Size == 8) {
			big_int64 TmpRead;
			TmpRead.Eval(Buffer);
			int64 tmpp = int64(TmpRead);
			Value = *((double *) &tmpp);
			bValueIsSet = true;
		}
	}

	return Size;
}

END_LIBEBML_NAMESPACE

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
	\version \$Id: EbmlDummy.h 639 2004-07-09 20:59:14Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_DUMMY_H
#define LIBEBML_DUMMY_H

#include "EbmlBinary.h"

START_LIBEBML_NAMESPACE

class EBML_DLL_API EbmlDummy : public EbmlBinary {
	public:
		EbmlDummy() :DummyId(DummyRawId)  {}
		EbmlDummy(const EbmlId & aId) :EbmlBinary(), DummyId(aId) {}
		EbmlDummy(const EbmlDummy & ElementToClone):EbmlBinary(ElementToClone), DummyId(ElementToClone.DummyId) {}
		static EbmlElement & Create() {return *(new EbmlDummy);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return DummyId;}

		bool ValidateSize() const {return true;}
		bool IsDummy() const {return true;}
		bool IsDefaultValue() const {return true;}

		EbmlElement * Clone() const {return new EbmlDummy(*this);}

	protected:
		const EbmlId DummyId;
		static const EbmlId DummyRawId;
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_DUMMY_H

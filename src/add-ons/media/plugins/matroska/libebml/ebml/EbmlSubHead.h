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
	\version \$Id: EbmlSubHead.h 639 2004-07-09 20:59:14Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_SUBHEAD_H
#define LIBEBML_SUBHEAD_H

#include <string>

#include "EbmlUInteger.h"
#include "EbmlString.h"

START_LIBEBML_NAMESPACE

class EBML_DLL_API EVersion : public EbmlUInteger {
	public:
		EVersion() :EbmlUInteger(1) {}
		EVersion(const EVersion & ElementToClone) : EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new EVersion);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EVersion(*this);}
};

class EBML_DLL_API EReadVersion : public EbmlUInteger {
	public:
		EReadVersion() :EbmlUInteger(1) {}
		EReadVersion(const EReadVersion & ElementToClone) : EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new EReadVersion);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EReadVersion(*this);}
};

class EBML_DLL_API EMaxIdLength : public EbmlUInteger {
	public:
		EMaxIdLength() :EbmlUInteger(4) {}
		EMaxIdLength(const EMaxIdLength & ElementToClone) : EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new EMaxIdLength);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EMaxIdLength(*this);}
};

class EBML_DLL_API EMaxSizeLength : public EbmlUInteger {
	public:
		EMaxSizeLength() :EbmlUInteger(8) {}
		EMaxSizeLength(const EMaxSizeLength & ElementToClone) : EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new EMaxSizeLength);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EMaxSizeLength(*this);}
};

class EBML_DLL_API EDocType : public EbmlString {
	public:
		EDocType() {}
		EDocType(const EDocType & ElementToClone) : EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new EDocType);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EDocType(*this);}
};

class EBML_DLL_API EDocTypeVersion : public EbmlUInteger {
	public:
		EDocTypeVersion() {}
		EDocTypeVersion(const EDocTypeVersion & ElementToClone) : EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new EDocTypeVersion);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EDocTypeVersion(*this);}
};

class EBML_DLL_API EDocTypeReadVersion : public EbmlUInteger {
	public:
		EDocTypeReadVersion() {}
		EDocTypeReadVersion(const EDocTypeReadVersion & ElementToClone) : EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new EDocTypeReadVersion);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new EDocTypeReadVersion(*this);}
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_SUBHEAD_H

/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class MATROSKA_DLL_API description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
**
** This file is part of libmatroska.
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxInfo.h,v 1.7 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_INFO_H
#define LIBMATROSKA_INFO_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlUnicodeString.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxInfo : public EbmlMaster {
	public:
		KaxInfo();
		KaxInfo(const KaxInfo & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxInfo);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxInfo(*this);}
};

class MATROSKA_DLL_API KaxMuxingApp : public EbmlUnicodeString {
	public:
		KaxMuxingApp() {}
		KaxMuxingApp(const KaxMuxingApp & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxMuxingApp);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxMuxingApp(*this);}
};

class MATROSKA_DLL_API KaxWritingApp : public EbmlUnicodeString {
	public:
		KaxWritingApp() {}
		KaxWritingApp(const KaxWritingApp & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxWritingApp);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxWritingApp(*this);}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_INFO_H

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
	\version \$Id: KaxClusterData.h,v 1.9 2004/04/21 19:50:10 mosu Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_CLUSTER_DATA_H
#define LIBMATROSKA_CLUSTER_DATA_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlUInteger.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxClusterTimecode : public EbmlUInteger {
	public:
		KaxClusterTimecode() {}
		KaxClusterTimecode(const KaxClusterTimecode & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxClusterTimecode);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxClusterTimecode(*this);}
};

class MATROSKA_DLL_API KaxClusterSilentTracks : public EbmlMaster {
	public:
		KaxClusterSilentTracks();
		KaxClusterSilentTracks(const KaxClusterSilentTracks & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxClusterSilentTracks);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxClusterSilentTracks(*this);}
};

class MATROSKA_DLL_API KaxClusterSilentTrackNumber : public EbmlUInteger {
	public:
		KaxClusterSilentTrackNumber() {}
		KaxClusterSilentTrackNumber(const KaxClusterSilentTrackNumber & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxClusterSilentTrackNumber);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxClusterSilentTrackNumber(*this);}
};

class MATROSKA_DLL_API KaxClusterPosition : public EbmlUInteger {
	public:
		KaxClusterPosition() {}
		KaxClusterPosition(const KaxClusterPosition & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxClusterPosition);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxClusterPosition(*this);}
};

class MATROSKA_DLL_API KaxClusterPrevSize : public EbmlUInteger {
	public:
		KaxClusterPrevSize() {}
		KaxClusterPrevSize(const KaxClusterPrevSize & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxClusterPrevSize);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxClusterPrevSize(*this);}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_CLUSTER_DATA_H

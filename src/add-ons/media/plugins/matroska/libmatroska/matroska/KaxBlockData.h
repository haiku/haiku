/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: KaxBlockData.h,v 1.10 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_BLOCK_ADDITIONAL_H
#define LIBMATROSKA_BLOCK_ADDITIONAL_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlSInteger.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class KaxReferenceBlock;
class KaxBlockGroup;
class KaxBlockBlob;

class MATROSKA_DLL_API KaxReferencePriority : public EbmlUInteger {
	public:
		KaxReferencePriority() :EbmlUInteger(0) {}
		KaxReferencePriority(const KaxReferencePriority & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxReferencePriority);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxReferencePriority(*this);}
};

/*!
	\brief element used for B frame-likes
*/
class MATROSKA_DLL_API KaxReferenceBlock : public EbmlSInteger {
	public:
		KaxReferenceBlock() :RefdBlock(NULL), ParentBlock(NULL) {bTimecodeSet = false;}
		KaxReferenceBlock(const KaxReferenceBlock & ElementToClone) :EbmlSInteger(ElementToClone), bTimecodeSet(ElementToClone.bTimecodeSet) {}
		static EbmlElement & Create() {return *(new KaxReferenceBlock);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxReferenceBlock(*this);}
		
		/*!
			\brief override this method to compute the timecode value
		*/
		virtual uint64 UpdateSize(bool bSaveDefault = false, bool bForceRender = false);

		const KaxBlockBlob & RefBlock() const;
		void SetReferencedBlock(const KaxBlockBlob * aRefdBlock);
		void SetReferencedBlock(const KaxBlockGroup & aRefdBlock);
		void SetParentBlock(const KaxBlockGroup & aParentBlock) {ParentBlock = &aParentBlock;}
		
	protected:
		const KaxBlockBlob * RefdBlock;
		const KaxBlockGroup * ParentBlock;
		void SetReferencedTimecode(int64 refTimecode) {Value = refTimecode; bTimecodeSet = true; bValueIsSet = true;};
		bool bTimecodeSet;
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxReferenceVirtual : public EbmlSInteger {
	public:
		KaxReferenceVirtual() {}
		KaxReferenceVirtual(const KaxReferenceVirtual & ElementToClone) :EbmlSInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxReferenceVirtual);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxReferenceVirtual(*this);}
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxTimeSlice : public EbmlMaster {
	public:
		KaxTimeSlice();
		KaxTimeSlice(const KaxTimeSlice & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTimeSlice);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTimeSlice(*this);}
};

class MATROSKA_DLL_API KaxSlices : public EbmlMaster {
	public:
		KaxSlices();
		KaxSlices(const KaxSlices & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxSlices);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxSlices(*this);}
};

class MATROSKA_DLL_API KaxSliceLaceNumber : public EbmlUInteger {
	public:
		KaxSliceLaceNumber() :EbmlUInteger(0) {}
		KaxSliceLaceNumber(const KaxSliceLaceNumber & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxSliceLaceNumber);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxSliceLaceNumber(*this);}
};

class MATROSKA_DLL_API KaxSliceFrameNumber : public EbmlUInteger {
	public:
		KaxSliceFrameNumber() :EbmlUInteger(0) {}
		KaxSliceFrameNumber(const KaxSliceFrameNumber & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxSliceFrameNumber);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxSliceFrameNumber(*this);}
};

class MATROSKA_DLL_API KaxSliceBlockAddID : public EbmlUInteger {
	public:
		KaxSliceBlockAddID() :EbmlUInteger(0) {}
		KaxSliceBlockAddID(const KaxSliceBlockAddID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxSliceBlockAddID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxSliceBlockAddID(*this);}
};

class MATROSKA_DLL_API KaxSliceDelay : public EbmlUInteger {
	public:
		KaxSliceDelay() :EbmlUInteger(0) {}
		KaxSliceDelay(const KaxSliceDelay & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxSliceDelay);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxSliceDelay(*this);}
};

class MATROSKA_DLL_API KaxSliceDuration : public EbmlUInteger {
	public:
		KaxSliceDuration() {}
		KaxSliceDuration(const KaxSliceDuration & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxSliceDuration);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxSliceDuration(*this);}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_BLOCK_ADDITIONAL_H

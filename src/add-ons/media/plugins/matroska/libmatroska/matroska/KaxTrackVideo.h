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
	\version \$Id: KaxTrackVideo.h,v 1.9 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_TRACK_VIDEO_H
#define LIBMATROSKA_TRACK_VIDEO_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlBinary.h"
#include "ebml/EbmlFloat.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxTrackVideo : public EbmlMaster {
	public:
		KaxTrackVideo();
		KaxTrackVideo(const KaxTrackVideo & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackVideo);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackVideo(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxVideoFlagInterlaced : public EbmlUInteger {
	public:
		KaxVideoFlagInterlaced() :EbmlUInteger(0) {}
		KaxVideoFlagInterlaced(const KaxVideoFlagInterlaced & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoFlagInterlaced);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoFlagInterlaced(*this);}
};

class MATROSKA_DLL_API KaxVideoStereoMode : public EbmlUInteger {
	public:
		KaxVideoStereoMode() :EbmlUInteger(0) {}
		KaxVideoStereoMode(const KaxVideoStereoMode & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoStereoMode);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoStereoMode(*this);}
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxVideoPixelWidth : public EbmlUInteger {
	public:
		KaxVideoPixelWidth() {}
		KaxVideoPixelWidth(const KaxVideoPixelWidth & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoPixelWidth);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoPixelWidth(*this);}
};

class MATROSKA_DLL_API KaxVideoPixelHeight : public EbmlUInteger {
	public:
		KaxVideoPixelHeight() {}
		KaxVideoPixelHeight(const KaxVideoPixelHeight & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoPixelHeight);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoPixelHeight(*this);}
};

class MATROSKA_DLL_API KaxVideoPixelCropBottom : public EbmlUInteger {
	public:
		KaxVideoPixelCropBottom(): EbmlUInteger(0) {}
		KaxVideoPixelCropBottom(const KaxVideoPixelCropBottom & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoPixelCropBottom);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoPixelCropBottom(*this);}
};

class MATROSKA_DLL_API KaxVideoPixelCropTop : public EbmlUInteger {
	public:
		KaxVideoPixelCropTop(): EbmlUInteger(0) {}
		KaxVideoPixelCropTop(const KaxVideoPixelCropTop & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoPixelCropTop);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoPixelCropTop(*this);}
};

class MATROSKA_DLL_API KaxVideoPixelCropLeft : public EbmlUInteger {
	public:
		KaxVideoPixelCropLeft(): EbmlUInteger(0) {}
		KaxVideoPixelCropLeft(const KaxVideoPixelCropLeft & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoPixelCropLeft);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoPixelCropLeft(*this);}
};

class MATROSKA_DLL_API KaxVideoPixelCropRight : public EbmlUInteger {
	public:
		KaxVideoPixelCropRight(): EbmlUInteger(0) {}
		KaxVideoPixelCropRight(const KaxVideoPixelCropRight & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoPixelCropRight);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoPixelCropRight(*this);}
};

class MATROSKA_DLL_API KaxVideoDisplayWidth : public EbmlUInteger {
	public:
		KaxVideoDisplayWidth() {}
		KaxVideoDisplayWidth(const KaxVideoDisplayWidth & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoDisplayWidth);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoDisplayWidth(*this);}
};

class MATROSKA_DLL_API KaxVideoDisplayHeight : public EbmlUInteger {
	public:
		KaxVideoDisplayHeight() {}
		KaxVideoDisplayHeight(const KaxVideoDisplayHeight & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoDisplayHeight);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoDisplayHeight(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxVideoDisplayUnit : public EbmlUInteger {
	public:
		KaxVideoDisplayUnit() {}
		KaxVideoDisplayUnit(const KaxVideoDisplayUnit & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoDisplayUnit);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoDisplayUnit(*this);}
};

class MATROSKA_DLL_API KaxVideoAspectRatio : public EbmlUInteger {
	public:
		KaxVideoAspectRatio() {}
		KaxVideoAspectRatio(const KaxVideoAspectRatio & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoAspectRatio);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoAspectRatio(*this);}
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxVideoColourSpace : public EbmlBinary {
	public:
		KaxVideoColourSpace() {}
		KaxVideoColourSpace(const KaxVideoColourSpace & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxVideoColourSpace);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		bool ValidateSize(void) const {return (Size == 4);}
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoColourSpace(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxVideoGamma : public EbmlFloat {
	public:
		KaxVideoGamma() {}
		KaxVideoGamma(const KaxVideoGamma & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoGamma);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxVideoGamma(*this);}
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxVideoFrameRate : public EbmlFloat {
	public:
		KaxVideoFrameRate() {}
		KaxVideoFrameRate(const KaxVideoFrameRate & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxVideoFrameRate);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bSaveDefault);
		EbmlElement * Clone() const {return new KaxVideoFrameRate(*this);}
};


END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_TRACK_VIDEO_H

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
	\version \$Id: KaxTrackAudio.h,v 1.11 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_TRACK_AUDIO_H
#define LIBMATROSKA_TRACK_AUDIO_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlFloat.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlBinary.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxTrackAudio : public EbmlMaster {
	public:
		KaxTrackAudio();
		KaxTrackAudio(const KaxTrackAudio & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackAudio);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackAudio(*this);}
};

class MATROSKA_DLL_API KaxAudioSamplingFreq : public EbmlFloat {
	public:
		KaxAudioSamplingFreq() :EbmlFloat(8000.0) {}
		KaxAudioSamplingFreq(const KaxAudioSamplingFreq & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxAudioSamplingFreq);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxAudioSamplingFreq(*this);}
};

class MATROSKA_DLL_API KaxAudioOutputSamplingFreq : public EbmlFloat {
	public:
		KaxAudioOutputSamplingFreq() :EbmlFloat() {}
		KaxAudioOutputSamplingFreq(const KaxAudioOutputSamplingFreq & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxAudioOutputSamplingFreq);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxAudioOutputSamplingFreq(*this);}
};

class MATROSKA_DLL_API KaxAudioChannels : public EbmlUInteger {
	public:
		KaxAudioChannels() :EbmlUInteger(1) {}
		KaxAudioChannels(const KaxAudioChannels & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxAudioChannels);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxAudioChannels(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxAudioPosition : public EbmlBinary {
	public:
		KaxAudioPosition() {}
		KaxAudioPosition(const KaxAudioPosition & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxAudioPosition);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		bool ValidateSize(void) const {return true;}
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxAudioPosition(*this);}
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxAudioBitDepth : public EbmlUInteger {
	public:
		KaxAudioBitDepth() {}
		KaxAudioBitDepth(const KaxAudioBitDepth & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxAudioBitDepth);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxAudioBitDepth(*this);}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_TRACK_AUDIO_H

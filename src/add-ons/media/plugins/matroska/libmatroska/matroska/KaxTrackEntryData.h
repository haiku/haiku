/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: KaxTrackEntryData.h,v 1.9 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author John Cannon      <spyder2555 @ users.sf.net>
*/
#ifndef LIBMATROSKA_TRACK_ENTRY_DATA_H
#define LIBMATROSKA_TRACK_ENTRY_DATA_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlFloat.h"
#include "ebml/EbmlString.h"
#include "ebml/EbmlUnicodeString.h"
#include "ebml/EbmlBinary.h"
#include "ebml/EbmlMaster.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxTrackNumber : public EbmlUInteger {
	public:
		KaxTrackNumber() {}
		KaxTrackNumber(const KaxTrackNumber & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackNumber);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackNumber(*this);}
};

class MATROSKA_DLL_API KaxTrackUID : public EbmlUInteger {
	public:
		KaxTrackUID() {}
		KaxTrackUID(const KaxTrackUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackUID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackUID(*this);}
};

class MATROSKA_DLL_API KaxTrackType : public EbmlUInteger {
	public:
		KaxTrackType() {}
		KaxTrackType(const KaxTrackType & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackType);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackType(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxTrackFlagEnabled : public EbmlUInteger {
	public:
		KaxTrackFlagEnabled() :EbmlUInteger(1) {}
		KaxTrackFlagEnabled(const KaxTrackFlagEnabled & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackFlagEnabled);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackFlagEnabled(*this);}
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxTrackFlagDefault : public EbmlUInteger {
	public:
		KaxTrackFlagDefault() :EbmlUInteger(1) {}
		KaxTrackFlagDefault(const KaxTrackFlagDefault & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackFlagDefault);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackFlagDefault(*this);}
};

class MATROSKA_DLL_API KaxTrackFlagForced : public EbmlUInteger {
	public:
		KaxTrackFlagForced() :EbmlUInteger(0) {}
		KaxTrackFlagForced(const KaxTrackFlagForced & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackFlagForced);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackFlagForced(*this);}
};

class MATROSKA_DLL_API KaxTrackFlagLacing : public EbmlUInteger {
	public:
		KaxTrackFlagLacing() :EbmlUInteger(1) {}
		KaxTrackFlagLacing(const KaxTrackFlagLacing & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackFlagLacing);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackFlagLacing(*this);}
};

class MATROSKA_DLL_API KaxTrackMinCache : public EbmlUInteger {
	public:
		KaxTrackMinCache() :EbmlUInteger(0) {}
		KaxTrackMinCache(const KaxTrackMinCache & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackMinCache);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackMinCache(*this);}
};

class MATROSKA_DLL_API KaxTrackMaxCache : public EbmlUInteger {
	public:
		KaxTrackMaxCache() {}
		KaxTrackMaxCache(const KaxTrackMaxCache & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackMaxCache);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackMaxCache(*this);}
};

class MATROSKA_DLL_API KaxTrackDefaultDuration : public EbmlUInteger {
	public:
		KaxTrackDefaultDuration() {}
		KaxTrackDefaultDuration(const KaxTrackDefaultDuration & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackDefaultDuration);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackDefaultDuration(*this);}
};

class MATROSKA_DLL_API KaxTrackTimecodeScale : public EbmlFloat {
	public:
		KaxTrackTimecodeScale() :EbmlFloat(1.0) {}
		KaxTrackTimecodeScale(const KaxTrackTimecodeScale & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackTimecodeScale);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackTimecodeScale(*this);}
};

class MATROSKA_DLL_API KaxMaxBlockAdditionID : public EbmlUInteger {
	public:
		KaxMaxBlockAdditionID() :EbmlUInteger(0) {}
		KaxMaxBlockAdditionID(const KaxMaxBlockAdditionID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxMaxBlockAdditionID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxMaxBlockAdditionID(*this);}
};

class MATROSKA_DLL_API KaxTrackName : public EbmlUnicodeString {
	public:
		KaxTrackName() {}
		KaxTrackName(const KaxTrackName & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackName);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackName(*this);}
};

class MATROSKA_DLL_API KaxTrackLanguage : public EbmlString {
	public:
		KaxTrackLanguage() :EbmlString("eng") {}
		KaxTrackLanguage(const KaxTrackLanguage & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackLanguage);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackLanguage(*this);}
};

class MATROSKA_DLL_API KaxCodecID : public EbmlString {
	public:
		KaxCodecID() {}
		KaxCodecID(const KaxCodecID & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxCodecID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecID(*this);}
};

class MATROSKA_DLL_API KaxCodecPrivate : public EbmlBinary {
	public:
		KaxCodecPrivate() {}
		KaxCodecPrivate(const KaxCodecPrivate & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxCodecPrivate);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecPrivate(*this);}
};

class MATROSKA_DLL_API KaxCodecName : public EbmlUnicodeString {
	public:
		KaxCodecName() {}
		KaxCodecName(const KaxCodecName & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxCodecName);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecName(*this);}
};

class MATROSKA_DLL_API KaxTrackAttachmentLink : public EbmlBinary {
	public:
		KaxTrackAttachmentLink() {}
		KaxTrackAttachmentLink(const KaxTrackAttachmentLink & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTrackAttachmentLink);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackAttachmentLink(*this);}
};

class MATROSKA_DLL_API KaxTrackOverlay : public EbmlUInteger {
	public:
		KaxTrackOverlay() {}
		KaxTrackOverlay(const KaxTrackOverlay & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackOverlay);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackOverlay(*this);}
};

class MATROSKA_DLL_API KaxTrackTranslate : public EbmlMaster {
	public:
		KaxTrackTranslate();
		KaxTrackTranslate(const KaxTrackTranslate & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackTranslate);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackTranslate(*this);}
};

class MATROSKA_DLL_API KaxTrackTranslateCodec : public EbmlUInteger {
	public:
		KaxTrackTranslateCodec() {}
		KaxTrackTranslateCodec(const KaxTrackTranslateCodec & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackTranslateCodec);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackTranslateCodec(*this);}
};

class MATROSKA_DLL_API KaxTrackTranslateEditionUID : public EbmlUInteger {
	public:
		KaxTrackTranslateEditionUID() {}
		KaxTrackTranslateEditionUID(const KaxTrackTranslateEditionUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTrackTranslateEditionUID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackTranslateEditionUID(*this);}
};

class MATROSKA_DLL_API KaxTrackTranslateTrackID : public EbmlBinary {
	public:
		KaxTrackTranslateTrackID() {}
		KaxTrackTranslateTrackID(const KaxTrackTranslateTrackID & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTrackTranslateTrackID);}
		bool ValidateSize() const { return true;}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTrackTranslateTrackID(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxCodecSettings : public EbmlUnicodeString {
	public:
		KaxCodecSettings() {}
		KaxCodecSettings(const KaxCodecSettings & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxCodecSettings);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecSettings(*this);}
};

class MATROSKA_DLL_API KaxCodecInfoURL : public EbmlString {
	public:
		KaxCodecInfoURL() {}
		KaxCodecInfoURL(const KaxCodecInfoURL & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxCodecInfoURL);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecInfoURL(*this);}
};

class MATROSKA_DLL_API KaxCodecDownloadURL : public EbmlString {
	public:
		KaxCodecDownloadURL() {}
		KaxCodecDownloadURL(const KaxCodecDownloadURL & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxCodecDownloadURL);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecDownloadURL(*this);}
};

class MATROSKA_DLL_API KaxCodecDecodeAll : public EbmlUInteger {
	public:
		KaxCodecDecodeAll() :EbmlUInteger(1) {}
		KaxCodecDecodeAll(const KaxCodecDecodeAll & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxCodecDecodeAll);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCodecDecodeAll(*this);}
};
#endif // MATROSKA_VERSION

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_TRACK_ENTRY_DATA_H

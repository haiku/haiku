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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxTag.h,v 1.9 2004/04/14 23:26:17 robux4 Exp $
	\author Jory Stone     <jcsston @ toughguy.net>
	\author Steve Lhomme   <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_TAG_H
#define LIBMATROSKA_TAG_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlFloat.h"
#include "ebml/EbmlSInteger.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlString.h"
#include "ebml/EbmlUnicodeString.h"
#include "ebml/EbmlBinary.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxTag : public EbmlMaster {
	public:
		KaxTag();
		KaxTag(const KaxTag & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTag);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTag(*this);}
};

class MATROSKA_DLL_API KaxTagTargets : public EbmlMaster {
	public:
		KaxTagTargets();
		KaxTagTargets(const KaxTagTargets & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagTargets);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagTargets(*this);}
};

class MATROSKA_DLL_API KaxTagGeneral : public EbmlMaster {
	public:
		KaxTagGeneral();
		KaxTagGeneral(const KaxTagGeneral & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagGeneral);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagGeneral(*this);}
};

class MATROSKA_DLL_API KaxTagGenres : public EbmlMaster {
	public:
		KaxTagGenres();
		KaxTagGenres(const KaxTagGenres & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagGenres);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagGenres(*this);}
};

class MATROSKA_DLL_API KaxTagAudioSpecific : public EbmlMaster {
	public:
		KaxTagAudioSpecific();
		KaxTagAudioSpecific(const KaxTagAudioSpecific & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagAudioSpecific);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagAudioSpecific(*this);}
};

class MATROSKA_DLL_API KaxTagImageSpecific : public EbmlMaster {
	public:
		KaxTagImageSpecific();
		KaxTagImageSpecific(const KaxTagImageSpecific & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagImageSpecific);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagImageSpecific(*this);}
};

class MATROSKA_DLL_API KaxTagTargetTypeValue : public EbmlUInteger {
	public:
		KaxTagTargetTypeValue() :EbmlUInteger(50) {}
		KaxTagTargetTypeValue(const KaxTagTargetTypeValue & ElementToClone) :EbmlUInteger(ElementToClone) {}

		static EbmlElement & Create() {return *(new KaxTagTargetTypeValue);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagTargetTypeValue(*this);}
};

class MATROSKA_DLL_API KaxTagTargetType : public EbmlString {
	public:
		KaxTagTargetType() {}
		KaxTagTargetType(const KaxTagTargetType & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagTargetType);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagTargetType(*this);}
};

class MATROSKA_DLL_API KaxTagTrackUID : public EbmlUInteger {
	public:
		KaxTagTrackUID() :EbmlUInteger(0) {}
		KaxTagTrackUID(const KaxTagTrackUID & ElementToClone) :EbmlUInteger(ElementToClone) {}

		static EbmlElement & Create() {return *(new KaxTagTrackUID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagTrackUID(*this);}
};

class MATROSKA_DLL_API KaxTagEditionUID : public EbmlUInteger {
	public:
		KaxTagEditionUID() :EbmlUInteger(0) {}
		KaxTagEditionUID(const KaxTagEditionUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagEditionUID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagEditionUID(*this);}
};

class MATROSKA_DLL_API KaxTagChapterUID : public EbmlUInteger {
	public:
		KaxTagChapterUID() :EbmlUInteger(0) {}
		KaxTagChapterUID(const KaxTagChapterUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagChapterUID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagChapterUID(*this);}
};

class MATROSKA_DLL_API KaxTagAttachmentUID : public EbmlUInteger {
	public:
		KaxTagAttachmentUID() :EbmlUInteger(0) {}
		KaxTagAttachmentUID(const KaxTagAttachmentUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagAttachmentUID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagAttachmentUID(*this);}
};

class MATROSKA_DLL_API KaxTagArchivalLocation : public EbmlUnicodeString {
	public:
		KaxTagArchivalLocation() {}
		KaxTagArchivalLocation(const KaxTagArchivalLocation & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagArchivalLocation);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagArchivalLocation(*this);}
};

class MATROSKA_DLL_API KaxTagAudioEncryption : public EbmlBinary {
	public:
		KaxTagAudioEncryption() {}
		KaxTagAudioEncryption(const KaxTagAudioEncryption & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagAudioEncryption);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagAudioEncryption(*this);}
};

class MATROSKA_DLL_API KaxTagAudioGain : public EbmlFloat {
	public:
		KaxTagAudioGain() {}
		KaxTagAudioGain(const KaxTagAudioGain & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagAudioGain);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagAudioGain(*this);}
};

class MATROSKA_DLL_API KaxTagAudioGenre : public EbmlString {
	public:
		KaxTagAudioGenre() {}
		KaxTagAudioGenre(const KaxTagAudioGenre & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagAudioGenre);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagAudioGenre(*this);}
};

class MATROSKA_DLL_API KaxTagAudioPeak : public EbmlFloat {
	public:
		KaxTagAudioPeak() {}
		KaxTagAudioPeak(const KaxTagAudioPeak & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagAudioPeak);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagAudioPeak(*this);}
};

class MATROSKA_DLL_API KaxTagBibliography : public EbmlUnicodeString {
	public:
		KaxTagBibliography() {}
		KaxTagBibliography(const KaxTagBibliography & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagBibliography);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagBibliography(*this);}
};

class MATROSKA_DLL_API KaxTagBPM : public EbmlFloat {
	public:
		KaxTagBPM() {}
		KaxTagBPM(const KaxTagBPM & ElementToClone) :EbmlFloat(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagBPM);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagBPM(*this);}
};

class MATROSKA_DLL_API KaxTagCaptureDPI : public EbmlUInteger {
	public:
		KaxTagCaptureDPI() {}
		KaxTagCaptureDPI(const KaxTagCaptureDPI & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagCaptureDPI);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagCaptureDPI(*this);}
};

class MATROSKA_DLL_API KaxTagCaptureLightness : public EbmlBinary {
	public:
		KaxTagCaptureLightness() {}
		KaxTagCaptureLightness(const KaxTagCaptureLightness & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagCaptureLightness);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagCaptureLightness(*this);}
};

class MATROSKA_DLL_API KaxTagCapturePaletteSetting : public EbmlUInteger {
	public:
		KaxTagCapturePaletteSetting() {}
		KaxTagCapturePaletteSetting(const KaxTagCapturePaletteSetting & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagCapturePaletteSetting);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagCapturePaletteSetting(*this);}
};

class MATROSKA_DLL_API KaxTagCaptureSharpness : public EbmlBinary {
	public:
		KaxTagCaptureSharpness() {}
		KaxTagCaptureSharpness(const KaxTagCaptureSharpness & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagCaptureSharpness);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagCaptureSharpness(*this);}
};

class MATROSKA_DLL_API KaxTagCropped : public EbmlUnicodeString {
	public:
		KaxTagCropped() {}
		KaxTagCropped(const KaxTagCropped & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagCropped);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagCropped(*this);}
};

class MATROSKA_DLL_API KaxTagDiscTrack : public EbmlUInteger {
	public:
		KaxTagDiscTrack() {}
		KaxTagDiscTrack(const KaxTagDiscTrack & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagDiscTrack);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagDiscTrack(*this);}
};

class MATROSKA_DLL_API KaxTagEncoder : public EbmlUnicodeString {
	public:
		KaxTagEncoder() {}
		KaxTagEncoder(const KaxTagEncoder & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagEncoder);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagEncoder(*this);}
};

class MATROSKA_DLL_API KaxTagEncodeSettings : public EbmlUnicodeString {
	public:
		KaxTagEncodeSettings() {}
		KaxTagEncodeSettings(const KaxTagEncodeSettings & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagEncodeSettings);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagEncodeSettings(*this);}
};

class MATROSKA_DLL_API KaxTagEqualisation : public EbmlBinary {
	public:
		KaxTagEqualisation() {}
		KaxTagEqualisation(const KaxTagEqualisation & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagEqualisation);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagEqualisation(*this);}
};

class MATROSKA_DLL_API KaxTagFile : public EbmlUnicodeString {
	public:
		KaxTagFile() {}
		KaxTagFile(const KaxTagFile & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagFile);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagFile(*this);}
};

class MATROSKA_DLL_API KaxTagInitialKey : public EbmlString {
	public:
		KaxTagInitialKey() {}
		KaxTagInitialKey(const KaxTagInitialKey & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagInitialKey);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagInitialKey(*this);}
};

class MATROSKA_DLL_API KaxTagKeywords : public EbmlUnicodeString {
	public:
		KaxTagKeywords() {}
		KaxTagKeywords(const KaxTagKeywords & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagKeywords);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagKeywords(*this);}
};

class MATROSKA_DLL_API KaxTagLanguage : public EbmlString {
	public:
		KaxTagLanguage() {}
		KaxTagLanguage(const KaxTagLanguage & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagLanguage);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagLanguage(*this);}
};

class MATROSKA_DLL_API KaxTagLength : public EbmlUInteger {
	public:
		KaxTagLength() {}
		KaxTagLength(const KaxTagLength & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagLength);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagLength(*this);}
};

class MATROSKA_DLL_API KaxTagMood : public EbmlUnicodeString {
	public:
		KaxTagMood() {}
		KaxTagMood(const KaxTagMood & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagMood);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagMood(*this);}
};

class MATROSKA_DLL_API KaxTagOfficialAudioFileURL : public EbmlString {
	public:
		KaxTagOfficialAudioFileURL() {}
		KaxTagOfficialAudioFileURL(const KaxTagOfficialAudioFileURL & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagOfficialAudioFileURL);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagOfficialAudioFileURL(*this);}
};

class MATROSKA_DLL_API KaxTagOfficialAudioSourceURL : public EbmlString {
	public:
		KaxTagOfficialAudioSourceURL() {}
		KaxTagOfficialAudioSourceURL(const KaxTagOfficialAudioSourceURL & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagOfficialAudioSourceURL);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagOfficialAudioSourceURL(*this);}
};

class MATROSKA_DLL_API KaxTagOriginalDimensions : public EbmlString {
	public:
		KaxTagOriginalDimensions() {}
		KaxTagOriginalDimensions(const KaxTagOriginalDimensions & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagOriginalDimensions);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagOriginalDimensions(*this);}
};

class MATROSKA_DLL_API KaxTagOriginalMediaType : public EbmlUnicodeString {
	public:
		KaxTagOriginalMediaType() {}
		KaxTagOriginalMediaType(const KaxTagOriginalMediaType & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagOriginalMediaType);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagOriginalMediaType(*this);}
};

class MATROSKA_DLL_API KaxTagPlayCounter : public EbmlUInteger {
	public:
		KaxTagPlayCounter() {}
		KaxTagPlayCounter(const KaxTagPlayCounter & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagPlayCounter);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagPlayCounter(*this);}
};			 

class MATROSKA_DLL_API KaxTagPlaylistDelay : public EbmlUInteger {
	public:
		KaxTagPlaylistDelay() {}
		KaxTagPlaylistDelay(const KaxTagPlaylistDelay & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagPlaylistDelay);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagPlaylistDelay(*this);}
};

class MATROSKA_DLL_API KaxTagPopularimeter : public EbmlSInteger {
	public:
		KaxTagPopularimeter() {}
		KaxTagPopularimeter(const KaxTagPopularimeter & ElementToClone) :EbmlSInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagPopularimeter);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagPopularimeter(*this);}
};

class MATROSKA_DLL_API KaxTagProduct : public EbmlUnicodeString {
	public:
		KaxTagProduct() {}
		KaxTagProduct(const KaxTagProduct & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagProduct);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagProduct(*this);}
};

class MATROSKA_DLL_API KaxTagRating : public EbmlBinary {
	public:
		KaxTagRating() {}
		KaxTagRating(const KaxTagRating & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagRating);}
		bool ValidateSize() const {return true;} // we don't mind about what's inside
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagRating(*this);}
};

class MATROSKA_DLL_API KaxTagRecordLocation : public EbmlString {
	public:
		KaxTagRecordLocation() {}
		KaxTagRecordLocation(const KaxTagRecordLocation & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagRecordLocation);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagRecordLocation(*this);}
};

class MATROSKA_DLL_API KaxTagSetPart : public EbmlUInteger {
	public:
		KaxTagSetPart() {}
		KaxTagSetPart(const KaxTagSetPart & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagSetPart);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagSetPart(*this);}
};

class MATROSKA_DLL_API KaxTagSource : public EbmlUnicodeString {
	public:
		KaxTagSource() {}
		KaxTagSource(const KaxTagSource & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagSource);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagSource(*this);}
};

class MATROSKA_DLL_API KaxTagSourceForm : public EbmlUnicodeString {
	public:
		KaxTagSourceForm() {}
		KaxTagSourceForm(const KaxTagSourceForm & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagSourceForm);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagSourceForm(*this);}
};

class MATROSKA_DLL_API KaxTagSubGenre : public EbmlString {
	public:
		KaxTagSubGenre() {}
		KaxTagSubGenre(const KaxTagSubGenre & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagSubGenre);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagSubGenre(*this);}
};

class MATROSKA_DLL_API KaxTagSubject : public EbmlUnicodeString {
	public:
		KaxTagSubject() {}
		KaxTagSubject(const KaxTagSubject & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagSubject);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagSubject(*this);}
};

class MATROSKA_DLL_API KaxTagUnsynchronisedText : public EbmlUnicodeString {
	public:
		KaxTagUnsynchronisedText() {}
		KaxTagUnsynchronisedText(const KaxTagUnsynchronisedText & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagUnsynchronisedText);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagUnsynchronisedText(*this);}
};

class MATROSKA_DLL_API KaxTagUserDefinedURL : public EbmlString {
	public:
		KaxTagUserDefinedURL() {}
		KaxTagUserDefinedURL(const KaxTagUserDefinedURL & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagUserDefinedURL);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagUserDefinedURL(*this);}
};

class MATROSKA_DLL_API KaxTagVideoGenre : public EbmlBinary {
	public:
		KaxTagVideoGenre() {}
		KaxTagVideoGenre(const KaxTagVideoGenre & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagVideoGenre);}
		bool ValidateSize() const {return (Size >= 2);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagVideoGenre(*this);}
};

class MATROSKA_DLL_API KaxTagSimple : public EbmlMaster {
	public:
		KaxTagSimple();
		KaxTagSimple(const KaxTagSimple & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagSimple);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagSimple(*this);}
};

class MATROSKA_DLL_API KaxTagName : public EbmlUnicodeString {
	public:
		KaxTagName() {}
		KaxTagName(const KaxTagName & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagName);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagName(*this);}
};

class MATROSKA_DLL_API KaxTagLangue : public EbmlString {
	public:
		KaxTagLangue(): EbmlString("und") {}
		KaxTagLangue(const KaxTagLangue & ElementToClone) :EbmlString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagLangue);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagLangue(*this);}
};

class MATROSKA_DLL_API KaxTagDefault : public EbmlUInteger {
	public:
		KaxTagDefault() :EbmlUInteger(1) {}
		KaxTagDefault(const KaxTagTrackUID & ElementToClone) :EbmlUInteger(ElementToClone) {}

		static EbmlElement & Create() {return *(new KaxTagDefault);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagDefault(*this);}
};

class MATROSKA_DLL_API KaxTagString : public EbmlUnicodeString {
	public:
		KaxTagString() {}
		KaxTagString(const KaxTagString & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxTagString);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagString(*this);}
};

class MATROSKA_DLL_API KaxTagBinary : public EbmlBinary {
	public:
		KaxTagBinary() {}
		KaxTagBinary(const KaxTagBinary & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxTagBinary);}
		bool ValidateSize() const {return (Size >= 0);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxTagBinary(*this);}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_TAG_H

/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
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
	\version \$Id: KaxTag.cpp 733 2004-08-27 12:25:01Z robux4 $
	\author Jory Stone       <jcsston @ toughguy.net>
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxTag.h"
#include "matroska/KaxTagMulti.h"
#include "matroska/KaxContexts.h"

using namespace LIBEBML_NAMESPACE;

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxTag_ContextList[14] =
{	
	EbmlSemantic(true,  true,  KaxTagTargets::ClassInfos),
	EbmlSemantic(false, true,  KaxTagGeneral::ClassInfos),
	EbmlSemantic(false, true,  KaxTagGenres::ClassInfos),
	EbmlSemantic(false, true,  KaxTagAudioSpecific::ClassInfos),
	EbmlSemantic(false, true,  KaxTagImageSpecific::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiCommercial::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiDate::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiEntity::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiIdentifier::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiLegal::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiTitle::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiAttachment::ClassInfos),
//	EbmlSemantic(false, false, KaxTagLength::ClassInfos),
//	EbmlSemantic(false, false, KaxTagPlaylistDelay::ClassInfos),
//	EbmlSemantic(false, false, KaxTagUnsynchronisedText::ClassInfos),
//	EbmlSemantic(false, false, KaxTagUserDefinedURL::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiComment::ClassInfos),
	EbmlSemantic(true,  false, KaxTagSimple::ClassInfos),
};

EbmlSemantic KaxTagTargets_ContextList[6] =
{	
	EbmlSemantic(false, true,  KaxTagTargetTypeValue::ClassInfos),
	EbmlSemantic(false, true,  KaxTagTargetType::ClassInfos),
	EbmlSemantic(false, false, KaxTagTrackUID::ClassInfos),
	EbmlSemantic(false, false, KaxTagEditionUID::ClassInfos),
	EbmlSemantic(false, false, KaxTagChapterUID::ClassInfos),
	EbmlSemantic(false, false, KaxTagAttachmentUID::ClassInfos),
};

EbmlSemantic KaxTagGeneral_ContextList[17] =
{	
	EbmlSemantic(false, true,  KaxTagArchivalLocation::ClassInfos),
	EbmlSemantic(false, true,  KaxTagFile::ClassInfos),
	EbmlSemantic(false, false, KaxTagKeywords::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMood::ClassInfos),
	EbmlSemantic(false, false, KaxTagRecordLocation::ClassInfos),
	EbmlSemantic(false, true,  KaxTagSource::ClassInfos),
	EbmlSemantic(false, true,  KaxTagSourceForm::ClassInfos),
	EbmlSemantic(false, true,  KaxTagProduct::ClassInfos),
	EbmlSemantic(false, true,  KaxTagOriginalMediaType::ClassInfos),
	EbmlSemantic(false, true,  KaxTagPlayCounter::ClassInfos),
	EbmlSemantic(false, true,  KaxTagPopularimeter::ClassInfos),
	EbmlSemantic(false, true,  KaxTagSubject::ClassInfos),
	EbmlSemantic(false, true,  KaxTagBibliography::ClassInfos),
	EbmlSemantic(false, true,  KaxTagLanguage::ClassInfos),
	EbmlSemantic(false, true,  KaxTagRating::ClassInfos),
	EbmlSemantic(false, true,  KaxTagEncoder::ClassInfos),
	EbmlSemantic(false, true,  KaxTagEncodeSettings::ClassInfos),
};

EbmlSemantic KaxTagGenres_ContextList[3] =
{	
	EbmlSemantic(false, false, KaxTagAudioGenre::ClassInfos),
	EbmlSemantic(false, false, KaxTagVideoGenre::ClassInfos),
	EbmlSemantic(false, true,  KaxTagSubGenre::ClassInfos),
};

EbmlSemantic KaxTagAudioSpecific_ContextList[10] =
{	
	EbmlSemantic(false, true, KaxTagAudioPeak::ClassInfos),
	EbmlSemantic(false, true, KaxTagAudioEncryption::ClassInfos),
	EbmlSemantic(false, true, KaxTagAudioGain::ClassInfos),
	EbmlSemantic(false, true, KaxTagBPM::ClassInfos),
	EbmlSemantic(false, true, KaxTagDiscTrack::ClassInfos),
	EbmlSemantic(false, true, KaxTagSetPart::ClassInfos),
	EbmlSemantic(false, true, KaxTagEqualisation::ClassInfos),
	EbmlSemantic(false, true, KaxTagInitialKey::ClassInfos),
	EbmlSemantic(false, true, KaxTagOfficialAudioFileURL::ClassInfos),
	EbmlSemantic(false, true, KaxTagOfficialAudioSourceURL::ClassInfos),
};

EbmlSemantic KaxTagImageSpecific_ContextList[6] =
{
	EbmlSemantic(false, true, KaxTagCaptureDPI::ClassInfos),
	EbmlSemantic(false, true, KaxTagCaptureLightness::ClassInfos),
	EbmlSemantic(false, true, KaxTagCapturePaletteSetting::ClassInfos),
	EbmlSemantic(false, true, KaxTagCaptureSharpness::ClassInfos),
	EbmlSemantic(false, true, KaxTagCropped::ClassInfos),
	EbmlSemantic(false, true, KaxTagOriginalDimensions::ClassInfos),
};

EbmlSemantic KaxTagSimple_ContextList[6] =
{	
	EbmlSemantic(true,  true,  KaxTagName::ClassInfos),
	EbmlSemantic(true,  true,  KaxTagLangue::ClassInfos),
	EbmlSemantic(true,  true,  KaxTagDefault::ClassInfos),
	EbmlSemantic(false, true,  KaxTagString::ClassInfos),
	EbmlSemantic(false, true,  KaxTagBinary::ClassInfos),
	EbmlSemantic(false, false, KaxTagSimple::ClassInfos),
};

const EbmlSemanticContext KaxTag_Context = EbmlSemanticContext(countof(KaxTag_ContextList), KaxTag_ContextList, &KaxTags_Context, *GetKaxTagsGlobal_Context, &KaxTag::ClassInfos);
const EbmlSemanticContext KaxTagTargets_Context = EbmlSemanticContext(countof(KaxTagTargets_ContextList), KaxTagTargets_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagTargets::ClassInfos);
const EbmlSemanticContext KaxTagGeneral_Context = EbmlSemanticContext(countof(KaxTagGeneral_ContextList), KaxTagGeneral_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagGeneral::ClassInfos);
const EbmlSemanticContext KaxTagGenres_Context = EbmlSemanticContext(countof(KaxTagGenres_ContextList), KaxTagGenres_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagGenres::ClassInfos);
const EbmlSemanticContext KaxTagAudioSpecific_Context = EbmlSemanticContext(countof(KaxTagAudioSpecific_ContextList), KaxTagAudioSpecific_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagAudioSpecific::ClassInfos);
const EbmlSemanticContext KaxTagImageSpecific_Context = EbmlSemanticContext(countof(KaxTagImageSpecific_ContextList), KaxTagImageSpecific_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagImageSpecific::ClassInfos);
const EbmlSemanticContext KaxTagBibliography_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagBibliography::ClassInfos);
const EbmlSemanticContext KaxTagEncoder_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagEncoder::ClassInfos);
const EbmlSemanticContext KaxTagEncodeSettings_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagEncodeSettings::ClassInfos);
const EbmlSemanticContext KaxTagLanguage_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagLanguage::ClassInfos);
const EbmlSemanticContext KaxTagLength_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagLength::ClassInfos);
const EbmlSemanticContext KaxTagPlaylistDelay_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagPlaylistDelay::ClassInfos);
const EbmlSemanticContext KaxTagRating_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagRating::ClassInfos);
const EbmlSemanticContext KaxTagSubject_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagSubject::ClassInfos);
const EbmlSemanticContext KaxTagUnsynchronisedText_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagUnsynchronisedText::ClassInfos);
const EbmlSemanticContext KaxTagUserDefinedURL_Context = EbmlSemanticContext(0, NULL, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagUserDefinedURL::ClassInfos);
const EbmlSemanticContext KaxTagTargetTypeValue_Context = EbmlSemanticContext(0, NULL, &KaxTagTargets_Context, *GetKaxGlobal_Context, &KaxTagTargetTypeValue::ClassInfos);
const EbmlSemanticContext KaxTagTargetType_Context = EbmlSemanticContext(0, NULL, &KaxTagTargets_Context, *GetKaxGlobal_Context, &KaxTagTargetType::ClassInfos);
const EbmlSemanticContext KaxTagTrackUID_Context = EbmlSemanticContext(0, NULL, &KaxTagTargets_Context, *GetKaxGlobal_Context, &KaxTagTrackUID::ClassInfos);
const EbmlSemanticContext KaxTagEditionUID_Context = EbmlSemanticContext(0, NULL, &KaxTagTargets_Context, *GetKaxGlobal_Context, &KaxTagEditionUID::ClassInfos);
const EbmlSemanticContext KaxTagChapterUID_Context = EbmlSemanticContext(0, NULL, &KaxTagTargets_Context, *GetKaxGlobal_Context, &KaxTagChapterUID::ClassInfos);
const EbmlSemanticContext KaxTagAttachmentUID_Context = EbmlSemanticContext(0, NULL, &KaxTagTargets_Context, *GetKaxGlobal_Context, &KaxTagAttachmentUID::ClassInfos);
const EbmlSemanticContext KaxTagArchivalLocation_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagArchivalLocation::ClassInfos);
const EbmlSemanticContext KaxTagFile_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagFile::ClassInfos);
const EbmlSemanticContext KaxTagKeywords_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagKeywords::ClassInfos);
const EbmlSemanticContext KaxTagMood_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagMood::ClassInfos);
const EbmlSemanticContext KaxTagRecordLocation_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagRecordLocation::ClassInfos);
const EbmlSemanticContext KaxTagSource_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagSource::ClassInfos);
const EbmlSemanticContext KaxTagSourceForm_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagSourceForm::ClassInfos);
const EbmlSemanticContext KaxTagProduct_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagProduct::ClassInfos);
const EbmlSemanticContext KaxTagOriginalMediaType_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagOriginalMediaType::ClassInfos);
const EbmlSemanticContext KaxTagPlayCounter_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagPlayCounter::ClassInfos);
const EbmlSemanticContext KaxTagPopularimeter_Context = EbmlSemanticContext(0, NULL, &KaxTagGeneral_Context, *GetKaxGlobal_Context, &KaxTagPopularimeter::ClassInfos);
const EbmlSemanticContext KaxTagAudioGenre_Context = EbmlSemanticContext(0, NULL, &KaxTagGenres_Context, *GetKaxGlobal_Context, &KaxTagAudioGenre::ClassInfos);
const EbmlSemanticContext KaxTagVideoGenre_Context = EbmlSemanticContext(0, NULL, &KaxTagGenres_Context, *GetKaxGlobal_Context, &KaxTagVideoGenre::ClassInfos);
const EbmlSemanticContext KaxTagSubGenre_Context = EbmlSemanticContext(0, NULL, &KaxTagGenres_Context, *GetKaxGlobal_Context, &KaxTagSubGenre::ClassInfos);
const EbmlSemanticContext KaxTagAudioEncryption_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagAudioEncryption::ClassInfos);
const EbmlSemanticContext KaxTagAudioGain_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagAudioGain::ClassInfos);
const EbmlSemanticContext KaxTagAudioPeak_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagAudioPeak::ClassInfos);
const EbmlSemanticContext KaxTagBPM_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagBPM::ClassInfos);
const EbmlSemanticContext KaxTagDiscTrack_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagDiscTrack::ClassInfos);
const EbmlSemanticContext KaxTagSetPart_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagSetPart::ClassInfos);
const EbmlSemanticContext KaxTagEqualisation_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagEqualisation::ClassInfos);
const EbmlSemanticContext KaxTagInitialKey_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagInitialKey::ClassInfos);
const EbmlSemanticContext KaxTagOfficialAudioFileURL_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagOfficialAudioFileURL::ClassInfos);
const EbmlSemanticContext KaxTagOfficialAudioSourceURL_Context = EbmlSemanticContext(0, NULL, &KaxTagAudioSpecific_Context, *GetKaxGlobal_Context, &KaxTagOfficialAudioSourceURL::ClassInfos);
const EbmlSemanticContext KaxTagCaptureDPI_Context = EbmlSemanticContext(0, NULL, &KaxTagImageSpecific_Context, *GetKaxGlobal_Context, &KaxTagCaptureDPI::ClassInfos);
const EbmlSemanticContext KaxTagCaptureLightness_Context = EbmlSemanticContext(0, NULL, &KaxTagImageSpecific_Context, *GetKaxGlobal_Context, &KaxTagCaptureLightness::ClassInfos);
const EbmlSemanticContext KaxTagCapturePaletteSetting_Context = EbmlSemanticContext(0, NULL, &KaxTagImageSpecific_Context, *GetKaxGlobal_Context, &KaxTagCapturePaletteSetting::ClassInfos);
const EbmlSemanticContext KaxTagCaptureSharpness_Context = EbmlSemanticContext(0, NULL, &KaxTagImageSpecific_Context, *GetKaxGlobal_Context, &KaxTagCaptureSharpness::ClassInfos);
const EbmlSemanticContext KaxTagCropped_Context = EbmlSemanticContext(0, NULL, &KaxTagImageSpecific_Context, *GetKaxGlobal_Context, &KaxTagCropped::ClassInfos);
const EbmlSemanticContext KaxTagOriginalDimensions_Context = EbmlSemanticContext(0, NULL, &KaxTagImageSpecific_Context, *GetKaxGlobal_Context, &KaxTagOriginalDimensions::ClassInfos);

const EbmlSemanticContext KaxTagSimple_Context = EbmlSemanticContext(countof(KaxTagSimple_ContextList), KaxTagSimple_ContextList, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagSimple::ClassInfos);
const EbmlSemanticContext KaxTagName_Context = EbmlSemanticContext(0, NULL, &KaxTagSimple_Context, *GetKaxGlobal_Context, &KaxTagName::ClassInfos);
const EbmlSemanticContext KaxTagLangue_Context = EbmlSemanticContext(0, NULL, &KaxTagSimple_Context, *GetKaxGlobal_Context, &KaxTagLangue::ClassInfos);
const EbmlSemanticContext KaxTagDefault_Context = EbmlSemanticContext(0, NULL, &KaxTagSimple_Context, *GetKaxGlobal_Context, &KaxTagDefault::ClassInfos);
const EbmlSemanticContext KaxTagString_Context = EbmlSemanticContext(0, NULL, &KaxTagSimple_Context, *GetKaxGlobal_Context, &KaxTagString::ClassInfos);
const EbmlSemanticContext KaxTagBinary_Context = EbmlSemanticContext(0, NULL, &KaxTagSimple_Context, *GetKaxGlobal_Context, &KaxTagBinary::ClassInfos);

EbmlId KaxTag_TheId                      (0x7373, 2);
EbmlId KaxTagTargets_TheId               (0x63C0, 2);
EbmlId KaxTagGeneral_TheId               (0x67C9, 2);
EbmlId KaxTagGenres_TheId                (0x6583, 2);
EbmlId KaxTagAudioSpecific_TheId         (0x41C5, 2);
EbmlId KaxTagImageSpecific_TheId         (0x4990, 2);
EbmlId KaxTagBibliography_TheId          (0x4488, 2);
EbmlId KaxTagEncoder_TheId               (0x4431, 2);
EbmlId KaxTagEncodeSettings_TheId        (0x6526, 2);
EbmlId KaxTagLanguage_TheId              (0x22B59F, 3);
EbmlId KaxTagLength_TheId                (0x5243, 2);
EbmlId KaxTagPlaylistDelay_TheId         (0x72CC, 2);
EbmlId KaxTagRating_TheId                (0x52BC, 2);
EbmlId KaxTagSubject_TheId               (0x49C1, 2);
EbmlId KaxTagUnsynchronisedText_TheId    (0x874B, 2);
EbmlId KaxTagUserDefinedURL_TheId        (0x434A, 2);
EbmlId KaxTagTargetTypeValue_TheId       (0x68CA, 2);
EbmlId KaxTagTargetType_TheId            (0x63CA, 2);
EbmlId KaxTagTrackUID_TheId              (0x63C5, 2);
EbmlId KaxTagEditionUID_TheId            (0x63C9, 2);
EbmlId KaxTagChapterUID_TheId            (0x63C4, 2);
EbmlId KaxTagAttachmentUID_TheId         (0x63C6, 2);
EbmlId KaxTagAudioGenre_TheId            (0x65C2, 2);
EbmlId KaxTagVideoGenre_TheId            (0x65A1, 2);
EbmlId KaxTagSubGenre_TheId              (0x65AC, 2);
EbmlId KaxTagAudioEncryption_TheId       (0x41B4, 2);
EbmlId KaxTagAudioGain_TheId             (0x4199, 2);
EbmlId KaxTagAudioPeak_TheId             (0x4189, 2);
EbmlId KaxTagBPM_TheId                   (0x41A1, 2);
EbmlId KaxTagDiscTrack_TheId             (0x41B6, 2);
EbmlId KaxTagSetPart_TheId               (0x416E, 2);
EbmlId KaxTagEqualisation_TheId          (0x41B1, 2);
EbmlId KaxTagInitialKey_TheId            (0x413A, 2);
EbmlId KaxTagOfficialAudioFileURL_TheId	 (0x4133, 2);
EbmlId KaxTagOfficialAudioSourceURL_TheId(0x413E, 2);
EbmlId KaxTagArchivalLocation_TheId      (0x45A4, 2);
EbmlId KaxTagFile_TheId                  (0x454E, 2);
EbmlId KaxTagKeywords_TheId              (0x458C, 2);
EbmlId KaxTagMood_TheId                  (0x45AE, 2);
EbmlId KaxTagRecordLocation_TheId        (0x457E, 2);
EbmlId KaxTagSource_TheId                (0x458A, 2);
EbmlId KaxTagSourceForm_TheId            (0x45B5, 2);
EbmlId KaxTagProduct_TheId               (0x45E3, 2);
EbmlId KaxTagOriginalMediaType_TheId     (0x45A7, 2);
EbmlId KaxTagPlayCounter_TheId           (0x4566, 2);
EbmlId KaxTagPopularimeter_TheId         (0x4532, 2);
EbmlId KaxTagCaptureDPI_TheId            (0x49C7, 2);
EbmlId KaxTagCaptureLightness_TheId      (0x49E1, 2);
EbmlId KaxTagCapturePaletteSetting_TheId (0x4934, 2);
EbmlId KaxTagCaptureSharpness_TheId      (0x4922, 2);
EbmlId KaxTagCropped_TheId               (0x4987, 2);
EbmlId KaxTagOriginalDimensions_TheId    (0x4933, 2);

EbmlId KaxTagSimple_TheId								 (0x67C8, 2);
EbmlId KaxTagName_TheId									 (0x45A3, 2);
EbmlId KaxTagLangue_TheId								 (0x447A, 2);
EbmlId KaxTagDefault_TheId								 (0x44B4, 2);
EbmlId KaxTagString_TheId								 (0x4487, 2);
EbmlId KaxTagBinary_TheId								 (0x4485, 2);

const EbmlCallbacks KaxTag::ClassInfos(KaxTag::Create, KaxTag_TheId, "Tag", KaxTag_Context);
const EbmlCallbacks KaxTagTargets::ClassInfos(KaxTagTargets::Create, KaxTagTargets_TheId, "TagTargets", KaxTagTargets_Context);
const EbmlCallbacks KaxTagGeneral::ClassInfos(KaxTagGeneral::Create, KaxTagGeneral_TheId, "TagGeneral", KaxTagGeneral_Context);
const EbmlCallbacks KaxTagGenres::ClassInfos(KaxTagGenres::Create, KaxTagGenres_TheId, "TagGenres", KaxTagGenres_Context);
const EbmlCallbacks KaxTagAudioSpecific::ClassInfos(KaxTagAudioSpecific::Create, KaxTagAudioSpecific_TheId, "TagAudioSpecific", KaxTagAudioSpecific_Context);
const EbmlCallbacks KaxTagImageSpecific::ClassInfos(KaxTagImageSpecific::Create, KaxTagImageSpecific_TheId, "TagImageSpecific", KaxTagImageSpecific_Context);
const EbmlCallbacks KaxTagBibliography::ClassInfos(KaxTagBibliography::Create, KaxTagBibliography_TheId, "Bibliography", KaxTagBibliography_Context);
const EbmlCallbacks KaxTagCaptureDPI::ClassInfos(KaxTagCaptureDPI::Create, KaxTagCaptureDPI_TheId, "CaptureDPI", KaxTagCaptureDPI_Context);
const EbmlCallbacks KaxTagCaptureLightness::ClassInfos(KaxTagCaptureLightness::Create, KaxTagCaptureLightness_TheId, "CaptureLightness", KaxTagCaptureLightness_Context);
const EbmlCallbacks KaxTagCapturePaletteSetting::ClassInfos(KaxTagCapturePaletteSetting::Create, KaxTagCapturePaletteSetting_TheId, "CapturePaletteSetting", KaxTagCapturePaletteSetting_Context);
const EbmlCallbacks KaxTagCaptureSharpness::ClassInfos(KaxTagCaptureSharpness::Create, KaxTagCaptureSharpness_TheId, "CaptureSharpness", KaxTagCaptureSharpness_Context);
const EbmlCallbacks KaxTagCropped::ClassInfos(KaxTagCropped::Create, KaxTagCropped_TheId, "Cropped", KaxTagCropped_Context);
const EbmlCallbacks KaxTagEncoder::ClassInfos(KaxTagEncoder::Create, KaxTagEncoder_TheId, "Encoder", KaxTagEncoder_Context);
const EbmlCallbacks KaxTagEncodeSettings::ClassInfos(KaxTagEncodeSettings::Create, KaxTagEncodeSettings_TheId, "EncodeSettings", KaxTagEncodeSettings_Context);
const EbmlCallbacks KaxTagLanguage::ClassInfos(KaxTagLanguage::Create, KaxTagLanguage_TheId, "Language", KaxTagLanguage_Context);
const EbmlCallbacks KaxTagLength::ClassInfos(KaxTagLength::Create, KaxTagLength_TheId, "Length", KaxTagLength_Context);
const EbmlCallbacks KaxTagOriginalDimensions::ClassInfos(KaxTagOriginalDimensions::Create, KaxTagOriginalDimensions_TheId, "OriginalDimensions", KaxTagOriginalDimensions_Context);
const EbmlCallbacks KaxTagPlaylistDelay::ClassInfos(KaxTagPlaylistDelay::Create, KaxTagPlaylistDelay_TheId, "PlaylistDelay", KaxTagPlaylistDelay_Context);
const EbmlCallbacks KaxTagRating::ClassInfos(KaxTagRating::Create, KaxTagRating_TheId, "Rating", KaxTagRating_Context);
const EbmlCallbacks KaxTagSubject::ClassInfos(KaxTagSubject::Create, KaxTagSubject_TheId, "Subject", KaxTagSubject_Context);
const EbmlCallbacks KaxTagUnsynchronisedText::ClassInfos(KaxTagUnsynchronisedText::Create, KaxTagUnsynchronisedText_TheId, "UnsynchronisedText", KaxTagUnsynchronisedText_Context);
const EbmlCallbacks KaxTagUserDefinedURL::ClassInfos(KaxTagUserDefinedURL::Create, KaxTagUserDefinedURL_TheId, "UserDefinedURL", KaxTagUserDefinedURL_Context);
const EbmlCallbacks KaxTagTargetTypeValue::ClassInfos(KaxTagTargetTypeValue::Create, KaxTagTargetTypeValue_TheId, "TagTargetTypeValue", KaxTagTargetTypeValue_Context);
const EbmlCallbacks KaxTagTargetType::ClassInfos(KaxTagTargetType::Create, KaxTagTargetType_TheId, "TagTargetType", KaxTagTargetType_Context);
const EbmlCallbacks KaxTagTrackUID::ClassInfos(KaxTagTrackUID::Create, KaxTagTrackUID_TheId, "TagTrackUID", KaxTagTrackUID_Context);
const EbmlCallbacks KaxTagEditionUID::ClassInfos(KaxTagEditionUID::Create, KaxTagEditionUID_TheId, "TagEditionUID", KaxTagEditionUID_Context);
const EbmlCallbacks KaxTagChapterUID::ClassInfos(KaxTagChapterUID::Create, KaxTagChapterUID_TheId, "TagChapterUID", KaxTagChapterUID_Context);
const EbmlCallbacks KaxTagAttachmentUID::ClassInfos(KaxTagAttachmentUID::Create, KaxTagAttachmentUID_TheId, "TagAttachmentUID", KaxTagAttachmentUID_Context);
const EbmlCallbacks KaxTagAudioGenre::ClassInfos(KaxTagAudioGenre::Create, KaxTagAudioGenre_TheId, "AudioGenre", KaxTagAudioGenre_Context);
const EbmlCallbacks KaxTagVideoGenre::ClassInfos(KaxTagVideoGenre::Create, KaxTagVideoGenre_TheId, "VideoGenre", KaxTagVideoGenre_Context);
const EbmlCallbacks KaxTagSubGenre::ClassInfos(KaxTagSubGenre::Create, KaxTagSubGenre_TheId, "SubGenre", KaxTagSubGenre_Context);
const EbmlCallbacks KaxTagAudioEncryption::ClassInfos(KaxTagAudioEncryption::Create, KaxTagAudioEncryption_TheId, "AudioEncryption", KaxTagAudioEncryption_Context);
const EbmlCallbacks KaxTagAudioGain::ClassInfos(KaxTagAudioGain::Create, KaxTagAudioGain_TheId, "AudioGain", KaxTagAudioGain_Context);
const EbmlCallbacks KaxTagAudioPeak::ClassInfos(KaxTagAudioPeak::Create, KaxTagAudioPeak_TheId, "AudioPeak", KaxTagAudioPeak_Context);
const EbmlCallbacks KaxTagBPM::ClassInfos(KaxTagBPM::Create, KaxTagBPM_TheId, "BPM", KaxTagBPM_Context);
const EbmlCallbacks KaxTagDiscTrack::ClassInfos(KaxTagDiscTrack::Create, KaxTagDiscTrack_TheId, "DiscTrack", KaxTagDiscTrack_Context);
const EbmlCallbacks KaxTagSetPart::ClassInfos(KaxTagSetPart::Create, KaxTagSetPart_TheId, "SetPart", KaxTagSetPart_Context);
const EbmlCallbacks KaxTagEqualisation::ClassInfos(KaxTagEqualisation::Create, KaxTagEqualisation_TheId, "Equalisation", KaxTagEqualisation_Context);
const EbmlCallbacks KaxTagInitialKey::ClassInfos(KaxTagInitialKey::Create, KaxTagInitialKey_TheId, "InitialKey", KaxTagInitialKey_Context);
const EbmlCallbacks KaxTagOfficialAudioFileURL::ClassInfos(KaxTagOfficialAudioFileURL::Create, KaxTagOfficialAudioFileURL_TheId, "OfficialAudioFileURL", KaxTagOfficialAudioFileURL_Context);
const EbmlCallbacks KaxTagOfficialAudioSourceURL::ClassInfos(KaxTagOfficialAudioSourceURL::Create, KaxTagOfficialAudioSourceURL_TheId, "AudioSourceURL", KaxTagOfficialAudioSourceURL_Context);
const EbmlCallbacks KaxTagArchivalLocation::ClassInfos(KaxTagArchivalLocation::Create, KaxTagArchivalLocation_TheId, "ArchivalLocation", KaxTagArchivalLocation_Context);
const EbmlCallbacks KaxTagFile::ClassInfos(KaxTagFile::Create, KaxTagFile_TheId, "File", KaxTagFile_Context);
const EbmlCallbacks KaxTagKeywords::ClassInfos(KaxTagKeywords::Create, KaxTagKeywords_TheId, "Keywords", KaxTagKeywords_Context);
const EbmlCallbacks KaxTagMood::ClassInfos(KaxTagMood::Create, KaxTagMood_TheId, "Mood", KaxTagMood_Context);
const EbmlCallbacks KaxTagRecordLocation::ClassInfos(KaxTagRecordLocation::Create, KaxTagRecordLocation_TheId, "RecordLocation", KaxTagRecordLocation_Context);
const EbmlCallbacks KaxTagSource::ClassInfos(KaxTagSource::Create, KaxTagSource_TheId, "Source", KaxTagSource_Context);
const EbmlCallbacks KaxTagSourceForm::ClassInfos(KaxTagSourceForm::Create, KaxTagSourceForm_TheId, "SourceForm", KaxTagSourceForm_Context);
const EbmlCallbacks KaxTagProduct::ClassInfos(KaxTagProduct::Create, KaxTagProduct_TheId, "Product", KaxTagProduct_Context);
const EbmlCallbacks KaxTagOriginalMediaType::ClassInfos(KaxTagOriginalMediaType::Create, KaxTagOriginalMediaType_TheId, "OriginalMediaType", KaxTagOriginalMediaType_Context);
const EbmlCallbacks KaxTagPlayCounter::ClassInfos(KaxTagPlayCounter::Create, KaxTagPlayCounter_TheId, "PlayCounter", KaxTagPlayCounter_Context);
const EbmlCallbacks KaxTagPopularimeter::ClassInfos(KaxTagPopularimeter::Create, KaxTagPopularimeter_TheId, "Popularimeter", KaxTagPopularimeter_Context);

const EbmlCallbacks KaxTagSimple::ClassInfos(KaxTagSimple::Create, KaxTagSimple_TheId, "TagSimple", KaxTagSimple_Context);
const EbmlCallbacks KaxTagName::ClassInfos(KaxTagName::Create, KaxTagName_TheId, "TagName", KaxTagName_Context);
const EbmlCallbacks KaxTagLangue::ClassInfos(KaxTagLangue::Create, KaxTagLangue_TheId, "TagLanguage", KaxTagLangue_Context);
const EbmlCallbacks KaxTagDefault::ClassInfos(KaxTagDefault::Create, KaxTagDefault_TheId, "TagDefault", KaxTagDefault_Context);
const EbmlCallbacks KaxTagString::ClassInfos(KaxTagString::Create, KaxTagString_TheId, "TagString", KaxTagString_Context);
const EbmlCallbacks KaxTagBinary::ClassInfos(KaxTagBinary::Create, KaxTagBinary_TheId, "TagBinary", KaxTagBinary_Context);

KaxTag::KaxTag()
	:EbmlMaster(KaxTag_Context)
{}

KaxTagTargets::KaxTagTargets()
	:EbmlMaster(KaxTagTargets_Context)
{}

KaxTagGeneral::KaxTagGeneral()
	:EbmlMaster(KaxTagGeneral_Context)
{}

KaxTagGenres::KaxTagGenres()
	:EbmlMaster(KaxTagGenres_Context)
{}

KaxTagAudioSpecific::KaxTagAudioSpecific()
	:EbmlMaster(KaxTagAudioSpecific_Context)
{}

KaxTagImageSpecific::KaxTagImageSpecific()
	:EbmlMaster(KaxTagImageSpecific_Context)
{}

KaxTagSimple::KaxTagSimple()
	:EbmlMaster(KaxTagSimple_Context)
{}

END_LIBMATROSKA_NAMESPACE

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
	\version \$Id: KaxTracks.cpp 1202 2005-08-30 14:39:01Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxTracks.h"

// sub elements
#include "matroska/KaxTrackEntryData.h"
#include "matroska/KaxTrackAudio.h"
#include "matroska/KaxTrackVideo.h"
#include "matroska/KaxContentEncoding.h"
#include "matroska/KaxContexts.h"

START_LIBMATROSKA_NAMESPACE

const EbmlSemantic KaxTracks_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTrackEntry::ClassInfos),
};

#if MATROSKA_VERSION == 1
const EbmlSemantic KaxTrackEntry_ContextList[22] =
#else // MATROSKA_VERSION
const EbmlSemantic KaxTrackEntry_ContextList[27] =
#endif // MATROSKA_VERSION
{
	EbmlSemantic(true , true, KaxTrackNumber::ClassInfos),
	EbmlSemantic(true , true, KaxTrackUID::ClassInfos),
	EbmlSemantic(true , true, KaxTrackType::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(true , true, KaxTrackFlagEnabled::ClassInfos),
#endif // MATROSKA_VERSION
	EbmlSemantic(true , true, KaxTrackFlagDefault::ClassInfos),
	EbmlSemantic(true , true, KaxTrackFlagForced::ClassInfos),
	EbmlSemantic(true , true, KaxTrackFlagLacing::ClassInfos),
	EbmlSemantic(true , true, KaxTrackMinCache::ClassInfos),
	EbmlSemantic(false, true, KaxTrackMaxCache::ClassInfos),
	EbmlSemantic(false, true, KaxTrackDefaultDuration::ClassInfos),
	EbmlSemantic(true , true, KaxTrackTimecodeScale::ClassInfos),
	EbmlSemantic(true , true, KaxMaxBlockAdditionID::ClassInfos),
	EbmlSemantic(false, true, KaxTrackName::ClassInfos),
	EbmlSemantic(false, true, KaxTrackLanguage::ClassInfos),
	EbmlSemantic(true , true, KaxCodecID::ClassInfos),
	EbmlSemantic(false, true, KaxCodecPrivate::ClassInfos),
	EbmlSemantic(false, true, KaxCodecName::ClassInfos),
	EbmlSemantic(false, true, KaxTrackAttachmentLink::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(false, true, KaxCodecSettings::ClassInfos),
	EbmlSemantic(false, false,KaxCodecInfoURL::ClassInfos),
	EbmlSemantic(false, false,KaxCodecDownloadURL::ClassInfos),
	EbmlSemantic(true , true, KaxCodecDecodeAll::ClassInfos),
#endif // MATROSKA_VERSION
	EbmlSemantic(false, false,KaxTrackOverlay::ClassInfos),
	EbmlSemantic(false, false,KaxTrackTranslate::ClassInfos),
	EbmlSemantic(false, true, KaxTrackAudio::ClassInfos),
	EbmlSemantic(false, true, KaxTrackVideo::ClassInfos),
	EbmlSemantic(false, true, KaxContentEncodings::ClassInfos),
};

const EbmlSemanticContext KaxTracks_Context = EbmlSemanticContext(countof(KaxTracks_ContextList), KaxTracks_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxTracks::ClassInfos);
const EbmlSemanticContext KaxTrackEntry_Context = EbmlSemanticContext(countof(KaxTrackEntry_ContextList), KaxTrackEntry_ContextList, &KaxTracks_Context, *GetKaxGlobal_Context, &KaxTrackEntry::ClassInfos);

EbmlId KaxTracks_TheId    (0x1654AE6B, 4);
EbmlId KaxTrackEntry_TheId(0xAE, 1);

const EbmlCallbacks KaxTracks::ClassInfos(KaxTracks::Create, KaxTracks_TheId, "Tracks", KaxTracks_Context);
const EbmlCallbacks KaxTrackEntry::ClassInfos(KaxTrackEntry::Create, KaxTrackEntry_TheId, "TrackEntry", KaxTrackEntry_Context);

KaxTracks::KaxTracks()
	:EbmlMaster(KaxTracks_Context)
{}

KaxTrackEntry::KaxTrackEntry()
	:EbmlMaster(KaxTrackEntry_Context)
	,bGlobalTimecodeScaleIsSet(false)
{}

void KaxTrackEntry::EnableLacing(bool bEnable)
{
	KaxTrackFlagLacing & myLacing = GetChild<KaxTrackFlagLacing>(*this);
	*(static_cast<EbmlUInteger *>(&myLacing)) = bEnable ? 1 : 0;
}

END_LIBMATROSKA_NAMESPACE

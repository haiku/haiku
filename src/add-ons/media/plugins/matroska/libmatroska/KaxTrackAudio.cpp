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
	\version \$Id: KaxTrackAudio.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxTrackAudio.h"

// sub elements
#include "matroska/KaxContexts.h"

START_LIBMATROSKA_NAMESPACE

#if MATROSKA_VERSION == 1
const EbmlSemantic KaxTrackAudio_ContextList[4] =
#else // MATROSKA_VERSION
const EbmlSemantic KaxTrackAudio_ContextList[5] =
#endif // MATROSKA_VERSION
{
	EbmlSemantic(true , true, KaxAudioSamplingFreq::ClassInfos),
	EbmlSemantic(true , true, KaxAudioChannels::ClassInfos),
	EbmlSemantic(false, true, KaxAudioBitDepth::ClassInfos),
	EbmlSemantic(false, true, KaxAudioOutputSamplingFreq::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(false, true, KaxAudioPosition::ClassInfos),
#endif // MATROSKA_VERSION
};

const EbmlSemanticContext KaxTrackAudio_Context = EbmlSemanticContext(countof(KaxTrackAudio_ContextList), KaxTrackAudio_ContextList, &KaxTrackEntry_Context, *GetKaxGlobal_Context, &KaxTrackAudio::ClassInfos);
const EbmlSemanticContext KaxAudioSamplingFreq_Context = EbmlSemanticContext(0, NULL, &KaxTrackAudio_Context, *GetKaxGlobal_Context, &KaxAudioSamplingFreq::ClassInfos);
const EbmlSemanticContext KaxAudioOutputSamplingFreq_Context = EbmlSemanticContext(0, NULL, &KaxTrackAudio_Context, *GetKaxGlobal_Context, &KaxAudioOutputSamplingFreq::ClassInfos);
const EbmlSemanticContext KaxAudioChannels_Context = EbmlSemanticContext(0, NULL, &KaxTrackAudio_Context, *GetKaxGlobal_Context, &KaxAudioChannels::ClassInfos);
const EbmlSemanticContext KaxAudioBitDepth_Context = EbmlSemanticContext(0, NULL, &KaxTrackAudio_Context, *GetKaxGlobal_Context, &KaxAudioBitDepth::ClassInfos);
#if MATROSKA_VERSION >= 2
const EbmlSemanticContext KaxAudioPosition_Context = EbmlSemanticContext(0, NULL, &KaxTrackAudio_Context, *GetKaxGlobal_Context, &KaxAudioPosition::ClassInfos);
#endif // MATROSKA_VERSION

EbmlId KaxTrackAudio_TheId       (0xE1, 1);
EbmlId KaxAudioSamplingFreq_TheId(0xB5, 1);
EbmlId KaxAudioOutputSamplingFreq_TheId(0x78B5, 2);
EbmlId KaxAudioChannels_TheId    (0x9F, 1);
EbmlId KaxAudioBitDepth_TheId    (0x6264, 2);
#if MATROSKA_VERSION >= 2
EbmlId KaxAudioPosition_TheId    (0x7D7B, 2);
#endif // MATROSKA_VERSION

const EbmlCallbacks KaxTrackAudio::ClassInfos(KaxTrackAudio::Create, KaxTrackAudio_TheId, "TrackAudio", KaxTrackAudio_Context);
const EbmlCallbacks KaxAudioSamplingFreq::ClassInfos(KaxAudioSamplingFreq::Create, KaxAudioSamplingFreq_TheId, "AudioSamplingFreq", KaxAudioSamplingFreq_Context);
const EbmlCallbacks KaxAudioOutputSamplingFreq::ClassInfos(KaxAudioOutputSamplingFreq::Create, KaxAudioOutputSamplingFreq_TheId, "AudioOutputSamplingFreq", KaxAudioOutputSamplingFreq_Context);
const EbmlCallbacks KaxAudioChannels::ClassInfos(KaxAudioChannels::Create, KaxAudioChannels_TheId, "AudioChannels", KaxAudioChannels_Context);
const EbmlCallbacks KaxAudioBitDepth::ClassInfos(KaxAudioBitDepth::Create, KaxAudioBitDepth_TheId, "AudioBitDepth", KaxAudioBitDepth_Context);
#if MATROSKA_VERSION >= 2
const EbmlCallbacks KaxAudioPosition::ClassInfos(KaxAudioPosition::Create, KaxAudioPosition_TheId, "AudioPosition", KaxAudioPosition_Context);
#endif // MATROSKA_VERSION

KaxTrackAudio::KaxTrackAudio()
	:EbmlMaster(KaxTrackAudio_Context)
{}

END_LIBMATROSKA_NAMESPACE

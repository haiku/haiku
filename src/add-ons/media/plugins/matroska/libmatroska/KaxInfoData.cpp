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
	\version \$Id: KaxInfoData.cpp 1078 2005-03-03 13:13:04Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author John Cannon      <spyder2555 @ users.sf.net>
*/
#include "matroska/KaxInfoData.h"
#include "matroska/KaxContexts.h"

START_LIBMATROSKA_NAMESPACE

const EbmlSemantic KaxChapterTranslate_ContextList[3] =
{
	EbmlSemantic(false, false, KaxChapterTranslateEditionUID::ClassInfos),
	EbmlSemantic(true,  true,  KaxChapterTranslateCodec::ClassInfos),
	EbmlSemantic(true,  true,  KaxChapterTranslateID::ClassInfos),
};

EbmlId KaxSegmentUID_TheId      (0x73A4, 2);
EbmlId KaxSegmentFilename_TheId (0x7384, 2);
EbmlId KaxPrevUID_TheId         (0x3CB923, 3);
EbmlId KaxPrevFilename_TheId    (0x3C83AB, 3);
EbmlId KaxNextUID_TheId         (0x3EB923, 3);
EbmlId KaxNextFilename_TheId    (0x3E83BB, 3);
EbmlId KaxSegmentFamily_TheId   (0x4444, 2);
EbmlId KaxChapterTranslate_TheId(0x6924, 2);
EbmlId KaxChapterTranslateEditionUID_TheId(0x69FC, 2);
EbmlId KaxChapterTranslateCodec_TheId(0x69BF, 2);
EbmlId KaxChapterTranslateID_TheId(0x69A5, 2);
EbmlId KaxTimecodeScale_TheId   (0x2AD7B1, 3);
EbmlId KaxDuration_TheId        (0x4489, 2);
EbmlId KaxDateUTC_TheId         (0x4461, 2);
EbmlId KaxTitle_TheId           (0x7BA9, 2);

const EbmlSemanticContext KaxSegmentUID_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxSegmentUID::ClassInfos);
const EbmlSemanticContext KaxSegmentFilename_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxSegmentFilename::ClassInfos);
const EbmlSemanticContext KaxPrevUID_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxPrevUID::ClassInfos);
const EbmlSemanticContext KaxPrevFilename_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxPrevFilename::ClassInfos);
const EbmlSemanticContext KaxNextUID_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxNextUID::ClassInfos);
const EbmlSemanticContext KaxNextFilename_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxNextFilename::ClassInfos);
const EbmlSemanticContext KaxSegmentFamily_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxSegmentFamily::ClassInfos);
const EbmlSemanticContext KaxChapterTranslate_Context = EbmlSemanticContext(countof(KaxChapterTranslate_ContextList), KaxChapterTranslate_ContextList, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxChapterTranslate::ClassInfos);
const EbmlSemanticContext KaxChapterTranslateEditionUID_Context = EbmlSemanticContext(0, NULL, &KaxChapterTranslate_Context, *GetKaxGlobal_Context, &KaxChapterTranslateEditionUID::ClassInfos);
const EbmlSemanticContext KaxChapterTranslateCodec_Context = EbmlSemanticContext(0, NULL, &KaxChapterTranslate_Context, *GetKaxGlobal_Context, &KaxChapterTranslateCodec::ClassInfos);
const EbmlSemanticContext KaxChapterTranslateID_Context = EbmlSemanticContext(0, NULL, &KaxChapterTranslate_Context, *GetKaxGlobal_Context, &KaxChapterTranslateID::ClassInfos);
const EbmlSemanticContext KaxTimecodeScale_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxTimecodeScale::ClassInfos);
const EbmlSemanticContext KaxDuration_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxDuration::ClassInfos);
const EbmlSemanticContext KaxDateUTC_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxDateUTC::ClassInfos);
const EbmlSemanticContext KaxTitle_Context = EbmlSemanticContext(0, NULL, &KaxInfo_Context, *GetKaxGlobal_Context, &KaxTitle::ClassInfos);


const EbmlCallbacks KaxSegmentUID::ClassInfos(KaxSegmentUID::Create, KaxSegmentUID_TheId, "SegmentUID", KaxSegmentUID_Context);
const EbmlCallbacks KaxSegmentFilename::ClassInfos(KaxSegmentFilename::Create, KaxSegmentFilename_TheId, "SegmentFilename", KaxSegmentFilename_Context);
const EbmlCallbacks KaxPrevUID::ClassInfos(KaxPrevUID::Create, KaxPrevUID_TheId, "PrevUID", KaxPrevUID_Context);
const EbmlCallbacks KaxPrevFilename::ClassInfos(KaxPrevFilename::Create, KaxPrevFilename_TheId, "PrevFilename", KaxPrevFilename_Context);
const EbmlCallbacks KaxNextUID::ClassInfos(KaxNextUID::Create, KaxNextUID_TheId, "NextUID", KaxNextUID_Context);
const EbmlCallbacks KaxNextFilename::ClassInfos(KaxNextFilename::Create, KaxNextFilename_TheId, "NextFilename", KaxNextFilename_Context);
const EbmlCallbacks KaxSegmentFamily::ClassInfos(KaxSegmentFamily::Create, KaxSegmentFamily_TheId, "SegmentFamily", KaxSegmentFamily_Context);
const EbmlCallbacks KaxChapterTranslate::ClassInfos(KaxChapterTranslate::Create, KaxChapterTranslate_TheId, "ChapterTranslate", KaxChapterTranslate_Context);
const EbmlCallbacks KaxChapterTranslateEditionUID::ClassInfos(KaxChapterTranslateEditionUID::Create, KaxChapterTranslateEditionUID_TheId, "ChapterTranslateEditionUID", KaxChapterTranslateEditionUID_Context);
const EbmlCallbacks KaxChapterTranslateCodec::ClassInfos(KaxChapterTranslateCodec::Create, KaxChapterTranslateCodec_TheId, "ChapterTranslateCodec", KaxChapterTranslateCodec_Context);
const EbmlCallbacks KaxChapterTranslateID::ClassInfos(KaxChapterTranslateID::Create, KaxChapterTranslateID_TheId, "ChapterTranslateID", KaxChapterTranslateID_Context);
const EbmlCallbacks KaxTimecodeScale::ClassInfos(KaxTimecodeScale::Create, KaxTimecodeScale_TheId, "TimecodeScale", KaxTimecodeScale_Context);
const EbmlCallbacks KaxDuration::ClassInfos(KaxDuration::Create, KaxDuration_TheId, "Duration", KaxDuration_Context);
const EbmlCallbacks KaxDateUTC::ClassInfos(KaxDateUTC::Create, KaxDateUTC_TheId, "DateUTC", KaxDateUTC_Context);
const EbmlCallbacks KaxTitle::ClassInfos(KaxTitle::Create, KaxTitle_TheId, "Title", KaxTitle_Context);

KaxChapterTranslate::KaxChapterTranslate()
	:EbmlMaster(KaxChapterTranslate_Context)
{}

END_LIBMATROSKA_NAMESPACE

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
	\version \$Id: KaxChapters.cpp 1201 2005-08-30 14:28:27Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxChapters.h"
#include "matroska/KaxContexts.h"

// sub elements
START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxChapters_ContextList[1] =
{
    EbmlSemantic(true, false,  KaxEditionEntry::ClassInfos),
};

EbmlSemantic KaxEditionEntry_ContextList[5] =
{
    EbmlSemantic(false, true , KaxEditionUID::ClassInfos),
    EbmlSemantic(true , true , KaxEditionFlagHidden::ClassInfos),
    EbmlSemantic(true , true , KaxEditionFlagDefault::ClassInfos),
    EbmlSemantic(false, true , KaxEditionFlagOrdered::ClassInfos),
    EbmlSemantic(true , false, KaxChapterAtom::ClassInfos),
};

EbmlSemantic KaxChapterAtom_ContextList[12] =
{
    EbmlSemantic(false, false, KaxChapterAtom::ClassInfos),
    EbmlSemantic(true,  true,  KaxChapterUID::ClassInfos),
    EbmlSemantic(true,  true,  KaxChapterTimeStart::ClassInfos),
    EbmlSemantic(false, true,  KaxChapterTimeEnd::ClassInfos),
    EbmlSemantic(true , true,  KaxChapterFlagHidden::ClassInfos),
    EbmlSemantic(true , true,  KaxChapterFlagEnabled::ClassInfos),
    EbmlSemantic(false, true,  KaxChapterSegmentUID::ClassInfos),
    EbmlSemantic(false, true,  KaxChapterSegmentEditionUID::ClassInfos),
    EbmlSemantic(false, true,  KaxChapterPhysicalEquiv::ClassInfos),
    EbmlSemantic(false, true,  KaxChapterTrack::ClassInfos),
    EbmlSemantic(false, false, KaxChapterDisplay::ClassInfos),
    EbmlSemantic(false, false, KaxChapterProcess::ClassInfos),
};

EbmlSemantic KaxChapterTrack_ContextList[1] =
{
    EbmlSemantic(true, false, KaxChapterTrackNumber::ClassInfos),
};

EbmlSemantic KaxChapterDisplay_ContextList[3] =
{
    EbmlSemantic(true,  true,  KaxChapterString::ClassInfos),
    EbmlSemantic(true,  false, KaxChapterLanguage::ClassInfos),
    EbmlSemantic(false, false, KaxChapterCountry::ClassInfos),
};

EbmlSemantic KaxChapterProcess_ContextList[3] =
{
    EbmlSemantic(true,  true,  KaxChapterProcessCodecID::ClassInfos),
    EbmlSemantic(false, true,  KaxChapterProcessPrivate::ClassInfos),
    EbmlSemantic(false, false, KaxChapterProcessCommand::ClassInfos),
};

EbmlSemantic KaxChapterProcessCommand_ContextList[2] =
{
    EbmlSemantic(true,  true,  KaxChapterProcessTime::ClassInfos),
    EbmlSemantic(true,  true,  KaxChapterProcessData::ClassInfos),
};

const EbmlSemanticContext KaxChapters_Context = EbmlSemanticContext(countof(KaxChapters_ContextList), KaxChapters_ContextList, &KaxSegment_Context, *GetKaxGlobal_Context, &KaxChapters::ClassInfos);
const EbmlSemanticContext KaxEditionEntry_Context = EbmlSemanticContext(countof(KaxEditionEntry_ContextList), KaxEditionEntry_ContextList, &KaxChapters_Context, *GetKaxGlobal_Context, &KaxEditionEntry::ClassInfos);
const EbmlSemanticContext KaxEditionUID_Context = EbmlSemanticContext(0, NULL, &KaxEditionEntry_Context, *GetKaxGlobal_Context, &KaxEditionUID::ClassInfos);
const EbmlSemanticContext KaxEditionFlagHidden_Context = EbmlSemanticContext(0, NULL, &KaxEditionEntry_Context, *GetKaxGlobal_Context, &KaxEditionFlagHidden::ClassInfos);
const EbmlSemanticContext KaxEditionFlagDefault_Context = EbmlSemanticContext(0, NULL, &KaxEditionEntry_Context, *GetKaxGlobal_Context, &KaxEditionFlagDefault::ClassInfos);
const EbmlSemanticContext KaxEditionFlagOrdered_Context = EbmlSemanticContext(0, NULL, &KaxEditionEntry_Context, *GetKaxGlobal_Context, &KaxEditionFlagOrdered::ClassInfos);
const EbmlSemanticContext KaxChapterAtom_Context = EbmlSemanticContext(countof(KaxChapterAtom_ContextList), KaxChapterAtom_ContextList, &KaxEditionEntry_Context, *GetKaxGlobal_Context, &KaxChapterAtom::ClassInfos);
const EbmlSemanticContext KaxChapterTrack_Context = EbmlSemanticContext(countof(KaxChapterTrack_ContextList), KaxChapterTrack_ContextList, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterTrack::ClassInfos);
const EbmlSemanticContext KaxChapterDisplay_Context = EbmlSemanticContext(countof(KaxChapterDisplay_ContextList), KaxChapterDisplay_ContextList, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterDisplay::ClassInfos);
const EbmlSemanticContext KaxChapterUID_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterUID::ClassInfos);
const EbmlSemanticContext KaxChapterTimeStart_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterTimeStart::ClassInfos);
const EbmlSemanticContext KaxChapterTimeEnd_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterTimeEnd::ClassInfos);
const EbmlSemanticContext KaxChapterFlagHidden_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterFlagHidden::ClassInfos);
const EbmlSemanticContext KaxChapterFlagEnabled_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterFlagEnabled::ClassInfos);
const EbmlSemanticContext KaxChapterSegmentUID_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterSegmentUID::ClassInfos);
const EbmlSemanticContext KaxChapterSegmentEditionUID_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterSegmentEditionUID::ClassInfos);
const EbmlSemanticContext KaxChapterPhysicalEquiv_Context = EbmlSemanticContext(0, NULL, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterPhysicalEquiv::ClassInfos);
const EbmlSemanticContext KaxChapterTrackNumber_Context = EbmlSemanticContext(0, NULL, &KaxChapterTrack_Context, *GetKaxGlobal_Context, &KaxChapterTrackNumber::ClassInfos);
const EbmlSemanticContext KaxChapterString_Context = EbmlSemanticContext(0, NULL, &KaxChapterDisplay_Context, *GetKaxGlobal_Context, &KaxChapterString::ClassInfos);
const EbmlSemanticContext KaxChapterLanguage_Context = EbmlSemanticContext(0, NULL, &KaxChapterLanguage_Context, *GetKaxGlobal_Context, &KaxChapterLanguage::ClassInfos);
const EbmlSemanticContext KaxChapterCountry_Context = EbmlSemanticContext(0, NULL, &KaxChapterCountry_Context, *GetKaxGlobal_Context, &KaxChapterCountry::ClassInfos);
const EbmlSemanticContext KaxChapterProcess_Context = EbmlSemanticContext(countof(KaxChapterProcess_ContextList), KaxChapterProcess_ContextList, &KaxChapterAtom_Context, *GetKaxGlobal_Context, &KaxChapterProcess::ClassInfos);
const EbmlSemanticContext KaxChapterProcessCodecID_Context = EbmlSemanticContext(0, NULL, &KaxChapterProcess_Context, *GetKaxGlobal_Context, &KaxChapterProcessCodecID::ClassInfos);
const EbmlSemanticContext KaxChapterProcessPrivate_Context = EbmlSemanticContext(0, NULL, &KaxChapterProcess_Context, *GetKaxGlobal_Context, &KaxChapterProcessPrivate::ClassInfos);
const EbmlSemanticContext KaxChapterProcessCommand_Context = EbmlSemanticContext(countof(KaxChapterProcessCommand_ContextList), KaxChapterProcessCommand_ContextList, &KaxChapterProcess_Context, *GetKaxGlobal_Context, &KaxChapterProcessCommand::ClassInfos);
const EbmlSemanticContext KaxChapterProcessTime_Context = EbmlSemanticContext(0, NULL, &KaxChapterProcessCommand_Context, *GetKaxGlobal_Context, &KaxChapterProcessTime::ClassInfos);
const EbmlSemanticContext KaxChapterProcessData_Context = EbmlSemanticContext(0, NULL, &KaxChapterProcessCommand_Context, *GetKaxGlobal_Context, &KaxChapterProcessData::ClassInfos);

EbmlId KaxChapters_TheId               (0x1043A770, 4);
EbmlId KaxEditionEntry_TheId           (0x45B9, 2);
EbmlId KaxEditionUID_TheId             (0x45BC, 2);
EbmlId KaxEditionFlagHidden_TheId      (0x45BD, 2);
EbmlId KaxEditionFlagDefault_TheId     (0x45DB, 2);
EbmlId KaxEditionFlagOrdered_TheId     (0x45DD, 2);
EbmlId KaxChapterAtom_TheId            (0xB6, 1);
EbmlId KaxChapterUID_TheId             (0x73C4, 2);
EbmlId KaxChapterTimeStart_TheId       (0x91, 1);
EbmlId KaxChapterTimeEnd_TheId         (0x92, 1);
EbmlId KaxChapterFlagHidden_TheId      (0x98, 1);
EbmlId KaxChapterFlagEnabled_TheId     (0x4598, 2);
EbmlId KaxChapterSegmentUID_TheId      (0x6E67, 2);
EbmlId KaxChapterSegmentEditionUID_TheId(0x6EBC, 2);
EbmlId KaxChapterPhysicalEquiv_TheId   (0x63C3, 2);
EbmlId KaxChapterTrack_TheId           (0x8F, 1);
EbmlId KaxChapterTrackNumber_TheId     (0x89, 1);
EbmlId KaxChapterDisplay_TheId         (0x80, 1);
EbmlId KaxChapterString_TheId          (0x85, 1);
EbmlId KaxChapterLanguage_TheId        (0x437C, 2);
EbmlId KaxChapterCountry_TheId         (0x437E, 2);
EbmlId KaxChapterProcess_TheId         (0x6944, 2);
EbmlId KaxChapterProcessCodecID_TheId  (0x6955, 2);
EbmlId KaxChapterProcessPrivate_TheId  (0x450D, 2);
EbmlId KaxChapterProcessCommand_TheId  (0x6911, 2);
EbmlId KaxChapterProcessTime_TheId     (0x6922, 2);
EbmlId KaxChapterProcessData_TheId     (0x6933, 2);

const EbmlCallbacks KaxChapters::ClassInfos(KaxChapters::Create, KaxChapters_TheId, "Chapters", KaxChapters_Context);
const EbmlCallbacks KaxEditionEntry::ClassInfos(KaxEditionEntry::Create, KaxEditionEntry_TheId, "EditionEntry", KaxEditionEntry_Context);
const EbmlCallbacks KaxEditionUID::ClassInfos(KaxEditionUID::Create, KaxEditionUID_TheId, "EditionUID", KaxEditionUID_Context);
const EbmlCallbacks KaxEditionFlagHidden::ClassInfos(KaxEditionFlagHidden::Create, KaxEditionFlagHidden_TheId, "EditionFlagHidden", KaxEditionFlagHidden_Context);
const EbmlCallbacks KaxEditionFlagDefault::ClassInfos(KaxEditionFlagDefault::Create, KaxEditionFlagDefault_TheId, "EditionFlagDefault", KaxEditionFlagDefault_Context);
const EbmlCallbacks KaxEditionFlagOrdered::ClassInfos(KaxEditionFlagOrdered::Create, KaxEditionFlagOrdered_TheId, "EditionFlagOrdered", KaxEditionFlagOrdered_Context);
const EbmlCallbacks KaxChapterAtom::ClassInfos(KaxChapterAtom::Create, KaxChapterAtom_TheId, "ChapterAtom", KaxChapterAtom_Context);
const EbmlCallbacks KaxChapterUID::ClassInfos(KaxChapterUID::Create, KaxChapterUID_TheId, "ChapterUID", KaxChapterUID_Context);
const EbmlCallbacks KaxChapterTimeStart::ClassInfos(KaxChapterTimeStart::Create, KaxChapterTimeStart_TheId, "ChapterTimeStart", KaxChapterTimeStart_Context);
const EbmlCallbacks KaxChapterTimeEnd::ClassInfos(KaxChapterTimeEnd::Create, KaxChapterTimeEnd_TheId, "ChapterTimeEnd", KaxChapterTimeEnd_Context);
const EbmlCallbacks KaxChapterFlagHidden::ClassInfos(KaxChapterFlagHidden::Create, KaxChapterFlagHidden_TheId, "ChapterFlagHidden", KaxChapterFlagHidden_Context);
const EbmlCallbacks KaxChapterFlagEnabled::ClassInfos(KaxChapterFlagEnabled::Create, KaxChapterFlagEnabled_TheId, "ChapterFlagEnabled", KaxChapterFlagEnabled_Context);
const EbmlCallbacks KaxChapterSegmentUID::ClassInfos(KaxChapterSegmentUID::Create, KaxChapterSegmentUID_TheId, "ChapterSegmentUID", KaxChapterSegmentUID_Context);
const EbmlCallbacks KaxChapterSegmentEditionUID::ClassInfos(KaxChapterSegmentEditionUID::Create, KaxChapterSegmentEditionUID_TheId, "ChapterSegmentEditionUID", KaxChapterSegmentEditionUID_Context);
const EbmlCallbacks KaxChapterPhysicalEquiv::ClassInfos(KaxChapterPhysicalEquiv::Create, KaxChapterPhysicalEquiv_TheId, "ChapterPhysicalEquiv", KaxChapterPhysicalEquiv_Context);
const EbmlCallbacks KaxChapterTrack::ClassInfos(KaxChapterTrack::Create, KaxChapterTrack_TheId, "ChapterTrack", KaxChapterTrack_Context);
const EbmlCallbacks KaxChapterTrackNumber::ClassInfos(KaxChapterTrackNumber::Create, KaxChapterTrackNumber_TheId, "ChapterTrackNumber", KaxChapterTrackNumber_Context);
const EbmlCallbacks KaxChapterDisplay::ClassInfos(KaxChapterDisplay::Create, KaxChapterDisplay_TheId, "ChapterDisplay", KaxChapterDisplay_Context);
const EbmlCallbacks KaxChapterString::ClassInfos(KaxChapterString::Create, KaxChapterString_TheId, "ChapterString", KaxChapterString_Context);
const EbmlCallbacks KaxChapterLanguage::ClassInfos(KaxChapterLanguage::Create, KaxChapterLanguage_TheId, "ChapterLanguage", KaxChapterLanguage_Context);
const EbmlCallbacks KaxChapterCountry::ClassInfos(KaxChapterCountry::Create, KaxChapterCountry_TheId, "ChapterCountry", KaxChapterCountry_Context);
const EbmlCallbacks KaxChapterProcess::ClassInfos(KaxChapterProcess::Create, KaxChapterProcess_TheId, "ChapterProcess", KaxChapterProcess_Context);
const EbmlCallbacks KaxChapterProcessCodecID::ClassInfos(KaxChapterProcessCodecID::Create, KaxChapterProcessCodecID_TheId, "ChapterProcessCodecID", KaxChapterProcessCodecID_Context);
const EbmlCallbacks KaxChapterProcessPrivate::ClassInfos(KaxChapterProcessPrivate::Create, KaxChapterProcessPrivate_TheId, "ChapterProcessPrivate", KaxChapterProcessPrivate_Context);
const EbmlCallbacks KaxChapterProcessCommand::ClassInfos(KaxChapterProcessCommand::Create, KaxChapterProcessCommand_TheId, "ChapterProcessCommand", KaxChapterProcessCommand_Context);
const EbmlCallbacks KaxChapterProcessTime::ClassInfos(KaxChapterProcessTime::Create, KaxChapterProcessTime_TheId, "ChapterProcessTime", KaxChapterProcessTime_Context);
const EbmlCallbacks KaxChapterProcessData::ClassInfos(KaxChapterProcessData::Create, KaxChapterProcessData_TheId, "ChapterProcessData", KaxChapterProcessData_Context);

KaxChapters::KaxChapters()
 :EbmlMaster(KaxChapters_Context)
{}

KaxEditionEntry::KaxEditionEntry()
:EbmlMaster(KaxEditionEntry_Context)
{}

KaxChapterAtom::KaxChapterAtom()
:EbmlMaster(KaxChapterAtom_Context)
{}

KaxChapterTrack::KaxChapterTrack()
:EbmlMaster(KaxChapterTrack_Context)
{}

KaxChapterDisplay::KaxChapterDisplay()
:EbmlMaster(KaxChapterDisplay_Context)
{}

KaxChapterProcess::KaxChapterProcess()
:EbmlMaster(KaxChapterProcess_Context)
{}

KaxChapterProcessCommand::KaxChapterProcessCommand()
:EbmlMaster(KaxChapterProcessCommand_Context)
{}

END_LIBMATROSKA_NAMESPACE

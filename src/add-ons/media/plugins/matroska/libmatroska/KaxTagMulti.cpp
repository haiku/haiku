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
	\version \$Id: KaxTagMulti.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Jory Stone       <jcsston @ toughguy.net>
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxTagMulti.h"
#include "matroska/KaxContexts.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

EbmlSemantic KaxTagMultiGlobal_ContextList[1] =
{
	EbmlSemantic(false, false, KaxTagMultiComment::ClassInfos),
};

EbmlSemantic KaxTagMultiComment_ContextList[3] =
{
	EbmlSemantic(false, true, KaxTagMultiCommentName::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiCommentComments::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiCommentLanguage::ClassInfos),
};

EbmlSemantic KaxTagMultiCommercial_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagCommercial::ClassInfos),
};

EbmlSemantic KaxTagCommercial_ContextList[5] =
{
	EbmlSemantic(true,  true,  KaxTagMultiCommercialType::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiCommercialAddress::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiCommercialURL::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiCommercialEmail::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiPrice::ClassInfos),
};

EbmlSemantic KaxTagMultiPrice_ContextList[3] =
{
	EbmlSemantic(false, true, KaxTagMultiPriceCurrency::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiPriceAmount::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiPricePriceDate::ClassInfos),
};

EbmlSemantic KaxTagMultiDate_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagDate::ClassInfos),
};

EbmlSemantic KaxTagDate_ContextList[3] =
{
	EbmlSemantic(true,  true, KaxTagMultiDateType::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiDateDateBegin::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiDateDateEnd::ClassInfos),
};

EbmlSemantic KaxTagMultiEntity_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagEntity::ClassInfos),
};

EbmlSemantic KaxTagEntity_ContextList[5] =
{
	EbmlSemantic(true,  true,  KaxTagMultiEntityType::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiEntityName::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiEntityURL::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiEntityEmail::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiEntityAddress::ClassInfos),
};

EbmlSemantic KaxTagMultiIdentifier_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagIdentifier::ClassInfos),
};

EbmlSemantic KaxTagIdentifier_ContextList[3] =
{
	EbmlSemantic(true,  true, KaxTagMultiIdentifierType::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiIdentifierBinary::ClassInfos),
	EbmlSemantic(false, true, KaxTagMultiIdentifierString::ClassInfos),
};

EbmlSemantic KaxTagMultiLegal_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagLegal::ClassInfos),
};

EbmlSemantic KaxTagLegal_ContextList[4] =
{
	EbmlSemantic(true,  true,  KaxTagMultiLegalType::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiLegalContent::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiLegalURL::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiLegalAddress::ClassInfos),
};

EbmlSemantic KaxTagMultiTitle_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagTitle::ClassInfos),
};

EbmlSemantic KaxTagTitle_ContextList[8] =
{
	EbmlSemantic(true,  true,  KaxTagMultiTitleType::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiTitleName::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiTitleSubTitle::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiTitleEdition::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiTitleAddress::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiTitleURL::ClassInfos),
	EbmlSemantic(false, false, KaxTagMultiTitleEmail::ClassInfos),
	EbmlSemantic(false, true,  KaxTagMultiTitleLanguage::ClassInfos),
};

EbmlSemantic KaxTagMultiAttachment_ContextList[1] =
{
	EbmlSemantic(true, false, KaxTagAttachment::ClassInfos),
};

EbmlSemantic KaxTagAttachment_ContextList[1] =
{
	EbmlSemantic(false, true,  KaxTagAttachmentID::ClassInfos),
};

const EbmlSemanticContext KaxTagMultiGlobal_Context = EbmlSemanticContext(countof(KaxTagMultiGlobal_ContextList), KaxTagMultiGlobal_ContextList, NULL, *GetKaxGlobal_Context, NULL);

const EbmlSemanticContext KaxTagMultiComment_Context = EbmlSemanticContext(countof(KaxTagMultiComment_ContextList), KaxTagMultiComment_ContextList, &KaxTag_Context, *GetKaxGlobal_Context, &KaxTagMultiComment::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommentName_Context = EbmlSemanticContext(0, NULL, &KaxTagMultiComment_Context, *GetKaxGlobal_Context, &KaxTagMultiCommentName::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommentComments_Context = EbmlSemanticContext(0, NULL, &KaxTagMultiComment_Context, *GetKaxGlobal_Context, &KaxTagMultiCommentComments::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommentLanguage_Context = EbmlSemanticContext(0, NULL, &KaxTagMultiComment_Context, *GetKaxGlobal_Context, &KaxTagMultiCommentLanguage::ClassInfos);

const EbmlSemanticContext KaxTagMultiCommercial_Context = EbmlSemanticContext(countof(KaxTagMultiCommercial_ContextList), KaxTagMultiCommercial_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiCommercial::ClassInfos);
const EbmlSemanticContext KaxTagCommercial_Context = EbmlSemanticContext(countof(KaxTagCommercial_ContextList), KaxTagCommercial_ContextList, &KaxTagMultiCommercial_Context, *GetKaxTagsGlobal_Context, &KaxTagCommercial::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommercialType_Context = EbmlSemanticContext(0, NULL, &KaxTagCommercial_Context, *GetKaxGlobal_Context, &KaxTagMultiCommercialType::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommercialAddress_Context = EbmlSemanticContext(0, NULL, &KaxTagCommercial_Context, *GetKaxGlobal_Context, &KaxTagMultiCommercialAddress::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommercialURL_Context = EbmlSemanticContext(0, NULL, &KaxTagCommercial_Context, *GetKaxGlobal_Context, &KaxTagMultiCommercialURL::ClassInfos);
const EbmlSemanticContext KaxTagMultiCommercialEmail_Context = EbmlSemanticContext(0, NULL, &KaxTagCommercial_Context, *GetKaxGlobal_Context, &KaxTagMultiCommercialEmail::ClassInfos);

const EbmlSemanticContext KaxTagMultiPrice_Context = EbmlSemanticContext(countof(KaxTagMultiPrice_ContextList), KaxTagMultiPrice_ContextList, &KaxTagCommercial_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiPrice::ClassInfos);
const EbmlSemanticContext KaxTagMultiPriceCurrency_Context = EbmlSemanticContext(0, NULL, &KaxTagMultiPrice_Context, *GetKaxGlobal_Context, &KaxTagMultiPriceCurrency::ClassInfos);
const EbmlSemanticContext KaxTagMultiPriceAmount_Context = EbmlSemanticContext(0, NULL, &KaxTagMultiPrice_Context, *GetKaxGlobal_Context, &KaxTagMultiPriceAmount::ClassInfos);
const EbmlSemanticContext KaxTagMultiPricePriceDate_Context = EbmlSemanticContext(0, NULL, &KaxTagMultiPrice_Context, *GetKaxGlobal_Context, &KaxTagMultiPricePriceDate::ClassInfos);

const EbmlSemanticContext KaxTagMultiDate_Context = EbmlSemanticContext(countof(KaxTagMultiDate_ContextList), KaxTagMultiDate_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiDate::ClassInfos);
const EbmlSemanticContext KaxTagDate_Context = EbmlSemanticContext(countof(KaxTagDate_ContextList), KaxTagDate_ContextList, &KaxTagMultiDate_Context, *GetKaxTagsGlobal_Context, &KaxTagDate::ClassInfos);
const EbmlSemanticContext KaxTagMultiDateType_Context = EbmlSemanticContext(0, NULL, &KaxTagDate_Context, *GetKaxGlobal_Context, &KaxTagMultiDateType::ClassInfos);
const EbmlSemanticContext KaxTagMultiDateDateBegin_Context = EbmlSemanticContext(0, NULL, &KaxTagDate_Context, *GetKaxGlobal_Context, &KaxTagMultiDateDateBegin::ClassInfos);
const EbmlSemanticContext KaxTagMultiDateDateEnd_Context = EbmlSemanticContext(0, NULL, &KaxTagDate_Context, *GetKaxGlobal_Context, &KaxTagMultiDateDateEnd::ClassInfos);

const EbmlSemanticContext KaxTagMultiEntity_Context = EbmlSemanticContext(countof(KaxTagMultiEntity_ContextList), KaxTagMultiEntity_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiEntity::ClassInfos);
const EbmlSemanticContext KaxTagEntity_Context = EbmlSemanticContext(countof(KaxTagEntity_ContextList), KaxTagEntity_ContextList, &KaxTagMultiEntity_Context, *GetKaxTagsGlobal_Context, &KaxTagEntity::ClassInfos);
const EbmlSemanticContext KaxTagMultiEntityType_Context = EbmlSemanticContext(0, NULL, &KaxTagEntity_Context, *GetKaxGlobal_Context, &KaxTagMultiEntityType::ClassInfos);
const EbmlSemanticContext KaxTagMultiEntityName_Context = EbmlSemanticContext(0, NULL, &KaxTagEntity_Context, *GetKaxGlobal_Context, &KaxTagMultiEntityName::ClassInfos);
const EbmlSemanticContext KaxTagMultiEntityURL_Context = EbmlSemanticContext(0, NULL, &KaxTagEntity_Context, *GetKaxGlobal_Context, &KaxTagMultiEntityURL::ClassInfos);
const EbmlSemanticContext KaxTagMultiEntityEmail_Context = EbmlSemanticContext(0, NULL, &KaxTagEntity_Context, *GetKaxGlobal_Context, &KaxTagMultiEntityEmail::ClassInfos);
const EbmlSemanticContext KaxTagMultiEntityAddress_Context = EbmlSemanticContext(0, NULL, &KaxTagEntity_Context, *GetKaxGlobal_Context, &KaxTagMultiEntityAddress::ClassInfos);

const EbmlSemanticContext KaxTagMultiIdentifier_Context = EbmlSemanticContext(countof(KaxTagMultiIdentifier_ContextList), KaxTagMultiIdentifier_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiIdentifier::ClassInfos);
const EbmlSemanticContext KaxTagIdentifier_Context = EbmlSemanticContext(countof(KaxTagIdentifier_ContextList), KaxTagIdentifier_ContextList, &KaxTagMultiIdentifier_Context, *GetKaxTagsGlobal_Context, &KaxTagIdentifier::ClassInfos);
const EbmlSemanticContext KaxTagMultiIdentifierType_Context = EbmlSemanticContext(0, NULL, &KaxTagIdentifier_Context, *GetKaxGlobal_Context, &KaxTagMultiIdentifierType::ClassInfos);
const EbmlSemanticContext KaxTagMultiIdentifierBinary_Context = EbmlSemanticContext(0, NULL, &KaxTagIdentifier_Context, *GetKaxGlobal_Context, &KaxTagMultiIdentifierBinary::ClassInfos);
const EbmlSemanticContext KaxTagMultiIdentifierString_Context = EbmlSemanticContext(0, NULL, &KaxTagIdentifier_Context, *GetKaxGlobal_Context, &KaxTagMultiIdentifierString::ClassInfos);

const EbmlSemanticContext KaxTagMultiLegal_Context = EbmlSemanticContext(countof(KaxTagMultiLegal_ContextList), KaxTagMultiLegal_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiLegal::ClassInfos);
const EbmlSemanticContext KaxTagLegal_Context = EbmlSemanticContext(countof(KaxTagLegal_ContextList), KaxTagLegal_ContextList, &KaxTagMultiLegal_Context, *GetKaxTagsGlobal_Context, &KaxTagLegal::ClassInfos);
const EbmlSemanticContext KaxTagMultiLegalType_Context = EbmlSemanticContext(0, NULL, &KaxTagLegal_Context, *GetKaxGlobal_Context, &KaxTagMultiLegalType::ClassInfos);
const EbmlSemanticContext KaxTagMultiLegalContent_Context = EbmlSemanticContext(0, NULL, &KaxTagLegal_Context, *GetKaxGlobal_Context, &KaxTagMultiLegalContent::ClassInfos);
const EbmlSemanticContext KaxTagMultiLegalURL_Context = EbmlSemanticContext(0, NULL, &KaxTagLegal_Context, *GetKaxGlobal_Context, &KaxTagMultiLegalURL::ClassInfos);
const EbmlSemanticContext KaxTagMultiLegalAddress_Context = EbmlSemanticContext(0, NULL, &KaxTagLegal_Context, *GetKaxGlobal_Context, &KaxTagMultiLegalAddress::ClassInfos);

const EbmlSemanticContext KaxTagMultiTitle_Context = EbmlSemanticContext(countof(KaxTagMultiTitle_ContextList), KaxTagMultiTitle_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiTitle::ClassInfos);
const EbmlSemanticContext KaxTagTitle_Context = EbmlSemanticContext(countof(KaxTagTitle_ContextList), KaxTagTitle_ContextList, &KaxTagMultiTitle_Context, *GetKaxTagsGlobal_Context, &KaxTagTitle::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleType_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleType::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleName_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleName::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleSubTitle_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleSubTitle::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleEdition_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleEdition::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleAddress_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleAddress::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleURL_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleURL::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleEmail_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleEmail::ClassInfos);
const EbmlSemanticContext KaxTagMultiTitleLanguage_Context = EbmlSemanticContext(0, NULL, &KaxTagTitle_Context, *GetKaxGlobal_Context, &KaxTagMultiTitleLanguage::ClassInfos);

const EbmlSemanticContext KaxTagMultiAttachment_Context = EbmlSemanticContext(countof(KaxTagMultiAttachment_ContextList), KaxTagMultiAttachment_ContextList, &KaxTag_Context, *GetKaxTagsGlobal_Context, &KaxTagMultiAttachment::ClassInfos);
const EbmlSemanticContext KaxTagAttachment_Context = EbmlSemanticContext(countof(KaxTagAttachment_ContextList), KaxTagAttachment_ContextList, &KaxTagMultiAttachment_Context, *GetKaxTagsGlobal_Context, &KaxTagAttachment::ClassInfos);
const EbmlSemanticContext KaxTagAttachmentID_Context = EbmlSemanticContext(0, NULL, &KaxTagAttachment_Context, *GetKaxGlobal_Context, &KaxTagAttachmentID::ClassInfos);

//The Muti Elements
EbmlId KaxTagMultiComment_TheId             (0x5B7B, 2);
EbmlId KaxTagMultiCommentName_TheId         (0x5F7D, 2);
EbmlId KaxTagMultiCommentComments_TheId     (0x5F7C, 2);
EbmlId KaxTagMultiCommentLanguage_TheId     (0x22B59D, 3);

EbmlId KaxTagMultiCommercial_TheId          (0x4DC7, 2);
EbmlId KaxTagCommercial_TheId               (0x4EC7, 2);
EbmlId KaxTagMultiCommercialType_TheId      (0x5BD7, 2);
EbmlId KaxTagMultiCommercialAddress_TheId   (0x5BBB, 2);
EbmlId KaxTagMultiCommercialURL_TheId       (0x5BDA, 2);
EbmlId KaxTagMultiCommercialEmail_TheId     (0x5BC0, 2);

EbmlId KaxTagMultiPrice_TheId               (0x5BC3, 2);
EbmlId KaxTagMultiPriceCurrency_TheId       (0x5B6C, 2);
EbmlId KaxTagMultiPriceAmount_TheId         (0x5B6E, 2);
EbmlId KaxTagMultiPricePriceDate_TheId      (0x5B6F, 2);

EbmlId KaxTagMultiDate_TheId                (0x4DC8, 2);
EbmlId KaxTagDate_TheId                     (0x4EC8, 2);
EbmlId KaxTagMultiDateType_TheId            (0x5BD8, 2);
EbmlId KaxTagMultiDateDateBegin_TheId       (0x4460, 2);
EbmlId KaxTagMultiDateDateEnd_TheId         (0x4462, 2);

EbmlId KaxTagMultiEntity_TheId              (0x4DC9, 2);
EbmlId KaxTagEntity_TheId                   (0x4EC9, 2);
EbmlId KaxTagMultiEntityType_TheId          (0x5BD9, 2);
EbmlId KaxTagMultiEntityName_TheId          (0x5BED, 2);
EbmlId KaxTagMultiEntityAddress_TheId       (0x5BDC, 2);
EbmlId KaxTagMultiEntityURL_TheId           (0x5BDB, 2);
EbmlId KaxTagMultiEntityEmail_TheId         (0x5BC1, 2);

EbmlId KaxTagMultiIdentifier_TheId          (0x4DC6, 2);
EbmlId KaxTagIdentifier_TheId               (0x4EC6, 2);
EbmlId KaxTagMultiIdentifierType_TheId      (0x5BAD, 2);
EbmlId KaxTagMultiIdentifierBinary_TheId    (0x6B67, 2);
EbmlId KaxTagMultiIdentifierString_TheId    (0x6B68, 2);

EbmlId KaxTagMultiLegal_TheId               (0x4DC5, 2);
EbmlId KaxTagLegal_TheId                    (0x4EC5, 2);
EbmlId KaxTagMultiLegalType_TheId           (0x5BBD, 2);
EbmlId KaxTagMultiLegalContent_TheId        (0x5BB2, 2);
EbmlId KaxTagMultiLegalURL_TheId            (0x5B34, 2);
EbmlId KaxTagMultiLegalAddress_TheId        (0x5B9B, 2);

EbmlId KaxTagMultiTitle_TheId               (0x4DC4, 2);
EbmlId KaxTagTitle_TheId                    (0x4EC4, 2);
EbmlId KaxTagMultiTitleType_TheId           (0x5B7D, 2);
EbmlId KaxTagMultiTitleName_TheId           (0x5BB9, 2);
EbmlId KaxTagMultiTitleSubTitle_TheId       (0x5B5B, 2);
EbmlId KaxTagMultiTitleEdition_TheId        (0x5BAE, 2);
EbmlId KaxTagMultiTitleAddress_TheId        (0x5B33, 2);
EbmlId KaxTagMultiTitleURL_TheId            (0x5BA9, 2);
EbmlId KaxTagMultiTitleEmail_TheId          (0x5BC9, 2);
EbmlId KaxTagMultiTitleLanguage_TheId       (0x22B59E, 3);

EbmlId KaxTagMultiAttachment_TheId          (0x4DC3, 2);
EbmlId KaxTagAttachment_TheId               (0x4EC3, 2);
EbmlId KaxTagAttachmentID_TheId             (0x5BA0, 2);

const EbmlCallbacks KaxTagMultiComment::ClassInfos(KaxTagMultiComment::Create, KaxTagMultiComment_TheId, "MultiComment", KaxTagMultiComment_Context);
const EbmlCallbacks KaxTagMultiCommentName::ClassInfos(KaxTagMultiCommentName::Create, KaxTagMultiCommentName_TheId, "MultiCommentName", KaxTagMultiCommentName_Context);
const EbmlCallbacks KaxTagMultiCommentComments::ClassInfos(KaxTagMultiCommentComments::Create, KaxTagMultiCommentComments_TheId, "MultiCommentComments", KaxTagMultiCommentComments_Context);
const EbmlCallbacks KaxTagMultiCommentLanguage::ClassInfos(KaxTagMultiCommentLanguage::Create, KaxTagMultiCommentLanguage_TheId, "MultiCommentLanguage", KaxTagMultiCommentLanguage_Context);

const EbmlCallbacks KaxTagMultiCommercial::ClassInfos(KaxTagMultiCommercial::Create, KaxTagMultiCommercial_TheId, "MultiCommercial", KaxTagMultiCommercial_Context);
const EbmlCallbacks KaxTagCommercial::ClassInfos(KaxTagCommercial::Create, KaxTagCommercial_TheId, "Commercial", KaxTagCommercial_Context);
const EbmlCallbacks KaxTagMultiCommercialType::ClassInfos(KaxTagMultiCommercialType::Create, KaxTagMultiCommercialType_TheId, "MultiCommercialType", KaxTagMultiCommercialType_Context);
const EbmlCallbacks KaxTagMultiCommercialAddress::ClassInfos(KaxTagMultiCommercialAddress::Create, KaxTagMultiCommercialAddress_TheId, "MultiCommercialAddress", KaxTagMultiCommercialAddress_Context);
const EbmlCallbacks KaxTagMultiCommercialURL::ClassInfos(KaxTagMultiCommercialURL::Create, KaxTagMultiCommercialURL_TheId, "MultiCommercialURL", KaxTagMultiCommercialURL_Context);
const EbmlCallbacks KaxTagMultiCommercialEmail::ClassInfos(KaxTagMultiCommercialEmail::Create, KaxTagMultiCommercialEmail_TheId, "MultiCommercialEmail", KaxTagMultiCommercialEmail_Context);

const EbmlCallbacks KaxTagMultiPrice::ClassInfos(KaxTagMultiPrice::Create, KaxTagMultiPrice_TheId, "MultiPrice", KaxTagMultiPrice_Context);
const EbmlCallbacks KaxTagMultiPriceCurrency::ClassInfos(KaxTagMultiPriceCurrency::Create, KaxTagMultiPriceCurrency_TheId, "MultiPriceCurrency", KaxTagMultiPriceCurrency_Context);
const EbmlCallbacks KaxTagMultiPriceAmount::ClassInfos(KaxTagMultiPriceAmount::Create, KaxTagMultiPriceAmount_TheId, "MultiPriceAmount", KaxTagMultiPriceAmount_Context);
const EbmlCallbacks KaxTagMultiPricePriceDate::ClassInfos(KaxTagMultiPricePriceDate::Create, KaxTagMultiPricePriceDate_TheId, "MultiPricePriceDate", KaxTagMultiPricePriceDate_Context);

const EbmlCallbacks KaxTagMultiDate::ClassInfos(KaxTagMultiDate::Create, KaxTagMultiDate_TheId, "MultiDate", KaxTagMultiDate_Context);
const EbmlCallbacks KaxTagDate::ClassInfos(KaxTagDate::Create, KaxTagDate_TheId, "Date", KaxTagDate_Context);
const EbmlCallbacks KaxTagMultiDateType::ClassInfos(KaxTagMultiDateType::Create, KaxTagMultiDateType_TheId, "MultiDateType", KaxTagMultiDateType_Context);
const EbmlCallbacks KaxTagMultiDateDateBegin::ClassInfos(KaxTagMultiDateDateBegin::Create, KaxTagMultiDateDateBegin_TheId, "MultiDateDateBegin", KaxTagMultiDateDateBegin_Context);
const EbmlCallbacks KaxTagMultiDateDateEnd::ClassInfos(KaxTagMultiDateDateEnd::Create, KaxTagMultiDateDateEnd_TheId, "MultiDateDateEnd", KaxTagMultiDateDateEnd_Context);

const EbmlCallbacks KaxTagMultiEntity::ClassInfos(KaxTagMultiEntity::Create, KaxTagMultiEntity_TheId, "MultiEntity", KaxTagMultiEntity_Context);
const EbmlCallbacks KaxTagEntity::ClassInfos(KaxTagEntity::Create, KaxTagEntity_TheId, "Entity", KaxTagEntity_Context);
const EbmlCallbacks KaxTagMultiEntityType::ClassInfos(KaxTagMultiEntityType::Create, KaxTagMultiEntityType_TheId, "MultiEntityType", KaxTagMultiEntityType_Context);
const EbmlCallbacks KaxTagMultiEntityName::ClassInfos(KaxTagMultiEntityName::Create, KaxTagMultiEntityName_TheId, "MultiEntityName", KaxTagMultiEntityName_Context);
const EbmlCallbacks KaxTagMultiEntityURL::ClassInfos(KaxTagMultiEntityURL::Create, KaxTagMultiEntityURL_TheId, "MultiEntityURL", KaxTagMultiEntityURL_Context);
const EbmlCallbacks KaxTagMultiEntityEmail::ClassInfos(KaxTagMultiEntityEmail::Create, KaxTagMultiEntityEmail_TheId, "MultiEntityEmail", KaxTagMultiEntityEmail_Context);
const EbmlCallbacks KaxTagMultiEntityAddress::ClassInfos(KaxTagMultiEntityAddress::Create, KaxTagMultiEntityAddress_TheId, "MultiEntityAddress", KaxTagMultiEntityAddress_Context);

const EbmlCallbacks KaxTagMultiIdentifier::ClassInfos(KaxTagMultiIdentifier::Create, KaxTagMultiIdentifier_TheId, "MultiIdentifier", KaxTagMultiIdentifier_Context);
const EbmlCallbacks KaxTagIdentifier::ClassInfos(KaxTagIdentifier::Create, KaxTagIdentifier_TheId, "Identifier", KaxTagIdentifier_Context);
const EbmlCallbacks KaxTagMultiIdentifierType::ClassInfos(KaxTagMultiIdentifierType::Create, KaxTagMultiIdentifierType_TheId, "TagMultiIdentifierType", KaxTagMultiIdentifierType_Context);
const EbmlCallbacks KaxTagMultiIdentifierBinary::ClassInfos(KaxTagMultiIdentifierBinary::Create, KaxTagMultiIdentifierBinary_TheId, "MultiIdentifierBinary", KaxTagMultiIdentifierBinary_Context);
const EbmlCallbacks KaxTagMultiIdentifierString::ClassInfos(KaxTagMultiIdentifierString::Create, KaxTagMultiIdentifierString_TheId, "MultiIdentifierString", KaxTagMultiIdentifierString_Context);

const EbmlCallbacks KaxTagMultiLegal::ClassInfos(KaxTagMultiLegal::Create, KaxTagMultiLegal_TheId, "MultiLegal", KaxTagMultiLegal_Context);
const EbmlCallbacks KaxTagLegal::ClassInfos(KaxTagLegal::Create, KaxTagLegal_TheId, "Legal", KaxTagLegal_Context);
const EbmlCallbacks KaxTagMultiLegalType::ClassInfos(KaxTagMultiLegalType::Create, KaxTagMultiLegalType_TheId, "KaxTagMultiLegalType", KaxTagMultiLegalType_Context);
const EbmlCallbacks KaxTagMultiLegalContent::ClassInfos(KaxTagMultiLegalContent::Create, KaxTagMultiLegalContent_TheId, "TagMultiLegalContent", KaxTagMultiLegalContent_Context);
const EbmlCallbacks KaxTagMultiLegalURL::ClassInfos(KaxTagMultiLegalURL::Create, KaxTagMultiLegalURL_TheId, "KaxTagMultiLegalURL", KaxTagMultiLegalURL_Context);
const EbmlCallbacks KaxTagMultiLegalAddress::ClassInfos(KaxTagMultiLegalAddress::Create, KaxTagMultiLegalAddress_TheId, "KaxTagMultiLegalAddress", KaxTagMultiLegalAddress_Context);

const EbmlCallbacks KaxTagMultiTitle::ClassInfos(KaxTagMultiTitle::Create, KaxTagMultiTitle_TheId, "MultiEntity", KaxTagMultiTitle_Context);
const EbmlCallbacks KaxTagTitle::ClassInfos(KaxTagTitle::Create, KaxTagTitle_TheId, "Entity", KaxTagTitle_Context);
const EbmlCallbacks KaxTagMultiTitleType::ClassInfos(KaxTagMultiTitleType::Create, KaxTagMultiTitleType_TheId, "MultiTitleType", KaxTagMultiTitleType_Context);
const EbmlCallbacks KaxTagMultiTitleName::ClassInfos(KaxTagMultiTitleName::Create, KaxTagMultiTitleName_TheId, "MultiTitleName", KaxTagMultiTitleName_Context);
const EbmlCallbacks KaxTagMultiTitleSubTitle::ClassInfos(KaxTagMultiTitleSubTitle::Create, KaxTagMultiTitleSubTitle_TheId, "MultiTitleSubTitle", KaxTagMultiTitleSubTitle_Context);
const EbmlCallbacks KaxTagMultiTitleEdition::ClassInfos(KaxTagMultiTitleEdition::Create, KaxTagMultiTitleEdition_TheId, "MultiTitleEdition", KaxTagMultiTitleEdition_Context);
const EbmlCallbacks KaxTagMultiTitleAddress::ClassInfos(KaxTagMultiTitleAddress::Create, KaxTagMultiTitleAddress_TheId, "MultiTitleAddress", KaxTagMultiTitleAddress_Context);
const EbmlCallbacks KaxTagMultiTitleURL::ClassInfos(KaxTagMultiTitleURL::Create, KaxTagMultiTitleURL_TheId, "MultiTitleURL", KaxTagMultiTitleURL_Context);
const EbmlCallbacks KaxTagMultiTitleEmail::ClassInfos(KaxTagMultiTitleEmail::Create, KaxTagMultiTitleEmail_TheId, "MultiTitleEmail", KaxTagMultiTitleEmail_Context);
const EbmlCallbacks KaxTagMultiTitleLanguage::ClassInfos(KaxTagMultiTitleLanguage::Create, KaxTagMultiTitleLanguage_TheId, "MultiTitleLanguage", KaxTagMultiTitleLanguage_Context);

const EbmlCallbacks KaxTagMultiAttachment::ClassInfos(KaxTagMultiAttachment::Create, KaxTagMultiAttachment_TheId, "TagMultiAttachment", KaxTagMultiAttachment_Context);
const EbmlCallbacks KaxTagAttachment::ClassInfos(KaxTagAttachment::Create, KaxTagAttachment_TheId, "TagAttachment", KaxTagAttachment_Context);
const EbmlCallbacks KaxTagAttachmentID::ClassInfos(KaxTagAttachmentID::Create, KaxTagAttachmentID_TheId, "TagAttachmentID", KaxTagAttachmentID_Context);

KaxTagMultiComment::KaxTagMultiComment()
	:EbmlMaster(KaxTagMultiComment_Context)
{}


KaxTagMultiCommercial::KaxTagMultiCommercial()
	:EbmlMaster(KaxTagMultiCommercial_Context)
{}

KaxTagCommercial::KaxTagCommercial()
	:EbmlMaster(KaxTagCommercial_Context)
{}

KaxTagMultiPrice::KaxTagMultiPrice()
	:EbmlMaster(KaxTagMultiPrice_Context)
{}

KaxTagMultiDate::KaxTagMultiDate()
	:EbmlMaster(KaxTagMultiDate_Context)
{}

KaxTagDate::KaxTagDate()
	:EbmlMaster(KaxTagDate_Context)
{}

KaxTagMultiEntity::KaxTagMultiEntity()
	:EbmlMaster(KaxTagMultiEntity_Context)
{}
	
KaxTagEntity::KaxTagEntity()
	:EbmlMaster(KaxTagEntity_Context)
{}
	
KaxTagMultiLegal::KaxTagMultiLegal()
	:EbmlMaster(KaxTagMultiLegal_Context)
{}

KaxTagLegal::KaxTagLegal()
	:EbmlMaster(KaxTagLegal_Context)
{}

KaxTagMultiIdentifier::KaxTagMultiIdentifier()
	:EbmlMaster(KaxTagMultiIdentifier_Context)
{}

KaxTagIdentifier::KaxTagIdentifier()
	:EbmlMaster(KaxTagIdentifier_Context)
{}

KaxTagMultiTitle::KaxTagMultiTitle()
	:EbmlMaster(KaxTagMultiTitle_Context)
{}

KaxTagTitle::KaxTagTitle()
	:EbmlMaster(KaxTagTitle_Context)
{}

KaxTagMultiAttachment::KaxTagMultiAttachment()
	:EbmlMaster(KaxTagMultiAttachment_Context)
{}

KaxTagAttachment::KaxTagAttachment()
	:EbmlMaster(KaxTagAttachment_Context)
{}

const EbmlSemanticContext & GetKaxTagsGlobal_Context()
{
	return KaxTagMultiGlobal_Context;
}

END_LIBMATROSKA_NAMESPACE

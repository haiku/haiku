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
	\version \$Id: KaxContentEncoding.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#include "matroska/KaxContentEncoding.h"
#include "matroska/KaxContexts.h"

START_LIBMATROSKA_NAMESPACE

const EbmlSemantic KaxContentEncodings_ContextList[1] = {
  EbmlSemantic(true, true, KaxContentEncoding::ClassInfos),
};

const EbmlSemantic KaxContentEncoding_ContextList[5] = {
  EbmlSemantic(true, true, KaxContentEncodingOrder::ClassInfos),
  EbmlSemantic(true, true, KaxContentEncodingScope::ClassInfos),
  EbmlSemantic(true, true, KaxContentEncodingType::ClassInfos),
  EbmlSemantic(false, true, KaxContentCompression::ClassInfos),
  EbmlSemantic(false, true, KaxContentEncryption::ClassInfos),
};

const EbmlSemantic KaxContentCompression_ContextList[2] = {
  EbmlSemantic(true, true, KaxContentCompAlgo::ClassInfos),
  EbmlSemantic(false, true, KaxContentCompSettings::ClassInfos),
};

const EbmlSemantic KaxContentEncryption_ContextList[6] = {
  EbmlSemantic(false, true, KaxContentEncAlgo::ClassInfos),
  EbmlSemantic(false, true, KaxContentEncKeyID::ClassInfos),
  EbmlSemantic(false, true, KaxContentSignature::ClassInfos),
  EbmlSemantic(false, true, KaxContentSigKeyID::ClassInfos),
  EbmlSemantic(false, true, KaxContentSigAlgo::ClassInfos),
  EbmlSemantic(false, true, KaxContentSigHashAlgo::ClassInfos),
};

EbmlId KaxContentEncodings_TheId       (0x6d80, 2);
EbmlId KaxContentEncoding_TheId        (0x6240, 2);
EbmlId KaxContentEncodingOrder_TheId   (0x5031, 2);
EbmlId KaxContentEncodingScope_TheId   (0x5032, 2);
EbmlId KaxContentEncodingType_TheId    (0x5033, 2);
EbmlId KaxContentCompression_TheId     (0x5034, 2);
EbmlId KaxContentCompAlgo_TheId        (0x4254, 2);
EbmlId KaxContentCompSettings_TheId    (0x4255, 2);
EbmlId KaxContentEncryption_TheId      (0x5035, 2);
EbmlId KaxContentEncAlgo_TheId         (0x47e1, 2);
EbmlId KaxContentEncKeyID_TheId        (0x47e2, 2);
EbmlId KaxContentSignature_TheId       (0x47e3, 2);
EbmlId KaxContentSigKeyID_TheId        (0x47e4, 2);
EbmlId KaxContentSigAlgo_TheId         (0x47e5, 2);
EbmlId KaxContentSigHashAlgo_TheId     (0x47e6, 2);
 
const EbmlSemanticContext KaxContentEncodings_Context =
EbmlSemanticContext(countof(KaxContentEncodings_ContextList),
                    KaxContentEncodings_ContextList, &KaxTrackEntry_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncodings::ClassInfos);

const EbmlSemanticContext KaxContentEncoding_Context =
EbmlSemanticContext(countof(KaxContentEncoding_ContextList),
                    KaxContentEncoding_ContextList,
                    &KaxContentEncodings_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncoding::ClassInfos);

const EbmlSemanticContext KaxContentEncodingOrder_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncoding_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncodingOrder::ClassInfos);

const EbmlSemanticContext KaxContentEncodingScope_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncoding_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncodingScope::ClassInfos);

const EbmlSemanticContext KaxContentEncodingType_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncoding_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncodingType::ClassInfos);

const EbmlSemanticContext KaxContentCompression_Context =
EbmlSemanticContext(countof(KaxContentCompression_ContextList),
                    KaxContentCompression_ContextList,
                    &KaxContentEncoding_Context, *GetKaxGlobal_Context,
                    &KaxContentCompression::ClassInfos);

const EbmlSemanticContext KaxContentCompAlgo_Context =
EbmlSemanticContext(0, NULL, &KaxContentCompression_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentCompAlgo::ClassInfos);

const EbmlSemanticContext KaxContentCompSettings_Context =
EbmlSemanticContext(0, NULL, &KaxContentCompression_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentCompSettings::ClassInfos);

const EbmlSemanticContext KaxContentEncryption_Context =
EbmlSemanticContext(countof(KaxContentEncryption_ContextList),
                    KaxContentEncryption_ContextList,
                    &KaxContentEncoding_Context, *GetKaxGlobal_Context,
                    &KaxContentEncryption::ClassInfos);

const EbmlSemanticContext KaxContentEncAlgo_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncryption_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncAlgo::ClassInfos);

const EbmlSemanticContext KaxContentEncKeyID_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncryption_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentEncKeyID::ClassInfos);

const EbmlSemanticContext KaxContentSignature_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncryption_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentSignature::ClassInfos);

const EbmlSemanticContext KaxContentSigAlgo_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncryption_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentSigKeyID::ClassInfos);

const EbmlSemanticContext KaxContentSigHashAlgo_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncryption_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentSigKeyID::ClassInfos);

const EbmlSemanticContext KaxContentSigKeyID_Context =
EbmlSemanticContext(0, NULL, &KaxContentEncryption_Context,
                    *GetKaxGlobal_Context,
                    &KaxContentSigKeyID::ClassInfos);

const EbmlCallbacks
KaxContentEncodings::ClassInfos(KaxContentEncodings::Create,
                                KaxContentEncodings_TheId,
                                "ContentEncodings",
                                KaxContentEncodings_Context);
const EbmlCallbacks
KaxContentEncoding::ClassInfos(KaxContentEncoding::Create,
                               KaxContentEncoding_TheId,
                               "ContentEncoding",
                               KaxContentEncoding_Context);
const EbmlCallbacks
KaxContentEncodingOrder::ClassInfos(KaxContentEncodingOrder::Create,
                                    KaxContentEncodingOrder_TheId,
                                    "ContentEncodingOrder",
                                    KaxContentEncodingOrder_Context);
const EbmlCallbacks
KaxContentEncodingScope::ClassInfos(KaxContentEncodingScope::Create,
                                    KaxContentEncodingScope_TheId,
                                    "ContentEncodingScope",
                                    KaxContentEncodingScope_Context);
const EbmlCallbacks
KaxContentEncodingType::ClassInfos(KaxContentEncodingType::Create,
                                   KaxContentEncodingType_TheId,
                                   "ContentEncodingType",
                                   KaxContentEncodingType_Context);
const EbmlCallbacks
KaxContentCompression::ClassInfos(KaxContentCompression::Create,
                                  KaxContentCompression_TheId,
                                  "ContentCompression",
                                  KaxContentCompression_Context);
const EbmlCallbacks
KaxContentCompAlgo::ClassInfos(KaxContentCompAlgo::Create,
                               KaxContentCompAlgo_TheId,
                               "ContentCompAlgo",
                               KaxContentCompAlgo_Context);
const EbmlCallbacks
KaxContentCompSettings::ClassInfos(KaxContentCompSettings::Create,
                                   KaxContentCompSettings_TheId,
                                   "ContentCompSettings",
                                   KaxContentCompSettings_Context);
const EbmlCallbacks
KaxContentEncryption::ClassInfos(KaxContentEncryption::Create,
                                 KaxContentEncryption_TheId,
                                 "ContentEncryption",
                                 KaxContentEncryption_Context);
const EbmlCallbacks
KaxContentEncAlgo::ClassInfos(KaxContentEncAlgo::Create,
                              KaxContentEncAlgo_TheId,
                              "ContentEncAlgo",
                              KaxContentEncAlgo_Context);
const EbmlCallbacks
KaxContentEncKeyID::ClassInfos(KaxContentEncKeyID::Create,
                               KaxContentEncKeyID_TheId,
                               "ContentEncKeyID",
                               KaxContentEncKeyID_Context);
const EbmlCallbacks
KaxContentSignature::ClassInfos(KaxContentSignature::Create,
                                KaxContentSignature_TheId,
                                "ContentSignature",
                                KaxContentSignature_Context);
const EbmlCallbacks
KaxContentSigAlgo::ClassInfos(KaxContentSigAlgo::Create,
                              KaxContentSigAlgo_TheId,
                              "ContentSigAlgo",
                              KaxContentSigAlgo_Context);
const EbmlCallbacks
KaxContentSigHashAlgo::ClassInfos(KaxContentSigHashAlgo::Create,
                                  KaxContentSigHashAlgo_TheId,
                                  "ContentSigHashAlgo",
                                  KaxContentSigHashAlgo_Context);
const EbmlCallbacks
KaxContentSigKeyID::ClassInfos(KaxContentSigKeyID::Create,
                               KaxContentSigKeyID_TheId,
                               "ContentSigKeyID",
                               KaxContentSigKeyID_Context);

KaxContentEncodings::KaxContentEncodings():
  EbmlMaster(KaxContentEncodings_Context) {
}

KaxContentEncoding::KaxContentEncoding():
  EbmlMaster(KaxContentEncoding_Context) {
}

KaxContentCompression::KaxContentCompression():
  EbmlMaster(KaxContentCompression_Context) {
}

KaxContentEncryption::KaxContentEncryption():
  EbmlMaster(KaxContentEncryption_Context) {
}

END_LIBMATROSKA_NAMESPACE

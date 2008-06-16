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
	\version \$Id: KaxAttached.cpp 1202 2005-08-30 14:39:01Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxAttached.h"
#include "matroska/KaxContexts.h"

// sub elements

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

#if MATROSKA_VERSION == 1
EbmlSemantic KaxAttached_ContextList[5] =
#else // MATROSKA_VERSION
EbmlSemantic KaxAttached_ContextList[6] =
#endif // MATROSKA_VERSION
{
	EbmlSemantic(true,  true, KaxFileName::ClassInfos),
	EbmlSemantic(true,  true, KaxMimeType::ClassInfos),
	EbmlSemantic(true,  true, KaxFileData::ClassInfos),
	EbmlSemantic(false, true, KaxFileDescription::ClassInfos),
	EbmlSemantic(true,  true, KaxFileUID::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(false, true, KaxFileReferral::ClassInfos),
#endif // MATROSKA_VERSION
};

EbmlId KaxAttached_TheId       (0x61A7, 2);
EbmlId KaxFileDescription_TheId(0x467E, 2);
EbmlId KaxFileName_TheId       (0x466E, 2);
EbmlId KaxMimeType_TheId       (0x4660, 2);
EbmlId KaxFileData_TheId       (0x465C, 2);
EbmlId KaxFileUID_TheId        (0x46AE, 2);
#if MATROSKA_VERSION >= 2
EbmlId KaxFileReferral_TheId   (0x4675, 2);
#endif // MATROSKA_VERSION

const EbmlSemanticContext KaxAttached_Context = EbmlSemanticContext(countof(KaxAttached_ContextList), KaxAttached_ContextList, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxAttached::ClassInfos);
const EbmlSemanticContext KaxFileDescription_Context = EbmlSemanticContext(0, NULL, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxFileDescription::ClassInfos);
const EbmlSemanticContext KaxFileName_Context        = EbmlSemanticContext(0, NULL, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxFileName::ClassInfos);
const EbmlSemanticContext KaxMimeType_Context        = EbmlSemanticContext(0, NULL, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxMimeType::ClassInfos);
const EbmlSemanticContext KaxFileData_Context        = EbmlSemanticContext(0, NULL, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxFileData::ClassInfos);
const EbmlSemanticContext KaxFileUID_Context         = EbmlSemanticContext(0, NULL, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxFileUID::ClassInfos);
#if MATROSKA_VERSION >= 2
const EbmlSemanticContext KaxFileReferral_Context    = EbmlSemanticContext(0, NULL, &KaxAttachments_Context, *GetKaxGlobal_Context, &KaxFileReferral::ClassInfos);
#endif // MATROSKA_VERSION

const EbmlCallbacks KaxAttached::ClassInfos(KaxAttached::Create, KaxAttached_TheId, "AttachedFile", KaxAttached_Context);
const EbmlCallbacks KaxFileDescription::ClassInfos(KaxFileDescription::Create, KaxFileDescription_TheId, "FileDescription", KaxFileDescription_Context);
const EbmlCallbacks KaxFileName::ClassInfos(KaxFileName::Create, KaxFileName_TheId, "FileName", KaxFileName_Context);
const EbmlCallbacks KaxMimeType::ClassInfos(KaxMimeType::Create, KaxMimeType_TheId, "FileMimeType", KaxMimeType_Context);
const EbmlCallbacks KaxFileData::ClassInfos(KaxFileData::Create, KaxFileData_TheId, "FileData", KaxFileData_Context);
const EbmlCallbacks KaxFileUID::ClassInfos(KaxFileUID::Create, KaxFileUID_TheId, "FileUID", KaxFileUID_Context);
#if MATROSKA_VERSION >= 2
const EbmlCallbacks KaxFileReferral::ClassInfos(KaxFileReferral::Create, KaxFileReferral_TheId, "FileReferral", KaxFileReferral_Context);
#endif // MATROSKA_VERSION

KaxAttached::KaxAttached()
 :EbmlMaster(KaxAttached_Context)
{
	SetSizeLength(2); // mandatory min size support (for easier updating) (2^(7*2)-2 = 16Ko)
}

END_LIBMATROSKA_NAMESPACE

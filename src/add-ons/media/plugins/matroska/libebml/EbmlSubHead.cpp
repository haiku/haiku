/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
**
** This file is part of libebml.
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
	\version \$Id: EbmlSubHead.cpp 639 2004-07-09 20:59:14Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlContexts.h"

START_LIBEBML_NAMESPACE

EbmlId EVersion_TheId            (0x4286, 2);
EbmlId EReadVersion_TheId        (0x42F7, 2);
EbmlId EMaxIdLength_TheId        (0x42F2, 2);
EbmlId EMaxSizeLength_TheId      (0x42F3, 2);
EbmlId EDocType_TheId            (0x4282, 2);
EbmlId EDocTypeVersion_TheId     (0x4287, 2);
EbmlId EDocTypeReadVersion_TheId (0x4285, 2);

const EbmlCallbacks EVersion::ClassInfos(EVersion::Create,               EVersion_TheId,        "EBMLVersion",                        EVersion_Context);
const EbmlCallbacks EReadVersion::ClassInfos(EReadVersion::Create,       EReadVersion_TheId,    "EBMLReadVersion",                    EReadVersion_Context);
const EbmlCallbacks EMaxIdLength::ClassInfos(EMaxIdLength::Create,       EMaxIdLength_TheId,    "EBMLMaxIdLength",                    EMaxIdLength_Context);
const EbmlCallbacks EMaxSizeLength::ClassInfos(EMaxSizeLength::Create,   EMaxSizeLength_TheId,  "EBMLMaxSizeLength",                  EMaxSizeLength_Context);
const EbmlCallbacks EDocType::ClassInfos(EDocType::Create,               EDocType_TheId,        "EBMLDocType",                        EDocType_Context);
const EbmlCallbacks EDocTypeVersion::ClassInfos(EDocTypeVersion::Create, EDocTypeVersion_TheId, "EBMLDocTypeVersion",                 EDocTypeVersion_Context);
const EbmlCallbacks EDocTypeReadVersion::ClassInfos(EDocTypeReadVersion::Create, EDocTypeReadVersion_TheId, "EBMLDocTypeReadVersion", EDocTypeReadVersion_Context);

const EbmlSemanticContext EVersion_Context        = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EVersion::ClassInfos);
const EbmlSemanticContext EReadVersion_Context    = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EReadVersion::ClassInfos);
const EbmlSemanticContext EMaxIdLength_Context    = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EMaxIdLength::ClassInfos);
const EbmlSemanticContext EMaxSizeLength_Context  = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EMaxSizeLength::ClassInfos);
const EbmlSemanticContext EDocType_Context        = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EDocType::ClassInfos);
const EbmlSemanticContext EDocTypeVersion_Context = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EDocTypeVersion::ClassInfos);
const EbmlSemanticContext EDocTypeReadVersion_Context = EbmlSemanticContext(0, NULL, &EbmlHead_Context, *GetEbmlGlobal_Context, &EDocTypeReadVersion::ClassInfos);

END_LIBEBML_NAMESPACE

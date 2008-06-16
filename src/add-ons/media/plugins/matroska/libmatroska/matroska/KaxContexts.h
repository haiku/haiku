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
	\version \$Id: KaxContexts.h,v 1.6 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_CONTEXTS_H
#define LIBMATROSKA_CONTEXTS_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlElement.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

extern const EbmlSemanticContext MATROSKA_DLL_API KaxMatroska_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxSegment_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxAttachments_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxAttached_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxFileDescription_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxFileName_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxMimeType_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxFileData_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxChapters_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCluster_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTags_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTag_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxBlockGroup_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxReferencePriority_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxReferenceBlock_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxReferenceVirtual_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCues_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxInfo_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxSeekHead_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTracks_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackEntry_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackNumber_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackType_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackFlagEnabled_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackFlagDefault_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackFlagLacing_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackName_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackLanguage_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecID_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecPrivate_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecName_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecSettings_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecInfoURL_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecDownloadURL_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxCodecDecodeAll_Context;
extern const EbmlSemanticContext MATROSKA_DLL_API KaxTrackOverlay_Context;

extern const EbmlSemanticContext & MATROSKA_DLL_API GetKaxGlobal_Context();
extern const EbmlSemanticContext & MATROSKA_DLL_API GetKaxTagsGlobal_Context();

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_CONTEXTS_H

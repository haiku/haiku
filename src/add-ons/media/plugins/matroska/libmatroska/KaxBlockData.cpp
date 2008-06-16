/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: KaxBlockData.cpp 1226 2005-10-13 21:16:43Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "matroska/KaxBlockData.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxBlock.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

const EbmlSemantic KaxSlices_ContextList[1] =
{
	EbmlSemantic(false, false,  KaxTimeSlice::ClassInfos),
};

const EbmlSemantic KaxTimeSlice_ContextList[5] =
{
	EbmlSemantic(false, true,  KaxSliceLaceNumber::ClassInfos),
	EbmlSemantic(false, true,  KaxSliceFrameNumber::ClassInfos),
	EbmlSemantic(false, true,  KaxSliceBlockAddID::ClassInfos),
	EbmlSemantic(false, true,  KaxSliceDelay::ClassInfos),
	EbmlSemantic(false, true,  KaxSliceDuration::ClassInfos),
};

EbmlId KaxReferencePriority_TheId(0xFA, 1);
EbmlId KaxReferenceBlock_TheId   (0xFB, 1);
EbmlId KaxSlices_TheId           (0x8E, 1);
EbmlId KaxTimeSlice_TheId        (0xE8, 1);
EbmlId KaxSliceLaceNumber_TheId  (0xCC, 1);
EbmlId KaxSliceFrameNumber_TheId (0xCD, 1);
EbmlId KaxSliceBlockAddID_TheId  (0xCB, 1);
EbmlId KaxSliceDelay_TheId       (0xCE, 1);
EbmlId KaxSliceDuration_TheId    (0xCF, 1);
#if MATROSKA_VERSION >= 2
EbmlId KaxReferenceVirtual_TheId (0xFD, 1);
#endif // MATROSKA_VERSION

const EbmlSemanticContext KaxReferencePriority_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxReferencePriority::ClassInfos);
const EbmlSemanticContext KaxReferenceBlock_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxReferenceBlock::ClassInfos);
const EbmlSemanticContext KaxSlices_Context = EbmlSemanticContext(countof(KaxSlices_ContextList), KaxSlices_ContextList, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxSlices::ClassInfos);
const EbmlSemanticContext KaxTimeSlice_Context = EbmlSemanticContext(countof(KaxTimeSlice_ContextList), KaxTimeSlice_ContextList, &KaxSlices_Context, *GetKaxGlobal_Context, &KaxTimeSlice::ClassInfos);
const EbmlSemanticContext KaxSliceLaceNumber_Context = EbmlSemanticContext(0, NULL, &KaxTimeSlice_Context, *GetKaxGlobal_Context, &KaxSliceLaceNumber::ClassInfos);
const EbmlSemanticContext KaxSliceFrameNumber_Context = EbmlSemanticContext(0, NULL, &KaxTimeSlice_Context, *GetKaxGlobal_Context, &KaxSliceFrameNumber::ClassInfos);
const EbmlSemanticContext KaxSliceBlockAddID_Context = EbmlSemanticContext(0, NULL, &KaxTimeSlice_Context, *GetKaxGlobal_Context, &KaxSliceBlockAddID::ClassInfos);
const EbmlSemanticContext KaxSliceDelay_Context = EbmlSemanticContext(0, NULL, &KaxTimeSlice_Context, *GetKaxGlobal_Context, &KaxSliceDelay::ClassInfos);
const EbmlSemanticContext KaxSliceDuration_Context = EbmlSemanticContext(0, NULL, &KaxTimeSlice_Context, *GetKaxGlobal_Context, &KaxSliceDuration::ClassInfos);
#if MATROSKA_VERSION >= 2
const EbmlSemanticContext KaxReferenceVirtual_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxReferenceVirtual::ClassInfos);
#endif // MATROSKA_VERSION

const EbmlCallbacks KaxReferencePriority::ClassInfos(KaxReferencePriority::Create, KaxReferencePriority_TheId, "FlagReferenced", KaxReferencePriority_Context);
const EbmlCallbacks KaxReferenceBlock::ClassInfos(KaxReferenceBlock::Create, KaxReferenceBlock_TheId, "ReferenceBlock", KaxReferenceBlock_Context);
const EbmlCallbacks KaxSlices::ClassInfos(KaxSlices::Create, KaxSlices_TheId, "Slices", KaxSlices_Context);
const EbmlCallbacks KaxTimeSlice::ClassInfos(KaxTimeSlice::Create, KaxTimeSlice_TheId, "TimeSlice", KaxTimeSlice_Context);
const EbmlCallbacks KaxSliceLaceNumber::ClassInfos(KaxSliceLaceNumber::Create, KaxSliceLaceNumber_TheId, "SliceLaceNumber", KaxSliceLaceNumber_Context);
const EbmlCallbacks KaxSliceFrameNumber::ClassInfos(KaxSliceFrameNumber::Create, KaxSliceFrameNumber_TheId, "SliceFrameNumber", KaxSliceFrameNumber_Context);
const EbmlCallbacks KaxSliceBlockAddID::ClassInfos(KaxSliceBlockAddID::Create, KaxSliceBlockAddID_TheId, "SliceBlockAddID", KaxSliceBlockAddID_Context);
const EbmlCallbacks KaxSliceDelay::ClassInfos(KaxSliceDelay::Create, KaxSliceDelay_TheId, "SliceDelay", KaxSliceDelay_Context);
const EbmlCallbacks KaxSliceDuration::ClassInfos(KaxSliceDuration::Create, KaxSliceDuration_TheId, "SliceDuration", KaxSliceDuration_Context);
#if MATROSKA_VERSION >= 2
const EbmlCallbacks KaxReferenceVirtual::ClassInfos(KaxReferenceVirtual::Create, KaxReferenceVirtual_TheId, "ReferenceVirtual", KaxReferenceVirtual_Context);
#endif // MATROSKA_VERSION

KaxSlices::KaxSlices()
 :EbmlMaster(KaxSlices_Context)
{}

KaxTimeSlice::KaxTimeSlice()
 :EbmlMaster(KaxTimeSlice_Context)
{}

const KaxBlockBlob & KaxReferenceBlock::RefBlock() const
{
	assert(RefdBlock != NULL);
	return *RefdBlock;
}

uint64 KaxReferenceBlock::UpdateSize(bool bSaveDefault, bool bForceRender)
{
	if (!bTimecodeSet) {
		assert(RefdBlock != NULL);
		assert(ParentBlock != NULL);

		const KaxInternalBlock &block = *RefdBlock;
		Value = (int64(block.GlobalTimecode()) - int64(ParentBlock->GlobalTimecode())) / int64(ParentBlock->GlobalTimecodeScale());
	}
	return EbmlSInteger::UpdateSize(bSaveDefault, bForceRender);
}

void KaxReferenceBlock::SetReferencedBlock(const KaxBlockBlob * aRefdBlock)
{
	assert(RefdBlock == NULL);
	assert(aRefdBlock != NULL);
	RefdBlock = aRefdBlock; 
	bValueIsSet = true;
}

void KaxReferenceBlock::SetReferencedBlock(const KaxBlockGroup & aRefdBlock)
{
	KaxBlockBlob *block_blob = new KaxBlockBlob(BLOCK_BLOB_NO_SIMPLE);
	block_blob->SetBlockGroup(*const_cast<KaxBlockGroup*>(&aRefdBlock));
	RefdBlock = block_blob; 
	bValueIsSet = true;
}

END_LIBMATROSKA_NAMESPACE

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
	\version \$Id: KaxClusterData.cpp 640 2004-07-09 21:05:36Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include "matroska/KaxClusterData.h"
#include "matroska/KaxContexts.h"

START_LIBMATROSKA_NAMESPACE

EbmlId KaxClusterTimecode_TheId(0xE7, 1);
EbmlId KaxClusterPrevSize_TheId(0xAB, 1);
EbmlId KaxClusterPosition_TheId(0xA7, 1);

const EbmlSemanticContext KaxClusterTimecode_Context = EbmlSemanticContext(0, NULL, &KaxCluster_Context, *GetKaxGlobal_Context, &KaxClusterTimecode::ClassInfos);
const EbmlSemanticContext KaxClusterPosition_Context = EbmlSemanticContext(0, NULL, &KaxCluster_Context, *GetKaxGlobal_Context, &KaxClusterPosition::ClassInfos);
const EbmlSemanticContext KaxClusterPrevSize_Context = EbmlSemanticContext(0, NULL, &KaxCluster_Context, *GetKaxGlobal_Context, &KaxClusterPrevSize::ClassInfos);

const EbmlCallbacks KaxClusterTimecode::ClassInfos(KaxClusterTimecode::Create, KaxClusterTimecode_TheId, "ClusterTimecode", KaxClusterTimecode_Context);
const EbmlCallbacks KaxClusterPrevSize::ClassInfos(KaxClusterPrevSize::Create, KaxClusterPrevSize_TheId, "ClusterPrevSize", KaxClusterPrevSize_Context);
const EbmlCallbacks KaxClusterPosition::ClassInfos(KaxClusterPosition::Create, KaxClusterPosition_TheId, "ClusterPosition", KaxClusterPosition_Context);

END_LIBMATROSKA_NAMESPACE

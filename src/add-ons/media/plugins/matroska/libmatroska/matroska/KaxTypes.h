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
	\version \$Id: KaxTypes.h,v 1.4 2004/04/14 23:26:17 robux4 Exp $
*/
#ifndef LIBMATROSKA_TYPES_H
#define LIBMATROSKA_TYPES_H

#include "matroska/KaxConfig.h"
#include "ebml/EbmlTypes.h"
#include "matroska/c/libmatroska_t.h"

START_LIBMATROSKA_NAMESPACE

enum LacingType {
	LACING_NONE = 0,
	LACING_XIPH,
	LACING_FIXED,
	LACING_EBML,
	LACING_AUTO
};

enum BlockBlobType {
	BLOCK_BLOB_NO_SIMPLE = 0,
	BLOCK_BLOB_SIMPLE_AUTO,
	BLOCK_BLOB_ALWAYS_SIMPLE,
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_TYPES_H

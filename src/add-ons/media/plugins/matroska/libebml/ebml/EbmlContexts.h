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
	\version \$Id: EbmlContexts.h 736 2004-08-28 14:05:09Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_CONTEXTS_H
#define LIBEBML_CONTEXTS_H

#include "EbmlTypes.h"
#include "EbmlElement.h"

START_LIBEBML_NAMESPACE

extern const EbmlSemanticContext EBML_DLL_API EbmlHead_Context;
extern const EbmlSemanticContext EBML_DLL_API EVersion_Context;
extern const EbmlSemanticContext EBML_DLL_API EReadVersion_Context;
extern const EbmlSemanticContext EBML_DLL_API EMaxIdLength_Context;
extern const EbmlSemanticContext EBML_DLL_API EMaxSizeLength_Context;
extern const EbmlSemanticContext EBML_DLL_API EDocType_Context;
extern const EbmlSemanticContext EBML_DLL_API EDocTypeVersion_Context;
extern const EbmlSemanticContext EBML_DLL_API EDocTypeReadVersion_Context;

// global elements
extern const EbmlSemanticContext EBML_DLL_API EbmlVoid_Context;
extern const EbmlSemanticContext EBML_DLL_API & GetEbmlGlobal_Context();
	
END_LIBEBML_NAMESPACE

#endif // LIBEBML_CONTEXTS_H

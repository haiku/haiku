/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: EbmlString.h 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_STRING_H
#define LIBEBML_STRING_H

#include <string>

#include "EbmlTypes.h"
#include "EbmlElement.h"

START_LIBEBML_NAMESPACE

/*!
    \class EbmlString
    \brief Handle all operations on a printable string EBML element
*/
class EBML_DLL_API EbmlString : public EbmlElement {
	public:
		EbmlString();
		EbmlString(const std::string & aDefaultValue);
		EbmlString(const EbmlString & ElementToClone);
	
		virtual ~EbmlString() {}
	
		bool ValidateSize() const {return true;} // any size is possible
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);
		uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false);
	
		EbmlString & operator=(const std::string);
		operator const std::string &() const {return Value;}
	
		void SetDefaultValue(std::string & aValue) {assert(!DefaultIsSet); DefaultValue = aValue; DefaultIsSet = true;}
    
		const std::string DefaultVal() const {assert(DefaultIsSet); return DefaultValue;}

		bool IsDefaultValue() const {
			return (DefaultISset() && Value == DefaultValue);
		}

	protected:
		std::string Value;  /// The actual value of the element
		std::string DefaultValue;
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_STRING_H

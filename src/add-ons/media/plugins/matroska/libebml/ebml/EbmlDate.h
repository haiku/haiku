/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: EbmlDate.h 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_DATE_H
#define LIBEBML_DATE_H

#include "EbmlTypes.h"
#include "EbmlElement.h"

START_LIBEBML_NAMESPACE

/*!
    \class EbmlDate
    \brief Handle all operations related to an EBML date
*/
class EBML_DLL_API EbmlDate : public EbmlElement {
	public:
		EbmlDate() :EbmlElement(8, false), myDate(0) {}
		EbmlDate(const EbmlDate & ElementToClone);
		
		/*!
			\brief set the date with a UNIX/C/EPOCH form
			\param NewDate UNIX/C date in UTC (no timezone)
		*/
		void SetEpochDate(int32 NewDate) {bValueIsSet = true; myDate = int64(NewDate - UnixEpochDelay) * 1000000000; bValueIsSet = true;}

		/*!
			\brief get the date with a UNIX/C/EPOCH form
			\note the date is in UTC (no timezone)
		*/
		int32 GetEpochDate() const {return int32(myDate/1000000000 + UnixEpochDelay);}
	
		bool ValidateSize() const {return ((Size == 8) || (Size == 0));}

		/*!
			\note no Default date handled
		*/
		uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false) {
			if(!bValueIsSet) 
				Size = 0;
			else
				Size = 8;
			return Size;
		}
		
		bool operator<(const EbmlDate & EltCmp) const {return myDate < EltCmp.myDate;}
		
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);

		bool IsDefaultValue() const {
			return false;
		}

	protected:
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		
		int64 myDate; ///< internal format of the date
	
		static const uint64 UnixEpochDelay;
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_DATE_H

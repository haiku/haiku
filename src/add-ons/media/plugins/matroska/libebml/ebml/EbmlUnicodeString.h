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
	\version \$Id: EbmlUnicodeString.h 1079 2005-03-03 13:18:14Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
	\author Jory Stone       <jcsston @ toughguy.net>
*/
#ifndef LIBEBML_UNICODE_STRING_H
#define LIBEBML_UNICODE_STRING_H

#include <string>

#include "EbmlTypes.h"
#include "EbmlElement.h"

START_LIBEBML_NAMESPACE

/*!
  \class UTFstring
  A class storing strings in a wchar_t (ie, in UCS-2 or UCS-4)
  \note inspired by wstring which is not available everywhere
*/
class EBML_DLL_API UTFstring {
public:
	typedef wchar_t value_type;

	UTFstring();
	UTFstring(const wchar_t *); // should be NULL terminated
	UTFstring(const UTFstring &);
	
	virtual ~UTFstring();
	bool operator==(const UTFstring&) const;
	inline bool operator!=(const UTFstring &cmp) const
	{
		return !(*this == cmp);
	}
	UTFstring & operator=(const UTFstring &);
	UTFstring & operator=(const wchar_t *);
	UTFstring & operator=(wchar_t);

	/// Return length of string
	size_t length() const {return _Length;}

	operator const wchar_t*() const {return _Data;}
	const wchar_t* c_str() const {return _Data;}

	const std::string & GetUTF8() const {return UTF8string;}
	void SetUTF8(const std::string &);

protected:
	size_t _Length; ///< length of the UCS string excluding the \0
	wchar_t* _Data; ///< internal UCS representation	
	std::string UTF8string;
	static bool wcscmp_internal(const wchar_t *str1, const wchar_t *str2);
	void UpdateFromUTF8();
	void UpdateFromUCS2();
};


/*!
    \class EbmlUnicodeString
    \brief Handle all operations on a Unicode string EBML element
	\note internally treated as a string made of wide characters (ie UCS-2 or UCS-4 depending on the machine)
*/
class EBML_DLL_API EbmlUnicodeString : public EbmlElement {
	public:
		EbmlUnicodeString();
		EbmlUnicodeString(const UTFstring & DefaultValue);
		EbmlUnicodeString(const EbmlUnicodeString & ElementToClone);
	
		virtual ~EbmlUnicodeString() {}
	
		bool ValidateSize() const {return true;} // any size is possible
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);
		uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false);
	
		EbmlUnicodeString & operator=(const UTFstring &); ///< platform dependant code
		operator const UTFstring &() const {return Value;}
	
		void SetDefaultValue(UTFstring & aValue) {assert(!DefaultIsSet); DefaultValue = aValue; DefaultIsSet = true;}
    
		UTFstring DefaultVal() const {assert(DefaultIsSet); return DefaultValue;}

		bool IsDefaultValue() const {
			return (DefaultISset() && Value == DefaultValue);
		}

	protected:
		UTFstring Value; /// The actual value of the element
		UTFstring DefaultValue;
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_UNICODE_STRING_H

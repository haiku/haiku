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
	\version \$Id: Debug.h 639 2004-07-09 20:59:14Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#ifndef LIBEBML_DEBUG_H
#define LIBEBML_DEBUG_H

#include <stdarg.h> // va_list
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <stdio.h>
#endif // WIN32

#include "EbmlConfig.h"

START_LIBEBML_NAMESPACE

static const int MAX_PREFIX_LENGTH = 128;

#if !defined(NDEBUG)
// define the working debugging class

class EBML_DLL_API ADbg  
{
public:
	ADbg(int level = 0);
	virtual ~ADbg();

	/// \todo make an inline function to test the level first and the process
	int OutPut(int level, const char * format,...) const;

	int OutPut(const char * format,...) const;

	inline int setLevel(const int level) {
		return my_level = level;
	}

	inline bool setIncludeTime(const bool included = true) {
		return my_time_included = included;
	}

	bool setDebugFile(const char * NewFilename);
	bool unsetDebugFile();

	inline bool setUseFile(const bool usefile = true) {
		return my_use_file = usefile;
	}

	inline const char * setPrefix(const char * string) {
		return strncpy(prefix, string, MAX_PREFIX_LENGTH);
	}

private:
	int my_level;
	bool my_time_included;
	bool my_use_file;
	bool my_debug_output;

	int _OutPut(const char * format,va_list params) const;

	char prefix[MAX_PREFIX_LENGTH];

#ifdef WIN32
	HANDLE hFile;
#else
	FILE *hFile;
#endif // WIN32
};

#else // !defined(NDEBUG)

// define a class that does nothing (no output)

class EBML_DLL_API ADbg
{
public:
	ADbg(int level = 0){}
	virtual ~ADbg() {}

	inline int OutPut(int level, const char * format,...) const {
		return 0;
	}

	inline int OutPut(const char * format,...) const {
		return 0;
	}

	inline int setLevel(const int level) {
		return level;
	}

	inline bool setIncludeTime(const bool included = true) {
		return true;
	}

	inline bool setDebugFile(const char * NewFilename) {
		return true;
	}

	inline bool unsetDebugFile() {
		return true;
	}

	inline bool setUseFile(const bool usefile = true) {
		return true;
	}

	inline const char * setPrefix(const char * string) {
		return string;
	}
};

#endif // !defined(NDEBUG)

extern class EBML_DLL_API ADbg globalDebug;


#ifdef LIBEBML_DEBUG
#define EBML_TRACE globalDebug.OutPut
#else
#define EBML_TRACE
#endif

// Unfortunately the Visual C++ new operator doesn't throw a std::bad_alloc. One solution is to
// define out own new operator. But we can't do this globally, since we allow static linking.
// The other is to check every new allocation with an MATROSKA_ASSERT_NEW.
#ifdef _MSC_VER
#define EBML_ASSERT_NEW(p) if(p==0)throw std::bad_alloc()
#else
#define EBML_ASSERT_NEW(p) assert(p!=0)
#endif

END_LIBEBML_NAMESPACE

#endif

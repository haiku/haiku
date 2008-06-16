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
	\version \$Id: Debug.cpp 1268 2007-01-19 10:15:08Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#include <stdio.h>

#ifdef WIN32
#include <windows.h> // For OutputDebugString
#else
#include <time.h>
#include <sys/time.h>
#endif // WIN32

#include "ebml/Debug.h"

START_LIBEBML_NAMESPACE

class ADbg globalDebug;

#if !defined(NDEBUG)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ADbg::ADbg(int level)
:my_level(level)
,my_time_included(false)
,my_use_file(false)
,my_debug_output(true)
,hFile(NULL)
{
	prefix[0] = '\0';
	OutPut(-1,"ADbg Creation at debug level = %d (0x%08X)",my_level,this);
}

ADbg::~ADbg()
{
	unsetDebugFile();
	OutPut(-1,"ADbg Deletion (0x%08X)",this);
}

inline int ADbg::_OutPut(const char * format,va_list params) const
{
	int result;

	char tst[1000];
	char myformat[256];

#ifdef WIN32
	if (my_time_included) {
		SYSTEMTIME time;
		GetSystemTime(&time);
		if (prefix[0] == '\0')
			wsprintfA(myformat,"%04d/%02d/%02d %02d:%02d:%02d.%03d UTC : %s\r\n",
							time.wYear,
							time.wMonth,
							time.wDay,
							time.wHour,
							time.wMinute,
							time.wSecond,
							time.wMilliseconds,
							format);
		else
			wsprintfA(myformat,"%04d/%02d/%02d %02d:%02d:%02d.%03d UTC : %s - %s\r\n",
							time.wYear,
							time.wMonth,
							time.wDay,
							time.wHour,
							time.wMinute,
							time.wSecond,
							time.wMilliseconds,
							prefix,
							format);
	} else {
		if (prefix[0] == '\0')
			wsprintfA( myformat, "%s\r\n", format);
		else
			wsprintfA( myformat, "%s - %s\r\n", prefix, format);
	}
	result = vsprintf(tst,myformat,params);
	
	if (my_debug_output)
		OutputDebugStringA(tst);

	if (my_use_file && (hFile != NULL)) {
		SetFilePointer( hFile, 0, 0, FILE_END );
		DWORD written;
		WriteFile( hFile, tst, lstrlenA(tst), &written, NULL );
	}
#else
	if (my_time_included) {
		time_t nowSecs;
		struct tm *now;
		struct timeval tv;
		
		nowSecs = time(NULL);
		gettimeofday(&tv, NULL);
		now = gmtime(&nowSecs);
		if (prefix[0] == '\0')
			sprintf(myformat,"%04d/%02d/%02d %02d:%02d:%02ld.%03ld UTC : %s\r\n",
					now->tm_year, now->tm_mon, now->tm_mday,
					now->tm_hour, now->tm_min, tv.tv_sec,
					(long)tv.tv_usec / 1000, format);
		else
			sprintf(myformat,"%04d/%02d/%02d %02d:%02d:%02ld.%03ld UTC : %s - %s\r\n",
					now->tm_year, now->tm_mon, now->tm_mday,
					now->tm_hour, now->tm_min, tv.tv_sec,
					(long)tv.tv_usec / 1000, prefix, format);
		
	} else {
		if (prefix[0] == '\0')
			sprintf( myformat, "%s\r\n", format);
		else
			sprintf( myformat, "%s - %s\r\n", prefix, format);
	}

	result = vsprintf(tst,myformat,params);
	
	if (my_debug_output)
		fputs(tst, stderr);

	if (my_use_file && (hFile != NULL))
		fputs(tst, hFile);
#endif

	return result;
}

int ADbg::OutPut(int forLevel, const char * format,...) const
{
	int result=0;
	
	if (forLevel >= my_level) {
		va_list tstlist;

		va_start(tstlist, format);

		result = _OutPut(format,tstlist);

	}

	return result;
}

int ADbg::OutPut(const char * format,...) const
{
	va_list tstlist;

	va_start(tstlist, format);

	return _OutPut(format,tstlist);
}

bool ADbg::setDebugFile(const char * NewFilename) {
	bool result;
	result = unsetDebugFile();

	if (result) {
		result = false;

#ifdef WIN32
		hFile = CreateFileA(NewFilename, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		
		if (hFile != INVALID_HANDLE_VALUE) {
			SetFilePointer( hFile, 0, 0, FILE_END );

			result = true;
#else
		hFile = fopen(NewFilename, "w+");
		if (hFile != NULL) {
			fseek(hFile, 0, SEEK_END);
#endif
			OutPut(-1,"Debug hFile Opening succeeded");

		}
		else
			OutPut(-1,"Debug hFile %s Opening failed",NewFilename);
	}

	return result;
}

bool ADbg::unsetDebugFile() {
	bool result = (hFile == NULL);
	
#ifdef WIN32
	if (hFile != NULL) {
		result = (CloseHandle(hFile) != 0);
#else
	if (hFile != NULL) {
		result = (fclose(hFile) == 0);
#endif

		if (result) {
			OutPut(-1,"Debug hFile Closing succeeded");
			hFile = NULL;
		}
	}
	return result;
}

#endif // !defined(NDEBUG)

END_LIBEBML_NAMESPACE

/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Ingo Ralf Blum.  All rights reserved.
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
	\version \$Id: IOCallback.cpp 639 2004-07-09 20:59:14Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Moritz Bunkus <moritz @ bunkus.org>
*/

#if !defined(__GNUC__) || (__GNUC__ > 2)
#include <sstream>
#endif // GCC2
#include <stdexcept>


#include "ebml/IOCallback.h"

using namespace std;

START_LIBEBML_NAMESPACE

void IOCallback::writeFully(const void*Buffer,size_t Size)
{
	if (Size == 0)
		return;

	if (Buffer == NULL)
		throw;

	if(write(Buffer,Size) != Size)
	{
#if !defined(__GNUC__) || (__GNUC__ > 2)
		stringstream Msg;
		Msg<<"EOF in writeFully("<<Buffer<<","<<Size<<")";
		throw runtime_error(Msg.str());
#endif // GCC2
	}
}



void IOCallback::readFully(void*Buffer,size_t Size)
{
	if(Buffer == NULL)
		throw;

	if(read(Buffer,Size) != Size)
	{
#if !defined(__GNUC__) || (__GNUC__ > 2)
		stringstream Msg;
		Msg<<"EOF in readFully("<<Buffer<<","<<Size<<")";
		throw runtime_error(Msg.str());
#endif // GCC2
	}
}

END_LIBEBML_NAMESPACE

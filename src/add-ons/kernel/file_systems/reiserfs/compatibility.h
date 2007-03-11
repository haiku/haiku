// compatibility.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef REISERFS_COMPATIBILITY_H
#define REISERFS_COMPATIBILITY_H

#include <BeBuild.h>

#if B_BEOS_VERSION <= B_BEOS_VERSION_5
//#	define B_BAD_DATA -2147483632L
#else
#	ifndef closesocket
#		define closesocket(fd)	close(fd)
#	endif
#	define _IMPEXP_KERNEL
#endif

// a Haiku definition
#ifndef B_BUFFER_OVERFLOW
#	define B_BUFFER_OVERFLOW	EOVERFLOW
#endif

// make Zeta R5 source compatible without needing to link against libzeta.so
#ifdef find_directory
#	undef find_directory
#endif

#endif	// REISERFS_COMPATIBILITY_H

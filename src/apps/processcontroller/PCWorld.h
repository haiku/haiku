/*
	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _PCWORLD_H_
#define _PCWORLD_H_


#include <OS.h>


extern const char* kSignature;
extern const char* kTrackerSig;
extern const char* kDeskbarSig;
extern const char* kTerminalSig;

extern const char* kPosPrefName;
extern const char* kPreferencesFileName;

extern const char* kVersionName;
extern const int kCurrentVersion;


extern const char* kProgramName;
extern const char* kPCSemaphoreName;

extern thread_id id;


#endif // _PCWORLD_H_

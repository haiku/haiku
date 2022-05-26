//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RecentApps.h
//	Author:			Tyler Dauwalder (tyler@dauwalder.net)
//	Description:	Recently launched apps list
//------------------------------------------------------------------------------
/*! \file RecentApps.h
	\brief RecentApps class declarations
*/

#ifndef RECENT_APPS_H
#define RECENT_APPS_H

#include <SupportDefs.h>

#include <list>
#include <stdio.h>
#include <string>

class BMessage;
class TRoster;
struct entry_ref;


class RecentApps {
public:
	RecentApps();
	~RecentApps();
	
	status_t Add(const char *appSig, int32 appFlags = kQualifyingAppFlags);
	status_t Add(const entry_ref *ref, int32 appFlags);
	status_t Get(int32 maxCount, BMessage *list);
	status_t Clear();
	status_t Print();
	status_t Save(FILE* file);
	
	static const int32 kQualifyingAppFlags = 0;
private:
	friend class TRoster;
		// For loading from disk

	static status_t GetRefForApp(const char *appSig, entry_ref *result);	

	std::list<std::string> fAppList;
};

#endif	// RECENT_APPS_H

// String.h
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

#ifndef STRING_H
#define STRING_H

#include <SupportDefs.h>

#ifdef __cplusplus

class String {
public:
	String();
	String(const String &string);
	String(const char *string, int32 length = -1);
	~String();

	bool SetTo(const char *string, int32 maxLength = -1);
	void Unset();

	const char *GetString() const;
	int32 GetLength() const { return fLength; }

	String &operator=(const String &string);
	bool operator==(const String &string) const;
	bool operator!=(const String &string) const { return !(*this == string); }

private:
	bool _SetTo(const char *string, int32 length);

private:
	int32	fLength;
	char	*fString;
};

#endif	// __cplusplus

#endif	// STRING_H

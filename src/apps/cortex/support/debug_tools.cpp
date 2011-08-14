/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// debug_tools.cpp

#include "debug_tools.h"

#include <stdlib.h>
#include <Alert.h>
#include <stdarg.h>

#include <strstream>

using namespace std;

string point_to_string(const BPoint& p) {
	strstream out;
	out << '(' << p.x << ',' << p.y << ')';
	return out.str();
}

string rect_to_string(const BRect& r) {
	strstream out;
	out << '(' << r.left << ',' << r.top <<
		")-(" << r.right << ',' << r.bottom << ')';
	return out.str();
}

string id_to_string(uint32 nId) {
	string out;
	out += (char)(nId >> 24);
	out += (char)(nId >> 16) & 0xff;
	out += (char)(nId >> 8) & 0xff;
	out += (char)nId & 0xff;
	return out;
}


void show_alert(char* pFormat, ...) {
	// +++++
}


// END -- debug_tools.h --

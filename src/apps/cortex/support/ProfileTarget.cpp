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


// ProfileTarget.cpp

#include "ProfileTarget.h"
#include <cstdio>
#include <cstring>
#include <list>
#include <algorithm>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ProfileTarget::~ProfileTarget() {}
ProfileTarget::ProfileTarget() {}
	
// -------------------------------------------------------- //
// user operations
// -------------------------------------------------------- //

void ProfileTarget::clear() {
	m_blockEntryMap.clear();
}

// [e.moon 14oct99] moved prototype out of header
bool operator<(const ProfileTarget::block_entry& a, const ProfileTarget::block_entry& b);

bool operator<(const ProfileTarget::block_entry& a, const ProfileTarget::block_entry& b) {
	return b.elapsed < a.elapsed;
}

class fnDumpEntry { public:
	uint32 nameLength;
	BString maxPad;
	fnDumpEntry(uint32 _n) : nameLength(_n) {
		maxPad.SetTo(' ', nameLength);
		fprintf(stderr, 
			"  BLOCK%s COUNT         ELAPSED        AVERAGE\n"
			"  ----------------------------------------------------------------------------\n",
			maxPad.String());
	}
	void operator()(ProfileTarget::block_entry& entry) const {
		BString namePad;
		namePad.SetTo(' ', nameLength-strlen(entry.name));
		fprintf(stderr, 
			"  %s:%s  %8ld        %8Ld        %.4f\n",
			entry.name,
			namePad.String(),
			entry.count,
			entry.elapsed,
			(float)entry.elapsed/entry.count);
	}
};


void ProfileTarget::dump() const {
	fprintf(stderr, "\nProfileTarget::dump()\n\n");

	list<block_entry> sorted;
	uint32 nameLength = 0;
		
	for(block_entry_map::const_iterator it = m_blockEntryMap.begin();
		it != m_blockEntryMap.end();
		it++) {
		if((*it).first.Length() > nameLength)
			nameLength = (*it).first.Length();
		sorted.push_back(block_entry());
		sorted.back() = (*it).second;
		sorted.back().name = (*it).first.String();
	}
	
	sorted.sort();
	for_each(sorted.begin(), sorted.end(), fnDumpEntry(nameLength));
}

// -------------------------------------------------------- //
// profile-source operations
// -------------------------------------------------------- //

inline void ProfileTarget::addBlockEntry(
	const char* blockName, bigtime_t elapsed) {

	block_entry& e = m_blockEntryMap[blockName];
	e.count++;
	e.elapsed += elapsed;	
}

// END -- ProfileTarget.cpp --


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


// ProfileTarget.h
// e.moon 19may99
//
// A bare-bones profiling tool.

#ifndef __EMOON_PROFILETARGET_H__
#define __EMOON_PROFILETARGET_H__

#include <map>

#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ProfileTarget {
public:					// ctor/dtor
	virtual ~ProfileTarget();
	ProfileTarget();
	
public:					// user operations
	void clear();
	void dump() const;

public:					// profile-source operations
	inline void addBlockEntry(const char* blockName, bigtime_t elapsed);

public:
	struct block_entry {
		block_entry() : elapsed(0), count(0), name(0) {}
		
		bigtime_t		elapsed;
		uint32			count;
		const char*		name;
	};
	
private:					// implementation
			
	typedef map<BString, block_entry> block_entry_map;
	block_entry_map		m_blockEntryMap;
};

//bool operator<(const ProfileTarget::block_entry& a, const ProfileTarget::block_entry& b);

__END_CORTEX_NAMESPACE
#endif /* __EMOON_PROFILETARGET_H__ */

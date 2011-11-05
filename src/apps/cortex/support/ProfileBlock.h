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


// ProfileBlock.h
// e.moon 19may99
//
// Quick'n'dirty profiling tool: create a ProfileBlock on the
// stack, giving it a name and a ProfileTarget to reference.
// Automatically writes an entry in the target once the
// object goes out of scope.

#ifndef __PROFILEBLOCK_H__
#define __PROFILEBLOCK_H__

#include "ProfileTarget.h"
#include <KernelKit.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ProfileBlock {
public:					// ctor/dtor
	ProfileBlock(ProfileTarget& target, const char* name) :
		m_target(target),
		m_name(name),
		m_initTime(system_time()) {}
		
	~ProfileBlock() {
		m_target.addBlockEntry(m_name, system_time()-m_initTime);
	}
	
private:
	ProfileTarget&		m_target;
	const char* const		m_name;
	bigtime_t					m_initTime;
};

__END_CORTEX_NAMESPACE
#endif /* __PROFILEBLOCK_H__ */

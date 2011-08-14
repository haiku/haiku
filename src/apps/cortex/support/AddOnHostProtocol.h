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


// AddOnHostProtocol.h
// * PURPOSE
//   contains all definitions needed for communications between
//   cortex/route and the add-on host app.
// * HISTORY
//   e.moon  02apr00   begun

#if !defined(__CORTEX_ADD_ON_HOST_PROTOCOL_H__)
#define __CORTEX_ADD_ON_HOST_PROTOCOL_H__

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE
namespace addon_host {

const char* const g_appSignature = "application/x-vnd.Cortex.AddOnHost";

enum message_t {
	// inbound (sends reply)
	// 'info' B_RAW_TYPE (dormant_node_info)
	M_INSTANTIATE							= AddOnHostApp_message_base,

	// outbound
	// 'node_id' int32 (media_node_id)
	M_INSTANTIATE_COMPLETE,
	// 'error' int32
	M_INSTANTIATE_FAILED,
	
	// inbound (sends reply)
	// 'info' B_RAW_TYPE (live_node_info)
	M_RELEASE,
	
	// outbound
	// 'node_id' int32
	M_RELEASE_COMPLETE,
	// 'error' int32
	M_RELEASE_FAILED
};

}; // addon_host
__END_CORTEX_NAMESPACE
#endif //__CORTEX_ADD_ON_HOST_PROTOCOL_H__


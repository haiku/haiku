/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_UTILS__H
#define _K_PPP_UTILS__H

#ifndef _K_PPP_DEFS__H
#include <KPPPDefs.h>
#endif

#include <OS.h>

class KPPPProtocol;


// helper functions
extern bool IsProtocolAllowed(const KPPPProtocol& protocol);

// the list template does not support iterating over each item :(
// this template iterates over each item in an indexed list
template<class _LIST, class _FUNCTION>
inline
void
ForEachItem(_LIST& list, _FUNCTION function)
{
	for(int index = 0; index < list.CountItems(); index++)
		function(list.ItemAt(index));
}


// These are very simple send/receive_data functions with a timeout
// and there is a race condition beween has_data() and send/receive_data().
// Timeouts in ms.
extern status_t send_data_with_timeout(thread_id thread, int32 code, void *buffer,
	size_t buffer_size, uint32 timeout);
extern status_t receive_data_with_timeout(thread_id *sender, int32 *code,
	void *buffer, size_t buffer_size, uint32 timeout);


#endif

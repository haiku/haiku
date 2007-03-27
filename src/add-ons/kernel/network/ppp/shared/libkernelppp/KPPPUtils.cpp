/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <KPPPUtils.h>
#include <KPPPInterface.h>


//!	Returns if this protocol is allowed to handle packets in the current state.
bool
IsProtocolAllowed(const KPPPProtocol& protocol)
{
	if (protocol.ProtocolNumber() == PPP_LCP_PROTOCOL)
		return true;
	else if (protocol.Interface().State() != PPP_OPENED_STATE)
		return false;
	else if (protocol.Interface().Phase() > PPP_AUTHENTICATION_PHASE
			|| (protocol.Interface().Phase() >= PPP_ESTABLISHMENT_PHASE
				&& protocol.Flags() & PPP_ALWAYS_ALLOWED))
		return true;
	else
		return false;
}


status_t
send_data_with_timeout(thread_id thread, int32 code, void *buffer,
	size_t buffer_size, uint32 timeout)
{
	for (uint32 tries = 0; tries < timeout; tries += 5) {
		if (has_data(thread))
			snooze(5000);
		else
			break;
	}
	
	if (!has_data(thread))
		return send_data(thread, code, buffer, buffer_size);
	else
		return B_TIMED_OUT;
}


status_t
receive_data_with_timeout(thread_id *sender, int32 *code, void *buffer,
	size_t buffer_size, uint32 timeout)
{
	thread_id me = find_thread(NULL);
	for (uint32 tries = 0; tries < timeout; tries += 5) {
		if (!has_data(me))
			snooze(5000);
		else
			break;
	}
	
	if (has_data(find_thread(NULL))) {
		*code = receive_data(sender, buffer, buffer_size);
		return B_OK;
	} else
		return B_TIMED_OUT;
}

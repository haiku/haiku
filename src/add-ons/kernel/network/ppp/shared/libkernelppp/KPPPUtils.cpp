//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <OS.h>

#include <KPPPUtils.h>


// These are very simple send/receive_data functions with a timeout
// and there is a race condition beween has_data() and send/receive_data().
// Timeouts in ms.
status_t
send_data_with_timeout(thread_id thread, int32 code, void *buffer,
	size_t buffer_size, uint32 timeout)
{
	for(uint32 tries = 0; tries < timeout; tries++) {
		if(has_data(thread))
			snooze(1000);
	}
	
	if(!has_data(thread))
		return send_data(thread, code, buffer, buffer_size);
	else
		return B_TIMED_OUT;
}


status_t
receive_data_with_timeout(thread_id *sender, int32 *code, void *buffer,
	size_t buffer_size, uint32 timeout)
{
	for(uint32 tries = 0; tries < timeout; tries++) {
		if(!has_data(find_thread(NULL))) {
			snooze(1000);
			continue;
		}
	}
	
	if(has_data(find_thread(NULL))) {
		*code = receive_data(sender, buffer, buffer_size);
		return B_OK;
	} else
		return B_TIMED_OUT;
}

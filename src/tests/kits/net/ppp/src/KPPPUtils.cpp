#include "KPPPUtils.h"

#include <OS.h>


// These are very simple send/receive_data functions with a timeout
// and there is a race condition beween has_data() and send/receive_data().
// Timeouts in ms.
status_t
send_data_with_timeout(thread_id thread, int32 code, void *buffer,
	size_t buffer_size, uint32 timeout)
{
	int32 tries;
	
	for(tries = 0; tries < timeout; tries++) {
		if(has_data(thread))
			snooze(1000);
		else
			return send_data(thread, code, buffer, buffer_size);
	}
	
	return B_TIMED_OUT;
}


status_t
receive_data_with_timeout(thread_id *sender, int32 *code, void *buffer,
	size_t buffer_size, uint32 timeout)
{
	int32 tries;
	
	for(tries = 0; tries < timeout; tries++) {
		if(has_data(find_thread(NULL))) {
			snooze(1000);
			continue;
		}
		
		*code = receive_data(sender, buffer, buffer_size);
		return B_OK;
	}
	
	return B_TIMED_OUT;
}

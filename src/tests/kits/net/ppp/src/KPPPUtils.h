//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_UTILS__H
#define _K_PPP_UTILS__H


// These are very simple send/receive_data functions with a timeout
// and there is a race condition beween has_data() and send/receive_data().
// Timeouts in ms.
status_t send_data_with_timeout(thread_id thread, int32 code, void *buffer,
	size_t buffer_size, uint32 timeout);
status_t receive_data_with_timeout(thread_id *sender, int32 *code, void *buffer,
	size_t buffer_size, uint32 timeout);


#endif

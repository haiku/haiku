//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_MODULE__H
#define _K_PPP_MODULE__H

#include <module.h>

class PPPInterface;


typedef struct ppp_module_info {
	module_info minfo;
	status_t (*control)(uint32 op, void *data, size_t length);
	bool (*add_to)(PPPInterface& mainInterface, PPPInterface *subInterface,
		driver_parameter *settings, ppp_module_key_type type);
			// multilink: handlers that must run on a real device
			// should be added to subInterface (may be NULL)
			// while mainInterface handlers are used for the
			// bundle of interfaces
} ppp_module_info;


#endif

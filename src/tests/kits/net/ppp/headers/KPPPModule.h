#ifndef _K_PPP_MODULE__H
#define _K_PPP_MODULE__H

#include <module.h>

class PPPInterface;


typedef struct ppp_module_info {
	module_info minfo;
	status_t (*control)(uint32 op, void *data, size_t length);
	status_t (*add_to)(PPPInterface *interface, driver_parameter *settings, int32 type);
} ppp_module_info;


#endif

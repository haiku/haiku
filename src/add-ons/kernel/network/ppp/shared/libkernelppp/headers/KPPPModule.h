/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_MODULE__H
#define _K_PPP_MODULE__H

#include <module.h>

class KPPPInterface;


//!	This structure is exported by PPP kernel modules.
typedef struct ppp_module_info {
	module_info minfo;
		//!< Default kernel module structure.
	
	//!	You may define this \e optional function if you want to export a private API.
	status_t (*control)(uint32 op, void *data, size_t length);
	
	/*!	\brief Signals your module to add its handlers to the PPP interface.
		
		This function \e must be exported.
		
		Multilink-only: Handlers that must run on a bundle's child should be
		added to \a subInterface while \a mainInterface handlers are used for the
		bundle interfaces. For example, devices must be added to every child
		individually because every \a subInterface establishes its own connection
		while the IP Control Protocol is added to the \a mainInterface because
		it is shared by all interfaces of the bundle.
		
		\param mainInterface The multilink bundle or (if subInterface == NULL) the
				actual interface object.
		\param subInterface If defined, this is a child interface of a bundle.
		\param settings The settings for your module.
		\param type Specifies which key caused loading the module. This would be
				\c PPP_AUTHENTICATOR_KEY_TYPE for "authenticator". You should check
				if \a type is correct (e.g.: loading an authenticator with "device"
				should fail).
		
		\return
			\c true: Modules could be added successfully.
	*/
	bool (*add_to)(KPPPInterface& mainInterface, KPPPInterface *subInterface,
		driver_parameter *settings, ppp_module_key_type type);
} ppp_module_info;


#endif

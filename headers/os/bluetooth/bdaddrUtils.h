/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */
 
#ifndef _BDADDR_UTILS_H
#define _BDADDR_UTILS_H

#include <bluetooth/bluetooth.h>

namespace Bluetooth {

class bdaddrUtils {
   
       public:
		static inline bdaddr_t NullAddress() 
		{
		
			return ((bdaddr_t) {{0, 0, 0, 0, 0, 0}});
		}
		
		
		static inline bdaddr_t LocalAddress() 
		{
		
			return ((bdaddr_t) {{0, 0, 0, 0xff, 0xff, 0xff}});
		}
		
		
		static inline bdaddr_t BroadcastAddress() 
		{
		
			return ((bdaddr_t) {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}});
		}
		
		
		static const char* ToString(bdaddr_t bdaddr) 
		{
			// TODO: 
			static char str[12];
			
			return str;
		}
			
};

}

#endif

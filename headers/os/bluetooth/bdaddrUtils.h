/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */
 
#ifndef _BDADDR_UTILS_H
#define _BDADDR_UTILS_H

#include <stdio.h>

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
		
		
		static bool Compare(bdaddr_t* ba1, bdaddr_t* ba2)
		{
			return (memcmp(ba1, ba2, sizeof(bdaddr_t)) == 0);
		}
		
		
		static char* ToString(const bdaddr_t bdaddr) 
		{
			// TODO: not safe
			static char str[18];
			
			sprintf(str,"%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",bdaddr.b[0],
															bdaddr.b[1],
															bdaddr.b[2],
															bdaddr.b[3],
															bdaddr.b[4],
															bdaddr.b[5]);
															
			return str;
		}

		static bdaddr_t FromString(const char * addr)
		{
			int b0, b1, b2, b3, b4, b5;
			size_t count = sscanf(addr, "%2X:%2X:%2X:%2X:%2X:%2X",
						&b0,
						&b1,
						&b2,
						&b3,
						&b4,
						&b5);

			if (count == 6)
				return ((bdaddr_t) {{b0, b1, b2, b3, b4, b5}});

			return NullAddress();
		}	

};

}

#ifndef _BT_USE_EXPLICIT_NAMESPACE
using Bluetooth::bdaddrUtils;
#endif


#endif

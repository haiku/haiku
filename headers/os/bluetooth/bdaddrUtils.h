/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 */

#ifndef _BDADDR_UTILS_H
#define _BDADDR_UTILS_H


namespace Bluetooth {

	class bdaddrUtils {
    
        public:
			static inline bdaddr_t nullAddress() {
			
				return ((bdaddr_t) {{0, 0, 0, 0, 0, 0}});
			}
			static inline bdaddr_t localAddress() {
			
				return ((bdaddr_t) {{0, 0, 0, 0xff, 0xff, 0xff}});
			}
			
			static inline bdaddr_t broadcastAddress() {
			
				return ((bdaddr_t) {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}});
			}
			
			static const char* ToString(bdaddr_t bdaddr) {
				static char str[12];
				
				return str;
			}
			
	};
}

#endif

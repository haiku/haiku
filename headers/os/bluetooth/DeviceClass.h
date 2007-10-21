/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _DEVICE_CLASS_H
#define _DEVICE_CLASS_H

#include <String.h>

namespace Bluetooth {

	class DeviceClass {
    
        public:
            DeviceClass(int record) {
                this->record = record;
            }
    
            int getServiceClasses() {
       			return (record & 0x00FFE000) >> 13;
            }
    
            int getMajorDeviceClass() {
       			return (record & 0x00001F00) >> 8;
            }

            void getMajorDeviceClass(BString* str) {

            }
    
            int getMinorDeviceClass() {
    			return (record & 0x000000FF) >> 2;
            }

            void getMinorDeviceClass(BString* str) {

            }
            
        private:
            int record;        
    };
}

#endif

/*
 * Originally released under the Be Sample Code License.
 * Copyright 2000, Be Incorporated. All rights reserved.
 *
 * Modified for Haiku by François Revol and Michael Lotz.
 * Copyright 2007-2016, Haiku Inc. All rights reserved.
 */


#ifndef LISTUSB_H
#define LISTUSB_H


#include <USBKit.h>


void DumpDescriptorData(const usb_generic_descriptor* descriptor);

void DumpAudioDescriptor(const usb_generic_descriptor* descriptor, int subclass);
void DumpVideoDescriptor(const usb_generic_descriptor* descriptor, int subclass);


#endif /* !LISTUSB_H */

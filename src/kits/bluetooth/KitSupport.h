/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef KITSUPPORT_H
#define KITSUPPORT_H


#include <Messenger.h>


BMessenger* _RetrieveBluetoothMessenger(void);

uint8 GetInquiryTime();
void SetInquiryTime(uint8 time);


#endif

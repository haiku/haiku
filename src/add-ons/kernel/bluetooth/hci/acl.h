/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _ACL_PROC_H_
#define _ACL_PROC_H_

#include <net_buffer.h>

#include <bluetooth/HCI/btHCI.h>

status_t AclAssembly(net_buffer* buffer, hci_id hid);

status_t InitializeAclConnectionThread();
status_t QuitAclConnectionThread();

#endif

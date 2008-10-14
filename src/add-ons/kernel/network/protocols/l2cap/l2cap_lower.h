/* 
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_LOWER_H
#define L2CAP_LOWER_H

status_t l2cap_receive(HciConnection* conn, net_buffer* buffer);


status_t InitializeConnectionPurgeThread();
status_t QuitConnectionPurgeThread();
void SchedConnectionPurgeThread(HciConnection* conn);

#endif

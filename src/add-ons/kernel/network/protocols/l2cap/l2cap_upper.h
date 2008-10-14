/* 
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_UPPER_H
#define L2CAP_UPPER_H

#include "l2cap_internal.h"

status_t l2cap_l2ca_con_ind(L2capChannel* channel);
status_t l2cap_upper_con_rsp(HciConnection* conn, L2capChannel* channel);

#endif

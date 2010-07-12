/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_UPPER_H
#define L2CAP_UPPER_H

#include "l2cap_internal.h"

status_t l2cap_l2ca_con_ind(L2capChannel* channel);
status_t l2cap_cfg_req_ind(L2capChannel* channel);
status_t l2cap_discon_req_ind(L2capChannel* channel);
status_t l2cap_discon_rsp_ind(L2capChannel* channel);

status_t l2cap_con_rsp_ind(HciConnection* conn, L2capChannel* channel);
status_t l2cap_cfg_rsp_ind(L2capChannel* channel);

status_t l2cap_upper_con_req(L2capChannel* channel);
status_t l2cap_upper_dis_req(L2capChannel* channel);



status_t l2cap_co_receive(HciConnection* conn, net_buffer* buffer, uint16 dcid);
status_t l2cap_cl_receive(HciConnection* conn, net_buffer* buffer, uint16 psm);

#endif

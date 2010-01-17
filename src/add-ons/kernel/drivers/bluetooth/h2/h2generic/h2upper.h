/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _H2UPPER_H_
#define _H2UPPER_H_

#include <util/list.h>

#include "h2generic.h"

status_t post_packet_up(bt_usb_dev* bdev, bt_packet_t type, void* buf);
status_t send_packet(hci_id hid, bt_packet_t type, net_buffer* nbuf);
status_t send_command(hci_id hid, snet_buffer* snbuf);

void sched_tx_processing(bt_usb_dev* bdev);

#endif

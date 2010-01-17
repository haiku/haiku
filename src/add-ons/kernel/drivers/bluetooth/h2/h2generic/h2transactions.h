/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _H2TRANSACTION_H_
#define _H2TRANSACTION_H_

#include "h2generic.h"

status_t submit_rx_event(bt_usb_dev* bdev);
status_t submit_rx_acl(bt_usb_dev* bdev);
status_t submit_rx_sco(bt_usb_dev* bdev);

status_t submit_tx_command(bt_usb_dev* bdev, snet_buffer* snbuf);
status_t submit_tx_acl(bt_usb_dev* bdev, net_buffer* nbuf);
status_t submit_tx_sco(bt_usb_dev* bdev);

#endif

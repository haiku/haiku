/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _BTSCO_H_
#define _BTSCO_H_

#include <bluetooth/bluetooth.h>

struct sockaddr_sco {
    uint8        sco_len;        /* total length */
    uint8        sco_family;     /* address family */
    bdaddr_t     sco_bdaddr;     /* address */
};


#endif // _BTSCO_H_

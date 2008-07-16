/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _COMMAND_MANAGER_H
#define _COMMAND_MANAGER_H

#include <bluetooth/bluetooth.h>

/* CONTROL BASEBAND */
void* buildReset(size_t* outsize);
void* buildReadLocalName(size_t* outsize);
void* buildWriteScan(uint8 scanmode, size_t* outsize);
void* buildAuthEnable(uint8 auth, size_t* outsize);

/* LINK CONTROL */
void* buildRemoteNameRequest(bdaddr_t bdaddr,uint8 pscan_rep_mode, uint16 clock_offset, size_t* outsize);
void* buildInquiry(uint32 lap, uint8 length, uint8 num_rsp, size_t* outsize);
void* buildInquiryCancel(size_t* outsize);
void* buildPinCodeRequestReply(bdaddr_t bdaddr, uint8 length, char pincode[16], size_t* outsize);
void* buildPinCodeRequestNegativeReply(bdaddr_t bdaddr, size_t* outsize);
void* buildAcceptConnectionRequest(bdaddr_t bdaddr, uint8 role, size_t* outsize);
void* buildRejectConnectionRequest(bdaddr_t bdaddr, size_t* outsize);

/* OGF_INFORMATIONAL_PARAM */
void* buildReadBufferSize(size_t* outsize);
void* buildReadBdAddr(size_t* outsize);

#endif

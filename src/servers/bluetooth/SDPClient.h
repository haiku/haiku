/*
 * Copyright 2026, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		mohammedrattia <mohammedrattia@gmail.com>
 */
#ifndef _SDP_CLIENT_H_
#define _SDP_CLIENT_H_

#include <bluetooth/sdp.h>
#include <bluetooth/SDPBuffer.h>
#include <Message.h>

#include "LocalDeviceImpl.h"


class SDPClient {
public:

						SDPClient(ServerRemoteDevice* rd);
						~SDPClient();

	status_t			Start();
	void				Stop();


	status_t			RequestServiceRecords();
	status_t			NotifyProfiles();

private:

	void				_ParseAttrLists(BMallocIO* attrListsDataIO);
	status_t			_IssueRequest(const void *request, int32 size, BMallocIO* reply);

	status_t			_NotifyHIDProfile(BMessage* attrList);


	int					fClientSocket;
	ServerRemoteDevice* fRemoteDevice;
	sockaddr_l2cap		fSockAddrL2cap;

	uint16 transactionID;
};


#endif //_SDP_CLIENT_H_

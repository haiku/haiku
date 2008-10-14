/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <KernelExport.h>

#include <NetBufferUtilities.h>


#include <l2cap.h>
#include "l2cap_internal.h"
#include "l2cap_signal.h"
#include "l2cap_command.h"
#include "l2cap_lower.h"

#include "L2capEndpoint.h"

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "upper"
#define SUBMODULE_COLOR 36
#include <btDebug.h>


status_t
l2cap_l2ca_con_ind(L2capChannel* channel)
{
	L2capEndpoint* endpoint = L2capEndpoint::ForPsm(channel->psm);
	
	if (endpoint == NULL) { //refuse connection no endpoint bound
		debugf("No endpoint bound for psm %d\n", channel->psm);
		return B_ERROR;
	}

	channel->endpoint = endpoint;
	debugf("Endpoint %p bound for psm %d, schannel %x dchannel %x\n",endpoint, channel->psm, channel->scid, channel->dcid);
	
	
	L2capFrame	*cmd = NULL;

	cmd = btCoreData->SpawnSignal(channel->conn, channel, NULL, channel->ident, L2CAP_CON_RSP);
	if (cmd == NULL)
		return ENOMEM;

	cmd->buffer = l2cap_con_rsp(cmd->ident, channel->dcid, channel->scid, L2CAP_SUCCESS, L2CAP_NO_INFO);
	if (cmd->buffer == NULL) {
		btCoreData->AcknowledgeSignal(cmd);
		return ENOBUFS;
	}

	/* Link command to the queue */
	SchedConnectionPurgeThread(channel->conn);
	return B_OK;
}


status_t
l2cap_upper_con_rsp(HciConnection* conn, L2capChannel* channel)
{
	flowf("\n");

	return B_OK;
}

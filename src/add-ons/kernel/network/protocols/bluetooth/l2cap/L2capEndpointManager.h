/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef L2CAP_ENDPOINT_MANAGER_H
#define L2CAP_ENDPOINT_MANAGER_H

#include <util/AVLTree.h>

#include "L2capEndpoint.h"


class L2capEndpointManager {
public:
						L2capEndpointManager();
						~L2capEndpointManager();

	status_t			Bind(L2capEndpoint* endpoint, const sockaddr_l2cap& address);
	status_t			Unbind(L2capEndpoint* endpoint);
	L2capEndpoint*		ForPSM(uint16 psm);

	status_t			BindToChannel(L2capEndpoint* endpoint);
	status_t			UnbindFromChannel(L2capEndpoint* endpoint);
	L2capEndpoint*		ForChannel(uint16 cid);
	void				Disconnected(HciConnection* connection);

private:
	struct EndpointTreeDefinitionBase {
		typedef uint16			Key;
		typedef L2capEndpoint	Value;

		AVLTreeNode* GetAVLTreeNode(Value* value) const
		{
			return value;
		}

		Value* GetValue(AVLTreeNode* node) const
		{
			return static_cast<Value*>(node);
		}
	};
	struct EndpointBindTreeDefinition : public EndpointTreeDefinitionBase {
		int Compare(const Key& a, const Value* b) const
		{
			return a - ((sockaddr_l2cap*)*b->LocalAddress())->l2cap_psm;
		}

		int Compare(const Value* a, const Value* b) const
		{
			return Compare(((sockaddr_l2cap*)*a->LocalAddress())->l2cap_psm, b);
		}
	};
	struct EndpointChannelTreeDefinition : public EndpointTreeDefinitionBase {
		int Compare(const Key& a, const Value* b) const
		{
			return a - b->ChannelID();
		}

		int Compare(const Value* a, const Value* b) const
		{
			return Compare(a->ChannelID(), b);
		}
	};

	rw_lock fBoundEndpointsLock;
	AVLTree<EndpointBindTreeDefinition> fBoundEndpoints;

	rw_lock fChannelEndpointsLock;
	uint16 fNextChannelID;
	AVLTree<EndpointChannelTreeDefinition> fChannelEndpoints;
};

extern L2capEndpointManager gL2capEndpointManager;


#endif	// L2CAP_ENDPOINT_MANAGER_H

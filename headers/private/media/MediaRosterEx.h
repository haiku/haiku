/*
 * Copyright (c) 2003 Marcus Overhagen.
 * All Rights Reserved.
 *
 * This file may be used under the terms of the MIT License.
 */
#ifndef _MEDIA_ROSTER_EX_H_
#define _MEDIA_ROSTER_EX_H_

#ifndef _MEDIA_T_LIST_H
	#include "TList.h"
#endif
#ifndef _DATA_EXCHANGE_H
	#include "DataExchange.h"
#endif
#ifndef _MEDIA_NODE_H
	#include <MediaNode.h>
#endif
#ifndef _MEDIA_ADD_ON_H
	#include <MediaAddOn.h>
#endif

namespace BPrivate { namespace media {

/* The BMediaRosterEx class is an extension to the BMediaRoster.
 * It provides functionality that can be used by the implementation
 * of media_server, media_addon_server and libmedia.so.
 * To access it, convert any BMediaRoster pointer in a BMediaRosterEx
 * pointer using the inline function provided below.
 */
class BMediaRosterEx : public BMediaRoster
{
public:
	BMediaRosterEx(status_t * out_error);
	status_t SaveNodeConfiguration(BMediaNode *node);
	status_t LoadNodeConfiguration(media_addon_id addonid, int32 flavorid, BMessage *out_msg);

	status_t IncrementAddonFlavorInstancesCount(media_addon_id addonid, int32 flavorid);
	status_t DecrementAddonFlavorInstancesCount(media_addon_id addonid, int32 flavorid);
	status_t ReleaseNodeAll(const media_node& node);

	status_t SetNodeCreator(media_node_id node, team_id creator);

	status_t RegisterNode(BMediaNode * node, media_addon_id addonid, int32 flavorid);
	status_t InstantiateDormantNode(media_addon_id addonid, int32 flavorid, team_id creator, media_node *out_node);
	status_t GetDormantFlavorInfo(media_addon_id addonid, int32 flavorid, dormant_flavor_info *out_flavor);
	status_t GetNode(node_type type, media_node * out_node, int32 * out_input_id = NULL, BString * out_input_name = NULL);
	status_t SetNode(node_type type, const media_node *node, const dormant_node_info *info = NULL, const media_input *input = NULL);
	status_t GetAllOutputs(const media_node & node, List<media_output> *list);
	status_t GetAllOutputs(BBufferProducer *node, List<media_output> *list);
	status_t GetAllInputs(const media_node & node, List<media_input> *list);
	status_t GetAllInputs(BBufferConsumer *node, List<media_input> *list);
	status_t PublishOutputs(const media_node & node, List<media_output> *list);
	status_t PublishInputs(const media_node & node, List<media_input> *list);

	BTimeSource *MakeTimeSourceObject(media_node_id timesource_id);

private:
	friend class BMediaRoster;
};

/* The pointer returned by BMediaRoster::Roster() is always a
 * BMediaRosterEx object pointer. Use this to convert it.
 */
inline BMediaRosterEx * MediaRosterEx(BMediaRoster *mediaroster)
{
	return static_cast<BMediaRosterEx *>(mediaroster);
}


} } // BPrivate::media
using namespace BPrivate::media;

#endif

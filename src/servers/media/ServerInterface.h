#ifndef _SERVER_INTERFACE_H_
#define _SERVER_INTERFACE_H_

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <Entry.h>

#define NEW_MEDIA_SERVER_SIGNATURE 	"application/x-vnd.OpenBeOS-media-server"

enum {
	// Application management
	MEDIA_SERVER_REGISTER_APP,
	MEDIA_SERVER_UNREGISTER_APP,

	// Buffer management
	MEDIA_SERVER_GET_SHARED_BUFFER_AREA,
	MEDIA_SERVER_REGISTER_BUFFER,
	MEDIA_SERVER_UNREGISTER_BUFFER,

	// Something else
	MEDIA_SERVER_SET_VOLUME,
	MEDIA_SERVER_GET_VOLUME,
	MEDIA_SERVER_GET_NODE_ID,
	MEDIA_SERVER_FIND_RUNNING_INSTANCES,
	MEDIA_SERVER_BUFFER_GROUP_REG,
	MEDIA_SERVER_GET_LATENT_INFO,
	MEDIA_SERVER_GET_DORMANT_FILE_FORMATS,
	MEDIA_SERVER_GET_GETDORMANTFLAVOR,
	MEDIA_SERVER_BROADCAST_MESSAGE,
	MEDIA_SERVER_RELEASE_NODE_REFERENCE,
	MEDIA_SERVER_SET_REALTIME_FLAGS,
	MEDIA_SERVER_GET_REALTIME_FLAGS,
	MEDIA_SERVER_INSTANTIATE_PERSISTENT_NODE,
	MEDIA_SERVER_SNIFF_FILE,
	MEDIA_SERVER_QUERY_LATENTS,
	MEDIA_SERVER_REGISTER_NODE,
	MEDIA_SERVER_UNREGISTER_NODE,
	MEDIA_SERVER_SET_DEFAULT,
	MEDIA_SERVER_ACQUIRE_NODE_REFERENCE,
	MEDIA_SERVER_SET_OUTPUT_BUFFERS,
	MEDIA_SERVER_RECLAIM_OUTPUT_BUFFERS,
	MEDIA_SERVER_ORPHAN_RECLAIMABLE_BUFFERS,
	MEDIA_SERVER_SET_TIME_SOURCE,
	MEDIA_SERVER_QUERY_NODES,
	MEDIA_SERVER_GET_DORMANT_NODE,
	MEDIA_SERVER_FORMAT_CHANGED,
	MEDIA_SERVER_GET_DEFAULT_INFO,
	MEDIA_SERVER_GET_RUNNING_DEFAULT,
	MEDIA_SERVER_SET_RUNNING_DEFAULT,
	MEDIA_SERVER_TYPE_ITEM_OP,
	MEDIA_SERVER_FORMAT_OP
};

enum {
	ADDONSERVER_INSTANTIATE_DORMANT_NODE,
	SERVER_REGISTER_MEDIAADDON,
	SERVER_UNREGISTER_MEDIAADDON,
	SERVER_GET_MEDIAADDON_REF,
	ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS,
	SERVER_REGISTER_DORMANT_NODE,
	SERVER_GET_DORMANT_NODES,
	SERVER_GET_DORMANT_FLAVOR_INFO,
	END
};



struct xfer_server_get_dormant_flavor_info
{
	media_addon_id addon;
	int32 flavor_id;
	port_id reply_port;
};

struct xfer_server_get_dormant_flavor_info_reply
{
	status_t 	result;
	type_code	dfi_type; // the flatten type_code
	size_t 		dfi_size; 
	char 		dfi[1];   // a flattened dormant_flavor_info, dfi_size large
};

struct xfer_server_get_dormant_nodes
{
	int32 maxcount;
	bool has_input;
	media_format inputformat;
	bool has_output;
	media_format outputformat;
	bool has_name;
	char name[B_MEDIA_NAME_LENGTH + 1]; // 1 for a trailing "*"
	uint64 require_kinds;
	uint64 deny_kinds;
	port_id reply_port;
};

struct xfer_server_get_dormant_nodes_reply
{
	status_t result;
	int32 count; // if count > 0, a second reply containing count dormant_node_infos is send
};

struct xfer_server_register_dormant_node
{
	media_addon_id	purge_id; // if > 0, server must first remove all dormant_flavor_infos belonging to that id
	type_code		dfi_type; // the flatten type_code
	size_t 			dfi_size; 
	char 			dfi[1];   // a flattened dormant_flavor_info, dfi_size large
};




namespace BPrivate { namespace media { namespace dataexchange {
	status_t QueryServer(BMessage *query, BMessage *reply);
}; }; };

using namespace BPrivate::media::dataexchange;

#endif

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


/* We can't send an entry_ref through a port to another team,
 * but we can assign it to an xfer_entry_ref and send this one,
 * when we receive it we can assign it to a normal entry_ref
 */
struct xfer_entry_ref
{
public:
	xfer_entry_ref()
		{
			device = -1;
			directory = -1;
			name[0] = 0;
		}
	operator entry_ref() const
		{
			entry_ref ref(device, directory, name);
			return ref;
		}
	void operator=(const entry_ref &ref)
		{
			device = ref.device;
			directory = ref.directory;
			strcpy(name, ref.name);
		}
private:
	dev_t	device;
	ino_t	directory;
	char	name[B_FILE_NAME_LENGTH];
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

struct xfer_addonserver_rescan_mediaaddon_flavors
{
	media_addon_id addonid;
};

struct xfer_server_register_mediaaddon
{
	port_id 	reply_port;
	xfer_entry_ref	ref; // a ref to the file
};

struct xfer_server_register_mediaaddon_reply
{
	media_addon_id addonid;
};

struct xfer_server_unregister_mediaaddon
{
	media_addon_id addonid;
};

struct xfer_server_get_mediaaddon_ref
{
	media_addon_id addonid;
	port_id 	reply_port;
};

struct xfer_server_get_mediaaddon_ref_reply
{
	status_t result;
	xfer_entry_ref	ref; // a ref to the file
};

struct xfer_producer_format_suggestion_requested
{
	media_type type;
	int32 quality;
	port_id reply_port;
};

struct xfer_producer_format_suggestion_requested_reply
{
	media_format format;
	status_t result;
};

struct xfer_producer_set_play_rate
{
	int32 numer;
	int32 denom;
	port_id reply_port;
};

struct xfer_producer_set_play_rate_reply
{
	status_t result;
};

struct xfer_producer_get_initial_latency
{
	port_id reply_port;
};

struct xfer_producer_get_initial_latency_reply
{
	bigtime_t initial_latency;
	uint32 flags;
};

struct xfer_producer_get_latency
{
	port_id reply_port;
};

struct xfer_producer_get_latency_reply
{
	bigtime_t latency;
	status_t result;
};

struct xfer_producer_set_buffer_group
{
	media_source source;
	media_destination destination;
	void *user_data;
	int32 change_tag;
	int32 buffer_count;
	media_buffer_id buffers[1];
};

struct xfer_producer_format_change_requested
{
	media_source source;
	media_destination destination;
	media_format format;
	void *user_data;
	int32 change_tag;
};

struct xfer_producer_video_clipping_changed
{
	media_source source;
	media_destination destination;
	media_video_display_info display;
	void *user_data;
	int32 change_tag;
	int32 short_count;
	int16 shorts[1];
};

struct xfer_producer_additional_buffer_requested
{
	media_source source;
	media_buffer_id prev_buffer;
	bigtime_t prev_time;
	bool has_seek_tag;
	media_seek_tag prev_tag;
};

struct xfer_producer_latency_changed
{
	media_source source;
	media_destination destination;
	bigtime_t latency;
	uint32 flags;
};

struct xfer_node_request_completed
{
	media_request_info info;
};

struct xfer_producer_enable_output
{
	media_source source;
	media_destination destination;
	bool enabled;
	void *user_data;
	int32 change_tag;
};

struct xfer_producer_late_notice_received
{
	media_source source;
	bigtime_t how_much;
	bigtime_t performance_time;
};

struct xfer_node_start
{
	bigtime_t performance_time;	
};

struct xfer_node_stop
{
	bigtime_t performance_time;	
	bool immediate;
};

struct xfer_node_seek
{
	bigtime_t media_time;
	bigtime_t performance_time;
};

struct xfer_node_set_run_mode
{
	BMediaNode::run_mode mode;
};

struct xfer_node_time_warp
{
	bigtime_t at_real_time;
	bigtime_t to_performance_time;
};

struct xfer_node_set_timesource
{
	media_node_id timesource_id;
};

struct xfer_consumer_buffer_received
{
	media_buffer_id buffer;
	media_header header;
};

struct xfer_consumer_producer_data_status
{
	media_destination for_whom;
	int32 status;
	bigtime_t at_performance_time;
};

struct xfer_consumer_get_latency_for
{
	media_destination for_whom;
	port_id reply_port;
};

struct xfer_consumer_get_latency_for_reply
{
	bigtime_t latency;
	media_node_id timesource;
	status_t result;
};
	
struct xfer_consumer_format_changed
{
	media_source producer;
	media_destination consumer;
	int32 change_tag;
	media_format format;
	port_id reply_port;
};

struct xfer_consumer_format_changed_reply
{
	status_t result;
};

struct xfer_consumer_seek_tag_requested
{
	media_destination destination;
	bigtime_t target_time;
	uint32 flags;
	port_id reply_port;
};

struct xfer_consumer_seek_tag_requested_reply
{
	media_seek_tag seek_tag;
	bigtime_t tagged_time;
	uint32 flags;
	status_t result;
};

namespace BPrivate { namespace media { namespace dataexchange {
	status_t QueryServer(BMessage *query, BMessage *reply);
}; }; };

using namespace BPrivate::media::dataexchange;

#endif

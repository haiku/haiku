#ifndef _SERVER_INTERFACE_H_
#define _SERVER_INTERFACE_H_

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>

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
	MEDIA_SERVER_REQUEST_NOTIFICATIONS,
	MEDIA_SERVER_CANCEL_NOTIFICATIONS,
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
	NODE_START,
	NODE_STOP,
	NODE_SEEK,
	NODE_SET_RUN_MODE,
	NODE_TIME_WARP,
	NODE_PREROLL,
	NODE_REGISTERED,
	NODE_SET_TIMESOURCE,
	NODE_REQUEST_COMPLETED,
	CONSUMER_ACCEPT_FORMAT,
	CONSUMER_GET_NEXT_INPUT,
	CONSUMER_DISPOSE_INPUT_COOKIE,
	CONSUMER_BUFFER_RECEIVED,
	CONSUMER_PRODUCER_DATA_STATUS,
	CONSUMER_GET_LATENCY_FOR,
	CONSUMER_CONNECTED,
	CONSUMER_DISCONNECTED,
	CONSUMER_FORMAT_CHANGED,
	CONSUMER_SEEK_TAG_REQUESTED,
	PRODUCER_LATE_NOTICE_RECEIVED,
	PRODUCER_ENABLE_OUTPUT,
	PRODUCER_LATENCY_CHANGED,
	PRODUCER_ADDITIONAL_BUFFER_REQUESTED,
	PRODUCER_VIDEO_CLIPPING_CHANGED,
	PRODUCER_FORMAT_CHANGE_REQUESTED,
	PRODUCER_SET_BUFFER_GROUP,
	PRODUCER_GET_LATENCY,
	PRODUCER_GET_NEXT_OUTPUT,
	PRODUCER_DISPOSE_OUTPUT_COOKIE,
	PRODUCER_GET_INITIAL_LATENCY,
	PRODUCER_FORMAT_SUGGESTION_REQUESTED,
	PRODUCER_FORMAT_PROPOSAL,
	PRODUCER_PREPARE_TO_CONNECT,
	PRODUCER_CONNECT,
	PRODUCER_DISCONNECT,
	PRODUCER_SET_PLAY_RATE,
	ADDONSERVER_INSTANTIATE_DORMANT_NODE,
	SERVER_REGISTER_MEDIAADDON,
	SERVER_UNREGISTER_MEDIAADDON,
	ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS,
	SERVER_REGISTER_DORMANT_NODE,
	SERVER_GET_NODE,
	SERVER_SET_NODE,
	TIMESOURCE_OP, // datablock is a struct time_source_op_info
	SERVER_GET_DORMANT_NODES,
	SERVER_GET_DORMANT_FLAVOR_INFO,
	END
};

enum node_type 
{ 
	VIDEO_INPUT, 
	AUDIO_INPUT, 
	VIDEO_OUTPUT, 
	AUDIO_MIXER, 
	AUDIO_OUTPUT, 
	AUDIO_OUTPUT_EX, 
	TIME_SOURCE, 
	SYSTEM_TIME_SOURCE 
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

struct xfer_server_set_node
{
	node_type type;
	bool use_node;
	media_node node;
	bool use_dni;
	dormant_node_info dni;
	bool use_input;
	media_input input;
	port_id reply_port;
};

struct xfer_server_set_node_reply
{
	status_t result;
};

struct xfer_server_get_node
{
	node_type type;
	port_id reply_port;
};

struct xfer_server_get_node_reply
{
	media_node node;
	status_t result;

	// for AUDIO_OUTPUT_EX
	char input_name[B_MEDIA_NAME_LENGTH];
	int32 input_id;
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
	port_id reply_port;
};

struct xfer_server_register_mediaaddon_reply
{
	media_addon_id addonid;
};

struct xfer_server_unregister_mediaaddon
{
	media_addon_id addonid;
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

struct xfer_producer_format_proposal
{
	media_source output;
	media_format format;
	port_id reply_port;
};

struct xfer_producer_format_proposal_reply
{
	status_t result;
};

struct xfer_producer_prepare_to_connect
{
	media_source source;
	media_destination destination;
	media_format format;
	port_id reply_port;
};

struct xfer_producer_prepare_to_connect_reply
{
	media_format format;
	media_source out_source;
	char name[B_MEDIA_NAME_LENGTH];
	status_t result;
};

struct xfer_producer_connect
{
	status_t error;
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
	port_id reply_port;
};

struct xfer_producer_connect_reply
{
	char name[B_MEDIA_NAME_LENGTH];
};

struct xfer_producer_disconnect
{
	media_source source;
	media_destination destination;
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

struct xfer_producer_get_next_output
{
	int32 cookie;
	port_id reply_port;
};

struct xfer_producer_get_next_output_reply
{
	int32 cookie;
	media_output output;
	status_t result;
};

struct xfer_producer_dispose_output_cookie
{
	int32 cookie;
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

struct xfer_node_registered
{
	media_node_id node_id;
};

struct xfer_node_set_timesource
{
	media_node_id timesource_id;
};

struct xfer_consumer_accept_format
{
	media_destination dest;
	media_format format;
	port_id reply_port;
};

struct xfer_consumer_accept_format_reply
{
	media_format format;
	status_t result;
};

struct xfer_consumer_get_next_input
{
	int32 cookie;
	port_id reply_port;
};

struct xfer_consumer_get_next_input_reply
{
	int32 cookie;
	media_input input;
	status_t result;
};

struct xfer_consumer_dispose_input_cookie
{
	int32 cookie;
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
	
struct xfer_consumer_connected
{
	media_source producer;
	media_destination where;
	media_format with_format;
	port_id reply_port;
};

struct xfer_consumer_connected_reply
{
	media_input input;
	status_t result;
};

struct xfer_consumer_disconnected
{
	media_source producer;
	media_destination where;
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

struct xfer_addonserver_instantiate_dormant_node
{
	dormant_node_info info;
	port_id reply_port;
};

struct xfer_addonserver_instantiate_dormant_node_reply
{
	media_node node;
	status_t result;
};

// implementation contained in libmedia.so (MediaRoster.cpp)
namespace MediaKitPrivate 
{
	status_t QueryServer(BMessage *query, BMessage *reply);
};

#endif

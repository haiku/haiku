/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _DATA_EXCHANGE_H
#define _DATA_EXCHANGE_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <Messenger.h>
#include <Buffer.h>
#include <Entry.h>

namespace BPrivate {
namespace media {
namespace dataexchange {

struct reply_data;
struct request_data;
struct command_data;

// BMessage based data exchange with the media_server
status_t SendToServer(BMessage *msg);

// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgcode, command_data *msg, int size);
status_t QueryServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize);

// Raw data based data exchange with the media_addon_server
status_t SendToAddonServer(int32 msgcode, command_data *msg, int size);
status_t QueryAddonServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize);

// Raw data based data exchange with the media_server
status_t SendToPort(port_id sendport, int32 msgcode, command_data *msg, int size);
status_t QueryPort(port_id requestport, int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize);

// The base struct used for all raw requests
struct request_data
{
	port_id reply_port;

	status_t SendReply(status_t result, reply_data *reply, int replysize) const;
};

// The base struct used for all raw replys
struct reply_data
{
	status_t result;
};

// The base struct used for all raw commands (asynchronous, no answer)
struct command_data
{
	// yes, it's empty ;)
};

}; // dataexchange
}; // media
}; // BPrivate

using namespace BPrivate::media::dataexchange;

// BMessage based server communication
enum {

	// BMediaRoster notification service
	MEDIA_SERVER_REQUEST_NOTIFICATIONS = 1000,
	MEDIA_SERVER_CANCEL_NOTIFICATIONS,
	MEDIA_SERVER_SEND_NOTIFICATIONS

};

// Raw port based communication
enum {
	ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS = 0x50,

	SERVER_MESSAGE_START = 0x100,
	SERVER_REGISTER_ADDONSERVER,
	SERVER_REGISTER_APP,
	SERVER_UNREGISTER_APP,
	SERVER_GET_NODE,
	SERVER_SET_NODE,
	SERVER_PUBLISH_INPUTS,
	SERVER_PUBLISH_OUTPUTS,
	SERVER_NODE_ID_FOR,
	SERVER_GET_LIVE_NODE_INFO,
	SERVER_GET_LIVE_NODES,
	SERVER_GET_NODE_FOR,
	SERVER_RELEASE_NODE,
	SERVER_REGISTER_NODE,
	SERVER_UNREGISTER_NODE,
	SERVER_GET_DORMANT_NODE_FOR,
	SERVER_GET_INSTANCES_FOR,
	SERVER_GET_SHARED_BUFFER_AREA,
	SERVER_REGISTER_BUFFER,
	SERVER_UNREGISTER_BUFFER,
	SERVER_RESCAN_DEFAULTS,
	SERVER_MESSAGE_END,
	NODE_MESSAGE_START = 0x200,
	
	NODE_START,
	NODE_STOP,
	NODE_SEEK,
	NODE_SET_RUN_MODE,
	NODE_TIME_WARP,
	NODE_PREROLL,
	NODE_SET_TIMESOURCE,
	NODE_REQUEST_COMPLETED,
	
	NODE_MESSAGE_END,
	CONSUMER_MESSAGE_START = 0x300,
	CONSUMER_GET_NEXT_INPUT,
	CONSUMER_DISPOSE_INPUT_COOKIE,
	CONSUMER_ACCEPT_FORMAT,
	CONSUMER_CONNECTED,
	CONSUMER_DISCONNECTED,
	
	CONSUMER_BUFFER_RECEIVED,
	CONSUMER_PRODUCER_DATA_STATUS,
	CONSUMER_GET_LATENCY_FOR,
	CONSUMER_FORMAT_CHANGED,
	CONSUMER_SEEK_TAG_REQUESTED,
	
	CONSUMER_MESSAGE_END,
	PRODUCER_MESSAGE_START = 0x400,
	PRODUCER_GET_NEXT_OUTPUT,
	PRODUCER_DISPOSE_OUTPUT_COOKIE,
	PRODUCER_FORMAT_PROPOSAL,
	PRODUCER_PREPARE_TO_CONNECT,
	PRODUCER_CONNECT,
	PRODUCER_DISCONNECT,
	
	PRODUCER_LATE_NOTICE_RECEIVED,
	PRODUCER_LATENCY_CHANGED,
	PRODUCER_ADDITIONAL_BUFFER_REQUESTED,
	PRODUCER_VIDEO_CLIPPING_CHANGED,
	PRODUCER_FORMAT_CHANGE_REQUESTED,
	PRODUCER_SET_BUFFER_GROUP,
	PRODUCER_GET_LATENCY,
	PRODUCER_GET_INITIAL_LATENCY,
	PRODUCER_FORMAT_SUGGESTION_REQUESTED,
	PRODUCER_SET_PLAY_RATE,
	PRODUCER_ENABLE_OUTPUT,	
	
	PRODUCER_MESSAGE_END,
	FILEINTERFACE_MESSAGE_START = 0x500,
	FILEINTERFACE_MESSAGE_END,
	CONTROLLABLE_MESSAGE_START = 0x600,
	CONTROLLABLE_MESSAGE_END,
	TIMESOURCE_MESSAGE_START = 0x700,
	
	TIMESOURCE_OP, // datablock is a struct time_source_op_info
	TIMESOURCE_ADD_SLAVE_NODE,
	TIMESOURCE_REMOVE_SLAVE_NODE,
	
	TIMESOURCE_MESSAGE_END,
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



// used by SERVER_GET_NODE and SERVER_SET_NODE
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

// used by SERVER_PUBLISH_INPUTS and SERVER_PUBLISH_OUTPUTS
enum
{
	MAX_OUTPUTS = 48,
	MAX_INPUTS = 48,
};

// used by SERVER_GET_LIVE_NODES
enum
{
	MAX_LIVE_INFO = 62,
};

// used by SERVER_GET_INSTANCES_FOR
enum
{
	MAX_NODE_ID = 4000,
};

struct addonserver_instantiate_dormant_node_request : public request_data
{
	dormant_node_info info;
};

struct addonserver_instantiate_dormant_node_reply : public reply_data
{
	media_node node;
};

struct server_set_node_request : public request_data
{
	node_type type;
	bool use_node;
	media_node node;
	bool use_dni;
	dormant_node_info dni;
	bool use_input;
	media_input input;
};

struct server_set_node_reply : public reply_data
{
};

struct server_get_node_request : public request_data
{
	node_type type;
	team_id team;
};

struct server_get_node_reply : public reply_data
{
	media_node node;

	// for AUDIO_OUTPUT_EX
	char input_name[B_MEDIA_NAME_LENGTH];
	int32 input_id;
};

struct producer_format_proposal_request : public request_data
{
	media_source output;
	media_format format;
};

struct producer_format_proposal_reply : public reply_data
{
	media_format format;
};

struct producer_prepare_to_connect_request : public request_data
{
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_prepare_to_connect_reply : public reply_data
{
	media_format format;
	media_source out_source;
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_connect_request : public request_data
{
	status_t error;
	media_source source;
	media_destination destination;
	media_format format;
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_connect_reply : public reply_data
{
	char name[B_MEDIA_NAME_LENGTH];
};

struct producer_disconnect_request : public request_data
{
	media_source source;
	media_destination destination;
};

struct producer_disconnect_reply : public reply_data
{
};

















struct producer_format_suggestion_requested_request : public request_data
{
	media_type type;
	int32 quality;
};

struct producer_format_suggestion_requested_reply : public reply_data
{
	media_format format;
};

struct producer_set_play_rate_request : public request_data
{
	int32 numer;
	int32 denom;
};

struct producer_set_play_rate_reply : public reply_data
{
};

struct producer_get_initial_latency_request : public request_data
{
};

struct producer_get_initial_latency_reply : public reply_data
{
	bigtime_t initial_latency;
	uint32 flags;
};

struct producer_get_latency_request : public request_data
{
};

struct producer_get_latency_reply : public reply_data
{
	bigtime_t latency;
};

struct producer_set_buffer_group_command : public command_data
{
	media_source source;
	media_destination destination;
	void *user_data;
	int32 change_tag;
	int32 buffer_count;
	media_buffer_id buffers[1];
};

struct producer_format_change_requested_command : public command_data
{
	media_source source;
	media_destination destination;
	media_format format;
	void *user_data;
	int32 change_tag;
};

struct producer_video_clipping_changed_command : public command_data
{
	media_source source;
	media_destination destination;
	media_video_display_info display;
	void *user_data;
	int32 change_tag;
	int32 short_count;
	int16 shorts[1];
};

struct producer_additional_buffer_requested_command : public command_data
{
	media_source source;
	media_buffer_id prev_buffer;
	bigtime_t prev_time;
	bool has_seek_tag;
	media_seek_tag prev_tag;
};

struct producer_latency_changed_command : public command_data
{
	media_source source;
	media_destination destination;
	bigtime_t latency;
	uint32 flags;
};

struct producer_enable_output_command : public command_data
{
	media_source source;
	media_destination destination;
	bool enabled;
	void *user_data;
	int32 change_tag;
};

struct producer_late_notice_received_command : public command_data
{
	media_source source;
	bigtime_t how_much;
	bigtime_t performance_time;
};































struct consumer_accept_format_request : public request_data
{
	media_destination dest;
	media_format format;
};

struct consumer_accept_format_reply : public reply_data
{
	media_format format;
};

struct consumer_connected_request : public request_data
{
	media_source producer;
	media_destination where;
	media_format with_format;
};

struct consumer_connected_reply : public reply_data
{
	media_input input;
};

struct server_publish_inputs_request : public request_data
{
	media_node node;
	int32 count;
	area_id area;	// if count > MAX_INPUTS, inputs are in the area
					// area is created in the library, and also deleted
					// in the library after the reply has been received
	media_input inputs[MAX_INPUTS];
};

struct server_publish_inputs_reply : public reply_data
{
};

struct server_publish_outputs_request : public request_data
{
	media_node node;
	int32 count;
	area_id area; // if count > MAX_OUTPUTS, outputs are in the area
					// area is created in the library, and also deleted
					// in the library after the reply has been received
	media_output outputs[MAX_OUTPUTS];
};

struct server_publish_outputs_reply : public reply_data
{
};

struct producer_get_next_output_request : public request_data
{
	int32 cookie;
};

struct producer_get_next_output_reply : public reply_data
{
	int32 cookie;
	media_output output;
};

struct producer_dispose_output_cookie_request : public request_data
{
	int32 cookie;
};

struct producer_dispose_output_cookie_reply : public reply_data
{
};

struct consumer_get_next_input_request : public request_data
{
	int32 cookie;
};

struct consumer_get_next_input_reply : public reply_data
{
	int32 cookie;
	media_input input;
};

struct consumer_dispose_input_cookie_request : public request_data
{
	int32 cookie;
};

struct consumer_dispose_input_cookie_reply : public reply_data
{
};

struct consumer_disconnected_request : public request_data
{
	media_source source;
	media_destination destination;
};

struct consumer_disconnected_reply : public reply_data
{
};

















struct consumer_buffer_received_command : public command_data
{
	media_buffer_id buffer;
	media_header header;
};

struct consumer_producer_data_status_command : public command_data
{
	media_destination for_whom;
	int32 status;
	bigtime_t at_performance_time;
};

struct consumer_get_latency_for_request : public request_data
{
	media_destination for_whom;
};

struct consumer_get_latency_for_reply : public reply_data
{
	bigtime_t latency;
	media_node_id timesource;
};

struct consumer_format_changed_request : public request_data
{
	media_source producer;
	media_destination consumer;
	int32 change_tag;
	media_format format;
};

struct consumer_format_changed_reply : public reply_data
{
};

struct consumer_seek_tag_requested_request : public request_data
{
	media_destination destination;
	bigtime_t target_time;
	uint32 flags;
};

struct consumer_seek_tag_requested_reply : public reply_data
{
	media_seek_tag seek_tag;
	bigtime_t tagged_time;
	uint32 flags;
};







struct server_register_addonserver_request : public request_data
{
	team_id team;
};

struct server_register_addonserver_reply : public reply_data
{
};

struct server_register_app_request : public request_data
{
	team_id team;
	BMessenger messenger;
};

struct server_register_app_reply : public reply_data
{
};

struct server_unregister_app_request : public request_data
{
	team_id team;
};

struct server_unregister_app_reply : public reply_data
{
};



struct server_register_node_request : public request_data
{
	media_addon_id addon_id;
	int32 addon_flavor_id;
	char name[B_MEDIA_NAME_LENGTH];
	uint64 kinds;
	port_id port;
	team_id team;
};

struct server_register_node_reply : public reply_data
{
	media_node_id nodeid;
};

struct server_unregister_node_request : public request_data
{
	media_node_id nodeid;
	team_id team;
};

struct server_unregister_node_reply : public reply_data
{
	media_addon_id addon_id;
};

struct server_get_live_node_info_request : public request_data
{
	media_node node;
};

struct server_get_live_node_info_reply : public reply_data
{
	live_node_info live_info;
};

struct server_get_live_nodes_request : public request_data
{
	int32 maxcount;
	bool has_input;
	bool has_output;
	bool has_name;
	media_format inputformat;
	media_format outputformat;
	char name[B_MEDIA_NAME_LENGTH + 1]; // 1 for a trailing "*"
	uint64 require_kinds;
};

struct server_get_live_nodes_reply : public reply_data
{
	int32 count;
	area_id area; 	// if count > MAX_LIVE_INFO, live_node_infos are in the area
					// area is created in the server, but deleted in the library
	live_node_info live_info[MAX_LIVE_INFO]; 
};

struct server_node_id_for_request : public request_data
{
	port_id port;
};

struct server_node_id_for_reply : public reply_data
{
	media_node_id nodeid;
};

struct server_get_node_for_request : public request_data
{
	media_node_id nodeid;
	team_id team;
};

struct server_get_node_for_reply : public reply_data
{
	media_node clone;
};

struct server_release_node_request : public request_data
{
	media_node node;
	team_id team;
};

struct server_release_node_reply : public reply_data
{
};

struct server_get_dormant_node_for_request : public request_data
{
	media_node node;
};

struct server_get_dormant_node_for_reply : public reply_data
{
	dormant_node_info node_info;
};

struct server_get_instances_for_request : public request_data
{
	int32 maxcount;
	media_addon_id addon_id;
	int32 addon_flavor_id;
};

struct server_get_instances_for_reply : public reply_data
{
	int32 count;
	media_node_id node_id[MAX_NODE_ID]; // no area here, MAX_NODE_ID is really large
};

struct server_rescan_defaults_command : public command_data
{
};




struct addonserver_rescan_mediaaddon_flavors_command : public command_data
{
	media_addon_id addonid;
};

struct server_register_mediaaddon_request : public request_data
{
	xfer_entry_ref	ref; // a ref to the file
};

struct server_register_mediaaddon_reply : public reply_data
{
	media_addon_id addonid;
};

struct server_unregister_mediaaddon_command : public command_data
{
	media_addon_id addonid;
};

struct server_get_mediaaddon_ref_request : public request_data
{
	media_addon_id addonid;
};

struct server_get_mediaaddon_ref_reply : public reply_data
{
	xfer_entry_ref	ref; // a ref to the file
};

struct server_get_shared_buffer_area_request : public request_data
{
};

struct server_get_shared_buffer_area_reply : public reply_data
{
	area_id area;
};

struct server_register_buffer_request : public request_data
{
	team_id team;
	//either info.buffer is != 0, or the area, size, offset is used
	buffer_clone_info info;
};

struct server_register_buffer_reply : public reply_data
{
	buffer_clone_info info;
};

struct server_unregister_buffer_command : public command_data
{
	team_id team;
	media_buffer_id bufferid;
};

struct node_request_completed_command : public command_data
{
	media_request_info info;
};

struct node_start_command : public command_data
{
	bigtime_t performance_time;	
};

struct node_stop_command : public command_data
{
	bigtime_t performance_time;	
	bool immediate;
};

struct node_seek_command : public command_data
{
	bigtime_t media_time;
	bigtime_t performance_time;
};

struct node_set_run_mode_command : public command_data
{
	BMediaNode::run_mode mode;
};

struct node_time_warp_command : public command_data
{
	bigtime_t at_real_time;
	bigtime_t to_performance_time;
};

struct node_set_timesource_command : public command_data
{
	media_node_id timesource_id;
};

struct timesource_add_slave_node_command : public command_data
{
	media_node node;
};

struct timesource_remove_slave_node_command : public command_data
{
	media_node node;
};

#endif // _DATA_EXCHANGE_H

/*******************************************************************************
/
/	File:			MediaRoster.h
/
/   Description:   The BMediaRoster is the main application interface to the Media Kit
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_MEDIA_ROSTER_H)
#define _MEDIA_ROSTER_H

#include <map>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <config_manager.h>

class BMimeType;
struct dormant_flavor_info;
class BMediaAddOn;

class BMediaRoster : public BLooper
{
public:

		status_t GetVideoInput(
				media_node * out_node);
		status_t GetAudioInput(
				media_node * out_node);
		status_t GetVideoOutput(
				media_node * out_node);
		status_t GetAudioMixer(
				media_node * out_node);
		status_t GetAudioOutput(
				media_node * out_node);	/* Use the mixer rather than the output for most needs! */
		status_t GetAudioOutput(
				media_node * out_node,
				int32 * out_input_id,
				BString * out_input_name);
		status_t GetTimeSource(
				media_node * out_node);

		status_t SetVideoInput(
				const media_node & producer);
		status_t SetVideoInput(
				const dormant_node_info & producer);
		status_t SetAudioInput(
				const media_node & producer);
		status_t SetAudioInput(
				const dormant_node_info & producer);
		status_t SetVideoOutput(
				const media_node & consumer);
		status_t SetVideoOutput(
				const dormant_node_info & consumer);
		status_t SetAudioOutput(
				const media_node & consumer);
		status_t SetAudioOutput(
				const media_input & input_to_output);
		status_t SetAudioOutput(
				const dormant_node_info & consumer);

		/* Get a media_node from a node ID -- this is how you reference your own nodes! */
		status_t GetNodeFor(
				media_node_id node,
				media_node * clone);
		status_t GetSystemTimeSource(	/* typically, you want to use GetTimeSource() */
				media_node * clone);
		status_t ReleaseNode(	/* might shut down Node if you're last */
				const media_node & node);

		BTimeSource * MakeTimeSourceFor(	/* Release() the object when done */
				const media_node & for_node);

		/* note that the media_source and media_destination found in */
		/* out_output and out_input are the ones actually used; from and to */
		/* are only "hints" that the app should not use once a real connection */
		/* has been established */
		status_t Connect(
				const media_source & from,
				const media_destination & to,
				media_format * io_format,
				media_output * out_output,
				media_input * out_input);
		enum connect_flags {
			B_CONNECT_MUTED = 0x1
		};
		status_t Connect(
				const media_source & from,
				const media_destination & to,
				media_format * io_format,
				media_output * out_output,
				media_input * out_input,
				uint32 in_flags,
				void * _reserved = 0);
		status_t Disconnect(
				media_node_id source_node,
				const media_source & source,
				media_node_id destination_node,
				const media_destination & destination);

		status_t StartNode(
				const media_node & node,
				bigtime_t at_performance_time);
		status_t StopNode(
				const media_node & node,
				bigtime_t at_performance_time,
				bool immediate = false);	/* immediate -> at_time is insignificant */
		status_t SeekNode(
				const media_node & node,
				bigtime_t to_media_time,
				bigtime_t at_performance_time = 0 /* if running */ );

		status_t StartTimeSource(
				const media_node & node,
				bigtime_t at_real_time);
		status_t StopTimeSource(
				const media_node & node,
				bigtime_t at_real_time,
				bool immediate = false);
		status_t SeekTimeSource(
				const media_node & node,
				bigtime_t to_performance_time,
				bigtime_t at_real_time);

		status_t SyncToNode(
				const media_node & node,
				bigtime_t at_time,
				bigtime_t timeout = B_INFINITE_TIMEOUT);
		status_t SetRunModeNode(
				const media_node & node,
				BMediaNode::run_mode mode);
		status_t PrerollNode(	/* synchronous */
				const media_node & node);

		status_t RollNode(
				const media_node & node, 
				bigtime_t startPerformance,
				bigtime_t stopPerformance,
				bigtime_t atMediaTime = -B_INFINITE_TIMEOUT);

		status_t SetProducerRunModeDelay(	/* should only be used with B_RECORDING */
				const media_node & node,
				bigtime_t delay,
				BMediaNode::run_mode mode = BMediaNode::B_RECORDING);
		status_t SetProducerRate(	/* not necessarily supported by node */
				const media_node & producer,
				int32 numer,
				int32 denom);

	/* Nodes will have available inputs/outputs as long as they are capable */
	/* of accepting more connections. The node may create an additional */
	/* output or input as the currently available is taken into usage. */
		status_t GetLiveNodeInfo(
				const media_node & node,
				live_node_info * out_live_info);
		status_t GetLiveNodes(
				live_node_info * out_live_nodes,
				int32 * io_total_count,
				const media_format * has_input = NULL,
				const media_format * has_output = NULL,
				const char * name = NULL,
				uint64 node_kinds = 0);	/* B_BUFFER_PRODUCER etc */

		status_t GetFreeInputsFor(
				const media_node & node,
				media_input * out_free_inputs,
				int32 buf_num_inputs,
				int32 * out_total_count,
				media_type filter_type = B_MEDIA_UNKNOWN_TYPE);
		status_t GetConnectedInputsFor(
				const media_node & node,
				media_input * out_active_inputs,
				int32 buf_num_inputs,
				int32 * out_total_count);
		status_t GetAllInputsFor(
				const media_node & node,
				media_input * out_inputs,
				int32 buf_num_inputs,
				int32 * out_total_count);
		status_t GetFreeOutputsFor(
				const media_node & node,
				media_output * out_free_outputs,
				int32 buf_num_outputs,
				int32 * out_total_count,
				media_type filter_type = B_MEDIA_UNKNOWN_TYPE);
		status_t GetConnectedOutputsFor(
				const media_node & node,
				media_output * out_active_outputs,
				int32 buf_num_outputs,
				int32 * out_total_count);
		status_t GetAllOutputsFor(
				const media_node & node,
				media_output * out_outputs,
				int32 buf_num_outputs,
				int32 * out_total_count);

		status_t StartWatching(
				const BMessenger & where);
		status_t StartWatching(
				const BMessenger & where,
				int32 notificationType);
		status_t StartWatching(
				const BMessenger & where,
				const media_node & node,
				int32 notificationType);
		status_t StopWatching(
				const BMessenger & where);
		status_t StopWatching(
				const BMessenger & where,
				int32 notificationType);
		status_t StopWatching(
				const BMessenger & where,
				const media_node & node,
				int32 notificationType);

		status_t RegisterNode(
				BMediaNode * node);
		status_t UnregisterNode(
				BMediaNode * node);

static	BMediaRoster * Roster(					//	will create if there isn't one
				status_t * out_error = NULL);	//	thread safe for multiple calls to Roster()
static	BMediaRoster * CurrentRoster();			//	won't create it if there isn't one
												//	not thread safe if you call Roster() at the same time
		status_t SetTimeSourceFor(
				media_node_id node,
				media_node_id time_source);

		status_t GetParameterWebFor(
				const media_node & node, 
				BParameterWeb ** out_web);
		status_t StartControlPanel(
				const media_node & node,
				BMessenger * out_messenger = NULL);

		status_t GetDormantNodes(
				dormant_node_info * out_info,
				int32 * io_count,
				const media_format * has_input = NULL,
				const media_format * has_output = NULL,
				const char * name = NULL,
				uint64 require_kinds = 0,
				uint64 deny_kinds = 0);
		status_t InstantiateDormantNode(
				const dormant_node_info & in_info,
				media_node * out_node,
				uint32 flags /* currently B_FLAVOR_IS_GLOBAL or B_FLAVOR_IS_LOCAL */ );
		status_t InstantiateDormantNode(
				const dormant_node_info & in_info,
				media_node * out_node);
		status_t GetDormantNodeFor(
				const media_node & node,
				dormant_node_info * out_info);
		status_t GetDormantFlavorInfoFor(
				const dormant_node_info & in_dormant,
				dormant_flavor_info * out_flavor);

		status_t GetLatencyFor(
				const media_node & producer,
				bigtime_t * out_latency);
		status_t GetInitialLatencyFor(
				const media_node & producer,
				bigtime_t * out_latency,
				uint32 * out_flags = 0);
		status_t GetStartLatencyFor(
				const media_node & time_source,
				bigtime_t * out_latency);

		status_t GetFileFormatsFor(
				const media_node & file_interface, 
				media_file_format * out_formats,
				int32 * io_num_infos);
		status_t SetRefFor(
				const media_node & file_interface,
				const entry_ref & file,
				bool create_and_truncate,
				bigtime_t * out_length);	/* if create is false */
		status_t GetRefFor(
				const media_node & node,
				entry_ref * out_file,
				BMimeType * mime_type = NULL);
		status_t SniffRefFor(
				const media_node & file_interface,
				const entry_ref & file,
				BMimeType * mime_type,
				float * out_capability);
		/* This is the generic "here's a file, now can someone please play it" interface */
		status_t SniffRef(
				const entry_ref & file,
				uint64 require_node_kinds,		/* if you need an EntityInterface or BufferConsumer or something */
				dormant_node_info * out_node,
				BMimeType * mime_type = NULL);
		status_t GetDormantNodeForType(
				const BMimeType & type,
				uint64 require_node_kinds,
				dormant_node_info * out_node);
		status_t GetReadFileFormatsFor(
				const dormant_node_info & in_node,
				media_file_format * out_read_formats,
				int32 in_read_count,
				int32 * out_read_count);
		status_t GetWriteFileFormatsFor(
				const dormant_node_info & in_node,
				media_file_format * out_write_formats,
				int32 in_write_count,
				int32 * out_write_count);

		status_t GetFormatFor(
				const media_output & output,
				media_format * io_format,
				uint32 flags = 0);
		status_t GetFormatFor(
				const media_input & input,
				media_format * io_format,
				uint32 flags = 0);
		status_t GetFormatFor(
				const media_node & node,
				media_format * io_format,
				float quality = B_MEDIA_ANY_QUALITY);
		ssize_t GetNodeAttributesFor(
				const media_node & node,
				media_node_attribute * outArray,
				size_t inMaxCount);
		media_node_id NodeIDFor(
				port_id source_or_destination_port);
		status_t GetInstancesFor(
				media_addon_id addon,
				int32 flavor,
				media_node_id * out_id,
				int32 * io_count = 0);	//	default to 1


		status_t SetRealtimeFlags(
				uint32 in_enabled);
		status_t GetRealtimeFlags(
				uint32 * out_enabled);
		ssize_t AudioBufferSizeFor(
				int32 channel_count,
				uint32 sample_format,
				float frame_rate,
				bus_type bus_kind);

		/* Use MediaFlags to inquire about specific features of the Media Kit. */
		/* Returns < 0 for "not present", positive size for output data size. */
		/* 0 means that the capability is present, but no data about it. */
static	ssize_t MediaFlags(
				media_flags cap,
				void * buf,
				size_t maxSize);

		/* BLooper overrides */
virtual		void MessageReceived(
				BMessage * message);

virtual		bool QuitRequested();

virtual		BHandler * ResolveSpecifier(
				BMessage *msg,
				int32 index,
				BMessage *specifier,
				int32 form,
				const char *property);
virtual		status_t GetSupportedSuites(
				BMessage *data);

			~BMediaRoster();

private:

		//	deprecated call
		status_t SetOutputBuffersFor(
				const media_source & output,
				BBufferGroup * group,
				bool will_reclaim = false);

		/* FBC stuffing (Mmmh, Stuffing!) */
virtual		status_t _Reserved_MediaRoster_0(void *);
virtual		status_t _Reserved_MediaRoster_1(void *);
virtual		status_t _Reserved_MediaRoster_2(void *);
virtual		status_t _Reserved_MediaRoster_3(void *);
virtual		status_t _Reserved_MediaRoster_4(void *);
virtual		status_t _Reserved_MediaRoster_5(void *);
virtual		status_t _Reserved_MediaRoster_6(void *);
virtual		status_t _Reserved_MediaRoster_7(void *);

friend class _DefaultDeleter;
friend class _BMediaRosterP;
friend class _HostApp;
friend class MLatentManager;
friend class BBufferProducer;
friend class media_node;
friend class BBuffer;
friend class BMediaNode;

static	bool _isMediaServer;

		BMediaRoster();

static	port_id _mReplyPort;
static	int32 _mReplyPortRes;
static	int32 _mReplyPortUnavailCount;

		uint32 _reserved_media_roster_[67];


static	BMediaRoster * _sDefault;

static	status_t ParseCommand(
				BMessage & reply);

		status_t GetDefaultInfo(
				media_node_id for_default,
				BMessage & out_config);
		status_t SetRunningDefault(
				media_node_id for_default,
				const media_node & node);
		
static	port_id checkout_reply_port(
				const char * name = NULL);
static	void checkin_reply_port(
				port_id port);

};


#endif /* _MEDIA_ROSTER_H */


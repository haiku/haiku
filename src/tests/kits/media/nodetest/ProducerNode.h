#include <BufferProducer.h>
#include <MediaEventLooper.h>

class ProducerNode : public virtual BBufferProducer, BMediaEventLooper
{
public:
	ProducerNode();
	~ProducerNode();

protected:

	/* functionality of BMediaNode */
virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;

virtual		void NodeRegistered();

	/* functionality of BBufferProducer */
virtual	status_t FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format);
virtual	status_t FormatProposal(
				const media_source & output,
				media_format * format);
virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * _deprecated_);
virtual	status_t GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output);
virtual	status_t DisposeOutputCookie(
				int32 cookie);
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group);
virtual	status_t VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * _deprecated_);
virtual	status_t GetLatency(
				bigtime_t * out_lantency);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name);
virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name);
virtual	void Disconnect(
				const media_source & what,
				const media_destination & where);
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * _deprecated_);

virtual status_t HandleMessage(int32 message,
				const void *data, size_t size);

virtual	void AdditionalBufferRequested(	
				const media_source & source,
				media_buffer_id prev_buffer,
				bigtime_t prev_time,
				const media_seek_tag * prev_tag);

virtual	void LatencyChanged(
				const media_source & source,
				const media_destination & destination,
				bigtime_t new_latency,
				uint32 flags);
				
	/* functionality of BMediaEventLooper */
virtual void HandleEvent(const media_timed_event *event,
						 bigtime_t lateness,
						 bool realTimeEvent = false);

	/* our own functionality */
void InitializeOutput();

static int32 _bufferproducer(void *arg);
void BufferProducer();

	BBufferGroup *mBufferGroup;
	sem_id		mBufferProducerSem;
	thread_id	mBufferProducer;

	bool 		 mOutputEnabled;
	media_output mOutput;
};



#include <BufferConsumer.h>
#include <MediaEventLooper.h>

class ConsumerNode : public virtual BBufferConsumer, BMediaEventLooper
{
public:
	ConsumerNode();
	~ConsumerNode();

protected:
	/* functionality of BMediaNode */
virtual		void NodeRegistered();
	/* BBufferConsumer */
virtual	status_t AcceptFormat(
				const media_destination & dest,
				media_format * format);
virtual	status_t GetNextInput(
				int32 * cookie,
				media_input * out_input);
virtual	void DisposeInputCookie(
				int32 cookie);
virtual	void BufferReceived(
				BBuffer * buffer);
virtual	void ProducerDataStatus(
				const media_destination & for_whom,
				int32 status,
				bigtime_t at_performance_time);
virtual	status_t GetLatencyFor(
				const media_destination & for_whom,
				bigtime_t * out_latency,
				media_node_id * out_timesource);
virtual	status_t Connected(
				const media_source & producer,
				const media_destination & where,
				const media_format & with_format,
				media_input * out_input);
virtual	void Disconnected(
				const media_source & producer,
				const media_destination & where);
virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 change_tag,
				const media_format & format);

virtual status_t HandleMessage(int32 message,
				const void *data, size_t size);

virtual status_t SeekTagRequested(
				const media_destination& destination,
				bigtime_t in_target_time,
				uint32 in_flags,
				media_seek_tag* out_seek_tag,
				bigtime_t* out_tagged_time,
				uint32* out_flags);
				
/* from BMediaNode */
virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;

/* from BMediaEventLooper */
virtual void HandleEvent(const media_timed_event *event,
						 bigtime_t lateness,
						 bool realTimeEvent = false);

/* our own functionality */
void InitializeInput();

	media_input mInput;
	media_format mPreferredFormat;
};

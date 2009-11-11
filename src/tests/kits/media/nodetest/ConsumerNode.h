
#include <BufferConsumer.h>
#include <MediaEventLooper.h>


class ConsumerNode : public virtual BBufferConsumer, BMediaEventLooper {
public:
								ConsumerNode();
	virtual						~ConsumerNode();

protected:
	/* functionality of BMediaNode */
	virtual	void				NodeRegistered();

	/* BBufferConsumer */
	virtual	status_t			AcceptFormat(const media_destination& dest,
									media_format* format);
	virtual	status_t			GetNextInput(int32* cookie,
									media_input* _input);
	virtual	void				DisposeInputCookie(int32 cookie);
	virtual	void				BufferReceived(BBuffer* buffer);
	virtual	void				ProducerDataStatus(
									const media_destination& forWhom,
									int32 status, bigtime_t atPerformanceTime);
	virtual	status_t			GetLatencyFor(const media_destination& forWhom,
									bigtime_t* _latency,
									media_node_id* _timesource);
	virtual	status_t			Connected(const media_source& producer,
									const media_destination& where,
									const media_format& withFormat,
									media_input* _input);
	virtual	void				Disconnected(const media_source& producer,
									const media_destination& where);
	virtual	status_t			FormatChanged(const media_source& producer,
									const media_destination& consumer,
									int32 changeTag,
									const media_format& format);

	virtual status_t			HandleMessage(int32 message,
									const void* data, size_t size);

	virtual status_t			SeekTagRequested(
									const media_destination& destination,
									bigtime_t targetTime, uint32 flags,
									media_seek_tag* _seekTag,
									bigtime_t* _taggedTime, uint32* _flags);

	/* from BMediaNode */
	virtual	BMediaAddOn*		AddOn(int32* internalID) const;

	/* from BMediaEventLooper */
	virtual void				HandleEvent(const media_timed_event* event,
							 		bigtime_t lateness,
							 		bool realTimeEvent = false);

	/* our own functionality */
	void InitializeInput();

private:
	media_input fInput;
};

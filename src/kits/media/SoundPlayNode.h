#ifndef _SOUND_PLAY_NODE_
#define _SOUND_PLAY_NODE_

/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SoundPlayNode.h
 *  DESCR: This is the BBufferProducer, used internally by BSoundPlayer
 *         This belongs into a private namespace, but isn't for 
 *         compatibility reasons.
 ***********************************************************************/

class _SoundPlayNode : public BBufferProducer
{
public:
	_SoundPlayNode(const char *name, const media_multi_audio_format *format, BSoundPlayer *player);
	~_SoundPlayNode();

	media_multi_audio_format Format() const;
	
private:

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

	virtual	BMediaAddOn* AddOn(
				int32 * internal_id) const;

public:
	void Start();
	void Stop();
				
private:
	int fFramesPerBuffer;
	thread_id fThreadId;
	volatile bool fStopThread;
	static int32 threadstart(void *arg);
	void PlayThread();
	media_multi_audio_format fFormat;
	BSoundPlayer *fPlayer;
};

#endif

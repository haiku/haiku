#ifndef _MIXER_CORE_H
#define _MIXER_CORE_H

#include <Locker.h>

class MixerInput;
class MixerOutput;

class MixerCore
{
public:
	MixerCore();
	~MixerCore();
	
	bool AddInput(const media_input &input);
	bool AddOutput(const media_output &output);

	bool RemoveInput(int32 inputID);
	bool RemoveOutput();
	
	int32 CreateInputID();
	
	MixerInput *Input(int index); // index = 0 to count-1, NOT inputID
	MixerOutput *Output();
	
	void Lock();
	void Unlock();
	
	void BufferReceived(BBuffer *buffer, bigtime_t lateness);
	
	void InputFormatChanged(int32 inputID, const media_multi_audio_format &format);
	void OutputFormatChanged(const media_multi_audio_format &format);

	void SetOutputBufferGroup(BBufferGroup *group);
	void SetTimeSource(BTimeSource *ts);
	void EnableOutput(bool enabled);
	void Start(bigtime_t time);
	void Stop();

	uint32 OutputBufferSize();
	bool IsStarted();
	
	uint32 OutputChannelCount();

private:
	static int32 _mix_thread_(void *arg);
	void MixThread();
	
private:
	BLocker		*fLocker;
	
	BList		*fInputs;
	MixerOutput	*fOutput;
	int32		fNextInputID;
	bool		fRunning;
	
	float		*fMixBuffer;
	int32		fMixBufferFrameRate;
	int32		fMixBufferFrameCount;
	int32		fMixBufferChannelCount;
	int32		*fMixBufferChannelTypes; //array
	bool		fDoubleRateMixing;
	bigtime_t	fLastMixStartTime;
	
	BBufferGroup *fBufferGroup;
	BTimeSource	*fTimeSource;
	thread_id	fMixThread;
	sem_id		fMixThreadWaitSem;
};


inline void MixerCore::Lock()
{
	fLocker->Lock();
}

inline void MixerCore::Unlock()
{
	fLocker->Unlock();
}

#endif

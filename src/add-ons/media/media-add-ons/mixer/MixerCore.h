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
	
	void InputFormatChanged(int32 inputID, const media_format *format);
	void OutputFormatChanged(const media_format *format);

private:
	void OutputBufferLengthChanged(bigtime_t length);
	
	
private:
	BLocker *fLocker;
	
	bigtime_t	fOutputBufferLength;
	bigtime_t	fInputBufferLength;
	
	BList		*fInputs;
	MixerOutput	*fOutput;
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


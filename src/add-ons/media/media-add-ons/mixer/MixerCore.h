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
	bool RemoveInput(const media_input &input);
	bool RemoveOutput(const media_output &output);
	
	void Lock();
	void Unlock();
	
private:
	BLocker *fLocker;
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


/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: !missing_symbols.cpp
 *  DESCR: Some undocumented symboles that libmedia.so exports.
 *         All return codes are guessed!!! (void most times means it's still wrong)
 ***********************************************************************/
#include <MediaTrack.h>
#include "debug.h"

/*

used by libgame.so
BPrivate::BTrackReader::CountFrames(void)
BPrivate::BTrackReader::Format(void) const
BPrivate::BTrackReader::FrameSize(void)
BPrivate::BTrackReader::ReadFrames(void *, long)
BPrivate::BTrackReader::SeekToFrame(long long *)
BPrivate::BTrackReader::Track(void)
BPrivate::BTrackReader::~BTrackReader(void)
BPrivate::BTrackReader::BTrackReader(BMediaTrack *, media_raw_audio_format const &)
BPrivate::BTrackReader::BTrackReader(BFile *, media_raw_audio_format const &)
BPrivate::dec_load_hook(void *, long)
BPrivate::extractor_load_hook(void *, long)
rtm_create_pool_etc
rtm_get_pool

used by libmidi.so
BSubscriber::IsInStream(void) const
BSubscriber::BSubscriber(char const *)
00036a94 B BSubscriber type_info node
BDACStream::SamplingRate(float *) const
BDACStream::SetSamplingRate(float)
BDACStream::BDACStream(void)
00036a88 B BDACStream type_info node 

used by BeIDE
BSubscriber::EnterStream(_sub_info *, bool, void *, bool (*)(void *, char *, unsigned long, void *), long (*)(void *, long), bool)
BSubscriber::ExitStream(bool)
BSubscriber::ProcessLoop(void)
BSubscriber::Subscribe(BAbstractBufferStream *)
BSubscriber::Unsubscribe(void)
BSubscriber::~BSubscriber(void)
BSubscriber::_ReservedSubscriber1(void)
BSubscriber::_ReservedSubscriber2(void)
BSubscriber::_ReservedSubscriber3(void)
BSubscriber::BSubscriber(char const *)
001132c0 W BSubscriber type_info function
00180484 B BSubscriber type_info node 
BDACStream::SetSamplingRate(float)
BDACStream::~BDACStream(void)
BDACStream::BDACStream(void)
00180478 B BDACStream type_info node 

used by 3dmiX
BDACStream::SetSamplingRate(float)
BDACStream::BDACStream(void)
000706c4 B BDACStream type_info node 
BSubscriber::BSubscriber(char const *)
000706d0 B BSubscriber type_info node  
*/

struct _sub_info
{
	uint32 dummy;
};

namespace BPrivate
{

int32 media_debug; /* is this a function, or a bool, or an int32 ???  */

//BPrivate::BTrackReader move to TrackReader.h & TrackReader.cpp

/*

Already in MediaFormats.cpp

void dec_load_hook(void *, long);
void extractor_load_hook(void *, long);

void dec_load_hook(void *, long)
{
}

void extractor_load_hook(void *, long)
{
}
*/

};

class _BMediaRosterP
{
	void AddCleanupFunction(void (*)(void *), void *);
	void RemoveCleanupFunction(void (*)(void *), void *);
};

void _BMediaRosterP::AddCleanupFunction(void (*)(void *), void *)
{
	UNIMPLEMENTED();
}

void _BMediaRosterP::RemoveCleanupFunction(void (*)(void *), void *)
{
	UNIMPLEMENTED();
}

extern "C" {

int MIDIisInitializingWorkaroundForDoom;
/*

these two moved to RealtimeAlloc.cpp

void rtm_create_pool_etc(void);
int32 rtm_get_pool(int32,int32 **P);

void rtm_create_pool_etc(void)
{
	UNIMPLEMENTED();
}

int32 rtm_get_pool(int32,int32 **p)
{
	UNIMPLEMENTED();
	*p = (int32*)0x1199;
	return B_OK;
}

*/


}


/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: Sound.cpp
 *  DESCR: 
 ***********************************************************************/

#include <Sound.h>
#include "debug.h"

/*************************************************************
 * public BSound
 *************************************************************/

BSound::BSound(	void * data,
				size_t size,
				const media_raw_audio_format & format,
				bool free_when_done = false)
{
	UNIMPLEMENTED();
}

BSound::BSound(	const entry_ref * sound_file,
				bool load_into_memory = false)
{
	UNIMPLEMENTED();
}

status_t 
BSound::InitCheck()
{
	UNIMPLEMENTED();

	return B_OK;
}

BSound * 
BSound::AcquireRef()
{
	UNIMPLEMENTED();
	return 0;
}

bool 
BSound::ReleaseRef()
{
	UNIMPLEMENTED();
	return 0;
}

int32 
BSound::RefCount() const
{
	UNIMPLEMENTED();
	return 0;
}	// unreliable!

/* virtual */ bigtime_t 
BSound::Duration() const
{
	UNIMPLEMENTED();
	return 0;
}

/* virtual */ const media_raw_audio_format & 
BSound::Format() const
{
	UNIMPLEMENTED();	
	return _m_format;
}

/* virtual */ const void * 
BSound::Data() const
{
	UNIMPLEMENTED();
	return 0;
}	/* returns NULL for files */

/* virtual */ off_t 
BSound::Size() const
{
	UNIMPLEMENTED();
	return 0;
}

/* virtual */ bool 
BSound::GetDataAt(off_t offset,
				  void * into_buffer,
				  size_t buffer_size,
				  size_t * out_used)
{
	UNIMPLEMENTED();
	return 0;
}

/*************************************************************
 * protected BSound
 *************************************************************/

BSound::BSound(const media_raw_audio_format & format)
{
	UNIMPLEMENTED();
}

/* virtual */ status_t 
BSound::Perform(int32 code,...)
{
	UNIMPLEMENTED();
	return 0;
}

/*************************************************************
 * private BSound
 *************************************************************/

/*
BSound::BSound(const BSound &);	//	unimplemented
BSound & BSound::operator=(const BSound &);	//	unimplemented
*/

void 
BSound::Reset()
{
	UNIMPLEMENTED();
}

/* virtual */ 
BSound::~BSound()
{
	UNIMPLEMENTED();
}

void 
BSound::free_data()
{
	UNIMPLEMENTED();
}

/* static */ status_t 
BSound::load_entry(void * arg)
{
	UNIMPLEMENTED();
	return 0;
}

void 
BSound::loader_thread()
{
	UNIMPLEMENTED();
}

bool 
BSound::check_stop()
{
	UNIMPLEMENTED();
	return 0;
}

/*************************************************************
 * public BSound
 *************************************************************/

/* virtual */ status_t 
BSound::BindTo(BSoundPlayer * player,
			   const media_raw_audio_format & format)
{
	UNIMPLEMENTED();
	return 0;
}

/* virtual */ status_t 
BSound::UnbindFrom(BSoundPlayer * player)
{
	UNIMPLEMENTED();
	return 0;
}

/*************************************************************
 * private BSound
 *************************************************************/

status_t BSound::_Reserved_Sound_0(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_1(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_2(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_3(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_4(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_5(void *) { return B_ERROR; }


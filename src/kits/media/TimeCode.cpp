/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TimeCode.cpp
 *  DESCR: 
 ***********************************************************************/
#include <TimeCode.h>
#include "debug.h"

status_t us_to_timecode(bigtime_t micros, int * hours, int * minutes, int * seconds, int * frames, const timecode_info * code = NULL)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

status_t timecode_to_us(int hours, int minutes, int seconds, int frames, bigtime_t * micros, const timecode_info * code = NULL)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

status_t frames_to_timecode(int32 l_frames, int * hours, int * minutes, int * seconds, int * frames, const timecode_info * code = NULL)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

status_t timecode_to_frames(int hours, int minutes, int seconds, int frames, int32 * l_frames, const timecode_info * code = NULL)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

status_t get_timecode_description(timecode_type type, timecode_info * out_timecode)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

status_t count_timecodes()
{
	UNIMPLEMENTED();
	return B_ERROR;
}

/*************************************************************
 * public BTimeCode
 *************************************************************/


BTimeCode::BTimeCode()
{
	UNIMPLEMENTED();
}


BTimeCode::BTimeCode(bigtime_t us,
					 timecode_type type)
{
	UNIMPLEMENTED();
}


BTimeCode::BTimeCode(const BTimeCode &clone)
{
	UNIMPLEMENTED();
}


BTimeCode::BTimeCode(int hours,
					 int minutes,
					 int seconds,
					 int frames,
					 timecode_type type)
{
	UNIMPLEMENTED();
}


BTimeCode::~BTimeCode()
{
	UNIMPLEMENTED();
}


void
BTimeCode::SetData(int hours,
				   int minutes,
				   int seconds,
				   int frames)
{
	UNIMPLEMENTED();
}


status_t
BTimeCode::SetType(timecode_type type)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


void
BTimeCode::SetMicroseconds(bigtime_t us)
{
	UNIMPLEMENTED();
}


void
BTimeCode::SetLinearFrames(int32 linear_frames)
{
	UNIMPLEMENTED();
}


BTimeCode &
BTimeCode::operator=(const BTimeCode &clone)
{
	UNIMPLEMENTED();
	return *this;
}


bool
BTimeCode::operator==(const BTimeCode &other) const
{
	UNIMPLEMENTED();

	return false;
}


bool
BTimeCode::operator<(const BTimeCode &other) const
{
	UNIMPLEMENTED();

	return false;
}


BTimeCode &
BTimeCode::operator+=(const BTimeCode &other)
{
	UNIMPLEMENTED();
	return *this;
}


BTimeCode &
BTimeCode::operator-=(const BTimeCode &other)
{
	UNIMPLEMENTED();
	return *this;
}


BTimeCode
BTimeCode::operator+(const BTimeCode &other) const
{
	UNIMPLEMENTED();
	BTimeCode tc(*this);
	tc += other;
	return tc;
}


BTimeCode
BTimeCode::operator-(const BTimeCode &other) const
{
	UNIMPLEMENTED();
	BTimeCode tc(*this);
	tc -= other;
	return tc;
}


int
BTimeCode::Hours() const
{
	UNIMPLEMENTED();

	return 0;
}


int
BTimeCode::Minutes() const
{
	UNIMPLEMENTED();

	return 0;
}


int
BTimeCode::Seconds() const
{
	UNIMPLEMENTED();

	return 0;
}


int
BTimeCode::Frames() const
{
	UNIMPLEMENTED();

	return 0;
}


timecode_type
BTimeCode::Type() const
{
	UNIMPLEMENTED();

	return B_TIMECODE_DEFAULT;
}


void
BTimeCode::GetData(int *out_hours,
				   int *out_minutes,
				   int *out_seconds,
				   int *out_frames,
				   timecode_type *out_type) const
{
	UNIMPLEMENTED();
}


bigtime_t
BTimeCode::Microseconds() const
{
	UNIMPLEMENTED();

	return 0;
}


int32
BTimeCode::LinearFrames() const
{
	UNIMPLEMENTED();

	return 0;
}


void
BTimeCode::GetString(char *str) const
{
	UNIMPLEMENTED();
}



/***********************************************************************
 * AUTHOR: Marcus Overhagen, David McPaul 
 *   FILE: TimeCode.cpp
 *  DESCR: 
 ***********************************************************************/
#include <TimeCode.h>
#include "debug.h"
#include <string.h>

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
	CALLED();
	return 8;	// Is this right?
}

/*************************************************************
 * public BTimeCode
 *************************************************************/


BTimeCode::BTimeCode()
{
	CALLED();
}


BTimeCode::BTimeCode(bigtime_t us,
					 timecode_type type)
{
	CALLED();
	if (SetType(type) == B_OK) {
		SetMicroseconds(us);
	}
}


BTimeCode::BTimeCode(const BTimeCode &clone)
{
	CALLED();
	if (SetType(clone.Type()) == B_OK) {
		SetData(clone.Hours(),clone.Minutes(),clone.Seconds(),clone.Frames());
	}
}


BTimeCode::BTimeCode(int hours,
					 int minutes,
					 int seconds,
					 int frames,
					 timecode_type type)
{
	CALLED();
	if (SetType(type) == B_OK) {
		SetData(hours,minutes,seconds,frames);
	}
}


BTimeCode::~BTimeCode()
{
	CALLED();
}


void
BTimeCode::SetData(int hours,
				   int minutes,
				   int seconds,
				   int frames)
{
	CALLED();
	m_hours = hours;
	m_minutes = minutes;
	m_seconds = seconds;
	m_frames = frames;
}


status_t
BTimeCode::SetType(timecode_type type)
{
	CALLED();
	m_info.type = type;
	strncpy(m_info.format,"%.2i:%.2i:%.2i:%.2i",31);
	m_info.every_nth = 0;
	m_info.except_nth = 0;

	switch (type) {
		case	B_TIMECODE_100:
				strncpy(m_info.name,"TIMECODE 100",31);
				m_info.fps_div = 100;
				break;
		case	B_TIMECODE_75:			// CD 
				strncpy(m_info.name,"CD",31);
				m_info.fps_div = 75;
				break;
		case	B_TIMECODE_30:			// MIDI
				strncpy(m_info.name,"MIDI",31);
				m_info.fps_div = 30;
				break;
		case	B_TIMECODE_30_DROP_2:	// NTSC
				strncpy(m_info.name,"NTSC",31);
				m_info.fps_div = 30;	// This is supposed to be 29.97fps
				m_info.drop_frames = 2; // Drop 2 frames every minute except every 10 minutes
				break;
		case	B_TIMECODE_30_DROP_4:	// Brazil
				strncpy(m_info.name,"NTSC Brazil",31);
				m_info.fps_div = 30;
				m_info.drop_frames = 4; // Drop 4 frames every minute except every 10 minutes
				break;
		case	B_TIMECODE_25:			// PAL
				strncpy(m_info.name,"PAL",31);
				m_info.fps_div = 25;
				break;
		case	B_TIMECODE_24:			// Film
				strncpy(m_info.name,"FILM",31);
				m_info.fps_div = 24;
				break;
		case	B_TIMECODE_18:			// Super8
				strncpy(m_info.name,"Super 8",31);
				m_info.fps_div = 18;
				break;
		default:
				SetType(B_TIMECODE_25);	// PAL default :-)
				break;
	}

	return B_OK;
}


void
BTimeCode::SetMicroseconds(bigtime_t us)
{
// Does not handle Drop formats correctly
	CALLED();
	
	m_seconds = us / 1000;
	m_frames = (us % 1000) / m_info.fps_div;

	// Now normalise to Hours Minutes Seconds
	m_minutes = m_seconds / 60;
	m_seconds = m_seconds % 60;
	
	m_hours = m_minutes / 60;
	m_minutes = m_minutes % 60;
}


void
BTimeCode::SetLinearFrames(int32 linear_frames)
{
// Does not handle Drop formats correctly

	CALLED();
	// First convert frames to seconds leaving the remainder as frames
	
	m_seconds = linear_frames / m_info.fps_div;		// DIV
	m_frames = linear_frames % m_info.fps_div;		// MOD
	
	// Now normalise to Hours Minutes Seconds
	m_minutes = m_seconds / 60;
	m_seconds = m_seconds % 60;
	
	m_hours = m_minutes / 60;
	m_minutes = m_minutes % 60;
}


BTimeCode &
BTimeCode::operator=(const BTimeCode &clone)
{
	CALLED();
	m_hours = clone.Hours();
	m_minutes = clone.Minutes();
	m_seconds = clone.Seconds();
	m_frames = clone.Frames();
	SetType(clone.Type());
	
	return *this;
}


bool
BTimeCode::operator==(const BTimeCode &other) const
{
	CALLED();

	return ((this->LinearFrames()) == (other.LinearFrames()));
}


bool
BTimeCode::operator<(const BTimeCode &other) const
{
	CALLED();

	return ((this->LinearFrames()) < (other.LinearFrames()));
}


BTimeCode &
BTimeCode::operator+=(const BTimeCode &other)
{
	CALLED();
	
	this->SetLinearFrames(this->LinearFrames() + other.LinearFrames());
	
	return *this;
}


BTimeCode &
BTimeCode::operator-=(const BTimeCode &other)
{
	CALLED();
	
	this->SetLinearFrames(this->LinearFrames() - other.LinearFrames());
	
	return *this;
}


BTimeCode
BTimeCode::operator+(const BTimeCode &other) const
{
	CALLED();
	BTimeCode tc(*this);
	tc += other;
	return tc;
}


BTimeCode
BTimeCode::operator-(const BTimeCode &other) const
{
	CALLED();
	BTimeCode tc(*this);
	tc -= other;
	return tc;
}


int
BTimeCode::Hours() const
{
	CALLED();

	return m_hours;
}


int
BTimeCode::Minutes() const
{
	CALLED();

	return m_minutes;
}


int
BTimeCode::Seconds() const
{
	CALLED();

	return m_seconds;
}


int
BTimeCode::Frames() const
{
	CALLED();

	return m_frames;
}


timecode_type
BTimeCode::Type() const
{
	CALLED();

	return m_info.type;
}


void
BTimeCode::GetData(int *out_hours,
				   int *out_minutes,
				   int *out_seconds,
				   int *out_frames,
				   timecode_type *out_type) const
{
	CALLED();
	*out_hours = m_hours;
	*out_minutes = m_minutes;
	*out_seconds = m_seconds;
	*out_frames = m_frames;
	*out_type = m_info.type;
}


bigtime_t
BTimeCode::Microseconds() const
{
	CALLED();

	bigtime_t us;
	
	us = m_hours*60+m_minutes;
	us = us*60+m_seconds;
	us = us*1000+m_frames*m_info.fps_div;
	
	return us;
}


int32
BTimeCode::LinearFrames() const
{
	CALLED();

	int32 linear_frames;
	
	linear_frames = (m_hours * 60) + m_minutes;
	linear_frames = (linear_frames * 60) + m_seconds;
	linear_frames = (linear_frames * m_info.fps_div) + m_frames;
	
	return linear_frames;
}


void
BTimeCode::GetString(char *str) const
{
	CALLED();
	sprintf(str,m_info.format, m_hours, m_minutes, m_seconds, m_frames);
}



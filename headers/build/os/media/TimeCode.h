/*****************************************************************

        File:         TimeCode.h

        Description:  Time code handling functions and class

        Copyright 1998-, Be Incorporated. All Rights Reserved.

*****************************************************************/

#if !defined(_TIME_CODE_H)
#define _TIME_CODE_H

#include <SupportDefs.h>


/* Time code is always in the form HH:MM:SS:FF, it's the definition of FF that varies */
enum timecode_type {
	B_TIMECODE_DEFAULT,
	B_TIMECODE_100,
	B_TIMECODE_75,			/* CD */
	B_TIMECODE_30,			/* MIDI */
	B_TIMECODE_30_DROP_2,	/* NTSC */
	B_TIMECODE_30_DROP_4,	/* Brazil */
	B_TIMECODE_25,			/* PAL */
	B_TIMECODE_24,			/* Film */
	B_TIMECODE_18			/* Super8 */
};
struct timecode_info {
	timecode_type type;
	int drop_frames;
	int every_nth;
	int except_nth;
	int fps_div;
	char name[32];		/* for popup menu etc */
	char format[32];	/* for sprintf(fmt, h, m, s, f) */
	char _reserved_[64];
};
_IMPEXP_MEDIA status_t us_to_timecode(bigtime_t micros, int * hours, int * minutes, int * seconds, int * frames, const timecode_info * code = NULL);
_IMPEXP_MEDIA status_t timecode_to_us(int hours, int minutes, int seconds, int frames, bigtime_t * micros, const timecode_info * code = NULL);
_IMPEXP_MEDIA status_t frames_to_timecode(int32 l_frames, int * hours, int * minutes, int * seconds, int * frames, const timecode_info * code = NULL);
_IMPEXP_MEDIA status_t timecode_to_frames(int hours, int minutes, int seconds, int frames, int32 * l_frames, const timecode_info * code = NULL);
_IMPEXP_MEDIA status_t get_timecode_description(timecode_type type, timecode_info * out_timecode);
_IMPEXP_MEDIA status_t count_timecodes();
/* we may want a set_default_timecode, too -- but that's bad from a thread standpoint */


class BTimeCode {
public:
		BTimeCode();
		BTimeCode(
				bigtime_t us,
				timecode_type type = B_TIMECODE_DEFAULT);
		BTimeCode(
				const BTimeCode & clone);
		BTimeCode(
				int hours,
				int minutes,
				int seconds,
				int frames,
				timecode_type type = B_TIMECODE_DEFAULT);
		~BTimeCode();

		void SetData(
				int hours,
				int minutes,
				int seconds,
				int frames);
		status_t SetType(
				timecode_type type);
		void SetMicroseconds(
				bigtime_t us);
		void SetLinearFrames(
				int32 linear_frames);

		BTimeCode & operator=(
				const BTimeCode & clone);
		bool operator==(
				const BTimeCode & other) const;
		bool operator<(
				const BTimeCode & other) const;

		BTimeCode & operator+=(
				const BTimeCode & other);
		BTimeCode & operator-=(
				const BTimeCode & other);

		BTimeCode operator+(
				const BTimeCode & other) const;
		BTimeCode operator-(
				const BTimeCode & other) const;

		int Hours() const;
		int Minutes() const;
		int Seconds() const;
		int Frames() const;
		timecode_type Type() const;
		void GetData(
				int * out_hours,
				int * out_minutes,
				int * out_seconds,
				int * out_frames,
				timecode_type * out_type = NULL) const;

		bigtime_t Microseconds() const;
		int32 LinearFrames() const;
		void GetString(
				char * str) const;	/* at least 24 bytes */

private:
		int m_hours;
		int m_minutes;
		int m_seconds;
		int m_frames;
		timecode_info m_info;
};


#endif /* _TIME_CODE_H */

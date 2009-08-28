/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_CODE_H
#define _TIME_CODE_H


#include <SupportDefs.h>


// Time code is always in the form HH:MM:SS:FF, it's the definition of "FF"
// that varies
enum timecode_type {
	B_TIMECODE_DEFAULT,
	B_TIMECODE_100,
	B_TIMECODE_75,			// CD
	B_TIMECODE_30,			// MIDI
	B_TIMECODE_30_DROP_2,	// NTSC
	B_TIMECODE_30_DROP_4,	// Brazil
	B_TIMECODE_25,			// PAL
	B_TIMECODE_24,			// Film
	B_TIMECODE_18			// Super8
};


struct timecode_info {
	timecode_type	type;
	int				drop_frames;
	int				every_nth;
	int				except_nth;
	int				fps_div;
	char			name[32];		// For popup menus and such
	char			format[32];		// For sprintf(fmt, h, m, s, f)

	char			_reserved_[64];
};


status_t us_to_timecode(bigtime_t micros, int* hours, int* minutes,
	int* seconds, int* frames, const timecode_info* code = NULL);

status_t timecode_to_us(int hours, int minutes, int seconds, int frames,
	bigtime_t* micros, const timecode_info* code = NULL);

status_t frames_to_timecode(int32 l_frames, int* hours, int* minutes,
	int* seconds, int* frames, const timecode_info* code = NULL);

status_t timecode_to_frames(int hours, int minutes, int seconds, int frames,
	int32* lFrames, const timecode_info* code = NULL);

status_t get_timecode_description(timecode_type type,
	timecode_info* _timecode);

status_t count_timecodes();


class BTimeCode {
public:
								BTimeCode();
								BTimeCode(bigtime_t microSeconds,
									timecode_type type = B_TIMECODE_DEFAULT);
								BTimeCode(const BTimeCode& other);
								BTimeCode(int hours, int minutes, int seconds,
									int frames,
									timecode_type type = B_TIMECODE_DEFAULT);
								~BTimeCode();

			void				SetData(int hours, int minutes, int seconds,
									int frames);
			status_t			SetType(timecode_type type);
			void				SetMicroseconds(bigtime_t microSeconds);
			void				SetLinearFrames(int32 linearFrames);

			BTimeCode&			operator=(const BTimeCode& other);
			bool				operator==(const BTimeCode& other) const;
			bool				operator<(const BTimeCode& other) const;

			BTimeCode&			operator+=(const BTimeCode& other);
			BTimeCode&			operator-=(const BTimeCode& other);

			BTimeCode			operator+(const BTimeCode& other) const;
			BTimeCode			operator-(const BTimeCode& other) const;

			int					Hours() const;
			int					Minutes() const;
			int					Seconds() const;
			int					Frames() const;
			timecode_type		Type() const;
			void				GetData(int* _hours, int* _minutes,
									int* _seconds, int* _frames,
									timecode_type* _type = NULL) const;

			bigtime_t			Microseconds() const;
			int32				LinearFrames() const;

			// Make sure the passed buffer is at least 24 bytes large.
			void				GetString(char* string) const;

private:
			int					fHours;
			int					fMinutes;
			int					fSeconds;
			int					fFrames;
			timecode_info		fInfo;
};

#endif // _TIME_CODE_H

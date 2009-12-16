/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TIME_COMPUTER_H
#define TIME_COMPUTER_H


#include <SupportDefs.h>


struct TimeComputer {
								TimeComputer();

			void				Init(float frameRate, bigtime_t realBaseTime);
			void				SetFrameRate(float frameRate);

			void				AddTimeStamp(bigtime_t realTime, uint64 frames);

			bigtime_t			RealTime() const	{ return fRealTime; }
			bigtime_t			PerformanceTime() const
									{ return fPerformanceTime; }
			double				Drift() const		{ return fDrift; }

private:
	static	const int32			kEntryCount = 32;

			struct Entry {
				bigtime_t	realTime;
				bigtime_t	performanceTime;
			};

private:
			void				_AddEntry(bigtime_t realTime,
									bigtime_t performanceTime);

private:
			bigtime_t			fRealTime;
			bigtime_t			fPerformanceTime;
			double				fDrift;
			float				fFrameRate;
			double				fUsecsPerFrame;
			bigtime_t			fPerformanceTimeBase;
			uint64				fFrameBase;
			bool				fResetTimeBase;
			Entry				fEntries[kEntryCount];
			int32				fFirstEntry;
			int32				fLastEntry;
};


#endif	// TIME_COMPUTER_H

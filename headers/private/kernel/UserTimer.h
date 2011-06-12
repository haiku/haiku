/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_USER_TIMER_H
#define _KERNEL_USER_TIMER_H


#include <sys/cdefs.h>
#include <time.h>

#include <util/DoublyLinkedList.h>

#include <ksignal.h>
#include <timer.h>
#include <user_timer_defs.h>


struct thread_creation_attributes;


namespace BKernel {


struct UserEvent;
struct Team;


struct UserTimer : DoublyLinkedListLinkImpl<UserTimer> {
								UserTimer();
	virtual						~UserTimer();

			int32				ID() const
									{ return fID; }
			void				SetID(int32 id)
									{ fID = id; }

			void				SetEvent(UserEvent* event)
									{ fEvent = event; }

	virtual	void				Schedule(bigtime_t nextTime, bigtime_t interval,
									uint32 flags, bigtime_t& _oldRemainingTime,
									bigtime_t& _oldInterval) = 0;
			void				Cancel();

	virtual	void				GetInfo(bigtime_t& _remainingTime,
									bigtime_t& _interval,
									uint32& _overrunCount) = 0;

protected:
	static	int32				HandleTimerHook(struct timer* timer);
	virtual	void				HandleTimer();

	inline	void				UpdatePeriodicStartTime();
	inline	void				CheckPeriodicOverrun(bigtime_t now);

protected:
			int32				fID;
			timer				fTimer;
			UserEvent*			fEvent;
			bigtime_t			fNextTime;
			bigtime_t			fInterval;
			uint32				fOverrunCount;
			bool				fScheduled;	// fTimer scheduled
};


struct SystemTimeUserTimer : public UserTimer {
	virtual	void				Schedule(bigtime_t nextTime, bigtime_t interval,
									uint32 flags, bigtime_t& _oldRemainingTime,
									bigtime_t& _oldInterval);
	virtual	void				GetInfo(bigtime_t& _remainingTime,
									bigtime_t& _interval,
									uint32& _overrunCount);

protected:
	virtual	void				HandleTimer();

			void				ScheduleKernelTimer(bigtime_t now,
									bool checkPeriodicOverrun);
};


struct RealTimeUserTimer : public SystemTimeUserTimer {
	virtual	void				Schedule(bigtime_t nextTime, bigtime_t interval,
									uint32 flags, bigtime_t& _oldRemainingTime,
									bigtime_t& _oldInterval);

			void				TimeWarped();

private:
			bigtime_t			fRealTimeOffset;
			bool				fAbsolute;

protected:
	virtual	void				HandleTimer();

public:
			// conceptually package private
			DoublyLinkedListLink<RealTimeUserTimer> fGlobalListLink;
};


struct TeamTimeUserTimer : public UserTimer {
								TeamTimeUserTimer(team_id teamID);
								~TeamTimeUserTimer();

	virtual	void				Schedule(bigtime_t nextTime, bigtime_t interval,
									uint32 flags, bigtime_t& _oldRemainingTime,
									bigtime_t& _oldInterval);
	virtual	void				GetInfo(bigtime_t& _remainingTime,
									bigtime_t& _interval,
									uint32& _overrunCount);

			void				Deactivate();

			void				Update(Thread* unscheduledThread);
			void				TimeWarped(bigtime_t changedBy);

protected:
	virtual	void				HandleTimer();

private:
			void				_Update(bool unscheduling);

private:
			team_id				fTeamID;
			Team*				fTeam;
			int32				fRunningThreads;
			bool				fAbsolute;

public:
			// conceptually package private
			DoublyLinkedListLink<TeamTimeUserTimer> fCPUTimeListLink;
};


struct TeamUserTimeUserTimer : public UserTimer {
								TeamUserTimeUserTimer(team_id teamID);
								~TeamUserTimeUserTimer();

	virtual	void				Schedule(bigtime_t nextTime, bigtime_t interval,
									uint32 flags, bigtime_t& _oldRemainingTime,
									bigtime_t& _oldInterval);
	virtual	void				GetInfo(bigtime_t& _remainingTime,
									bigtime_t& _interval,
									uint32& _overrunCount);

			void				Deactivate();
			void				Check();

private:
			team_id				fTeamID;
			Team*				fTeam;

public:
			// conceptually package private
			DoublyLinkedListLink<TeamUserTimeUserTimer> fCPUTimeListLink;
};


struct ThreadTimeUserTimer : public UserTimer {
								ThreadTimeUserTimer(thread_id threadID);
								~ThreadTimeUserTimer();

	virtual	void				Schedule(bigtime_t nextTime, bigtime_t interval,
									uint32 flags, bigtime_t& _oldRemainingTime,
									bigtime_t& _oldInterval);
	virtual	void				GetInfo(bigtime_t& _remainingTime,
									bigtime_t& _interval,
									uint32& _overrunCount);

			void				Deactivate();

			void				Start();
			void				Stop();
			void				TimeWarped(bigtime_t changedBy);

protected:
	virtual	void				HandleTimer();

private:
			thread_id			fThreadID;
			Thread*				fThread;	// != NULL only when active
			bool				fAbsolute;

public:
			// conceptually package private
			DoublyLinkedListLink<ThreadTimeUserTimer> fCPUTimeListLink;
};


struct UserTimerList {
								UserTimerList();
								~UserTimerList();

			UserTimer*			TimerFor(int32 id) const;
			void				AddTimer(UserTimer* timer);
			void				RemoveTimer(UserTimer* timer)
									{ fTimers.Remove(timer); }
			int32				DeleteTimers(bool userDefinedOnly);

private:
			typedef DoublyLinkedList<UserTimer> TimerList;

private:
			TimerList			fTimers;
};


typedef DoublyLinkedList<RealTimeUserTimer,
	DoublyLinkedListMemberGetLink<RealTimeUserTimer,
		&RealTimeUserTimer::fGlobalListLink> > RealTimeUserTimerList;

typedef DoublyLinkedList<TeamTimeUserTimer,
	DoublyLinkedListMemberGetLink<TeamTimeUserTimer,
		&TeamTimeUserTimer::fCPUTimeListLink> > TeamTimeUserTimerList;

typedef DoublyLinkedList<TeamUserTimeUserTimer,
	DoublyLinkedListMemberGetLink<TeamUserTimeUserTimer,
		&TeamUserTimeUserTimer::fCPUTimeListLink> > TeamUserTimeUserTimerList;

typedef DoublyLinkedList<ThreadTimeUserTimer,
	DoublyLinkedListMemberGetLink<ThreadTimeUserTimer,
		&ThreadTimeUserTimer::fCPUTimeListLink> > ThreadTimeUserTimerList;


}	// namespace BKernel


using BKernel::RealTimeUserTimer;
using BKernel::RealTimeUserTimerList;
using BKernel::SystemTimeUserTimer;
using BKernel::TeamUserTimeUserTimer;
using BKernel::TeamUserTimeUserTimerList;
using BKernel::TeamTimeUserTimer;
using BKernel::TeamTimeUserTimerList;
using BKernel::ThreadTimeUserTimer;
using BKernel::ThreadTimeUserTimerList;
using BKernel::UserTimer;
using BKernel::UserTimerList;


__BEGIN_DECLS

status_t	user_timer_create_thread_timers(Team* team, Thread* thread);
status_t	user_timer_create_team_timers(Team* team);

status_t	user_timer_get_clock(clockid_t clockID, bigtime_t& _time);
void		user_timer_real_time_clock_changed();

void		user_timer_stop_cpu_timers(Thread* thread, Thread* nextThread);
void		user_timer_continue_cpu_timers(Thread* thread,
				Thread* previousThread);
void		user_timer_check_team_user_timers(Team* team);

status_t	_user_get_clock(clockid_t clockID, bigtime_t* _time);
status_t	_user_set_clock(clockid_t clockID, bigtime_t time);

int32		_user_create_timer(clockid_t clockID, thread_id threadID,
				uint32 flags, const struct sigevent* event,
				const thread_creation_attributes* threadAttributes);
status_t	_user_delete_timer(int32 timerID, thread_id threadID);
status_t	_user_get_timer(int32 timerID, thread_id threadID,
				struct user_timer_info* info);
status_t	_user_set_timer(int32 timerID, thread_id threadID,
				bigtime_t startTime, bigtime_t interval, uint32 flags,
				struct user_timer_info* oldInfo);

__END_DECLS


#endif	// _KERNEL_USER_TIMER_H

/* jobs.h -- structures and stuff used by the jobs.c file. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#if !defined (_JOBS_H_)
#  define _JOBS_H_

#include "quit.h"
#include "siglist.h"

#include "stdc.h"

#include "posixwait.h"

/* Defines controlling the fashion in which jobs are listed. */
#define JLIST_STANDARD       0
#define JLIST_LONG	     1
#define JLIST_PID_ONLY	     2
#define JLIST_CHANGED_ONLY   3
#define JLIST_NONINTERACTIVE 4

/* I looked it up.  For pretty_print_job ().  The real answer is 24. */
#define LONGEST_SIGNAL_DESC 24

/* We keep an array of jobs.  Each entry in the array is a linked list
   of processes that are piped together.  The first process encountered is
   the group leader. */

/* Values for the `running' field of a struct process. */
#define PS_DONE		0
#define PS_RUNNING	1
#define PS_STOPPED	2

/* Each child of the shell is remembered in a STRUCT PROCESS.  A chain of
   such structures is a pipeline.  The chain is circular. */
typedef struct process {
  struct process *next;	/* Next process in the pipeline.  A circular chain. */
  pid_t pid;		/* Process ID. */
  WAIT status;		/* The status of this command as returned by wait. */
  int running;		/* Non-zero if this process is running. */
  char *command;	/* The particular program that is running. */
} PROCESS;

/* PRUNNING really means `not exited' */
#define PRUNNING(p)	((p)->running || WIFSTOPPED((p)->status))
#define PSTOPPED(p)	(WIFSTOPPED((p)->status))
#define PDEADPROC(p)	((p)->running == PS_DONE)

/* A description of a pipeline's state. */
typedef enum { JRUNNING, JSTOPPED, JDEAD, JMIXED } JOB_STATE;
#define JOBSTATE(job) (jobs[(job)]->state)

#define STOPPED(j)	(jobs[(j)]->state == JSTOPPED)
#define RUNNING(j)	(jobs[(j)]->state == JRUNNING)
#define DEADJOB(j)	(jobs[(j)]->state == JDEAD)

/* Values for the FLAGS field in the JOB struct below. */
#define J_FOREGROUND 0x01 /* Non-zero if this is running in the foreground.  */
#define J_NOTIFIED   0x02 /* Non-zero if already notified about job state.   */
#define J_JOBCONTROL 0x04 /* Non-zero if this job started under job control. */
#define J_NOHUP      0x08 /* Don't send SIGHUP to job if shell gets SIGHUP. */

#define IS_FOREGROUND(j)	((jobs[j]->flags & J_FOREGROUND) != 0)
#define IS_NOTIFIED(j)		((jobs[j]->flags & J_NOTIFIED) != 0)
#define IS_JOBCONTROL(j)	((jobs[j]->flags & J_JOBCONTROL) != 0)

typedef struct job {
  char *wd;	   /* The working directory at time of invocation. */
  PROCESS *pipe;   /* The pipeline of processes that make up this job. */
  pid_t pgrp;	   /* The process ID of the process group (necessary). */
  JOB_STATE state; /* The state that this job is in. */
  int flags;	   /* Flags word: J_NOTIFIED, J_FOREGROUND, or J_JOBCONTROL. */
#if defined (JOB_CONTROL)
  COMMAND *deferred;	/* Commands that will execute when this job is done. */
  sh_vptrfunc_t *j_cleanup; /* Cleanup function to call when job marked JDEAD */
  PTR_T cleanarg;	/* Argument passed to (*j_cleanup)() */
#endif /* JOB_CONTROL */
} JOB;

#define NO_JOB  -1	/* An impossible job array index. */
#define DUP_JOB -2	/* A possible return value for get_job_spec (). */

/* A value which cannot be a process ID. */
#define NO_PID (pid_t)-1

/* System calls. */
#if !defined (HAVE_UNISTD_H)
extern pid_t fork (), getpid (), getpgrp ();
#endif /* !HAVE_UNISTD_H */

/* Stuff from the jobs.c file. */
extern pid_t original_pgrp, shell_pgrp, pipeline_pgrp;
extern pid_t last_made_pid, last_asynchronous_pid;
extern int current_job, previous_job;
extern int asynchronous_notification;
extern JOB **jobs;
extern int job_slots;

extern void making_children __P((void));
extern void stop_making_children __P((void));
extern void cleanup_the_pipeline __P((void));
extern void save_pipeline __P((int));
extern void restore_pipeline __P((int));
extern void start_pipeline __P((void));
extern int stop_pipeline __P((int, COMMAND *));

extern void delete_job __P((int, int));
extern void nohup_job __P((int));
extern void delete_all_jobs __P((int));
extern void nohup_all_jobs __P((int));

extern int count_all_jobs __P((void));

extern void terminate_current_pipeline __P((void));
extern void terminate_stopped_jobs __P((void));
extern void hangup_all_jobs __P((void));
extern void kill_current_pipeline __P((void));

#if defined (__STDC__) && defined (pid_t)
extern int get_job_by_pid __P((int, int));
extern void describe_pid __P((int));
#else
extern int get_job_by_pid __P((pid_t, int));
extern void describe_pid __P((pid_t));
#endif

extern void list_one_job __P((JOB *, int, int, int));
extern void list_all_jobs __P((int));
extern void list_stopped_jobs __P((int));
extern void list_running_jobs __P((int));

extern pid_t make_child __P((char *, int));

extern int get_tty_state __P((void));
extern int set_tty_state __P((void));

extern int wait_for_single_pid __P((pid_t));
extern void wait_for_background_pids __P((void));
extern int wait_for __P((pid_t));
extern int wait_for_job __P((int));

extern void notify_and_cleanup __P((void));
extern void reap_dead_jobs __P((void));
extern int start_job __P((int, int));
extern int kill_pid __P((pid_t, int, int));
extern int initialize_job_control __P((int));
extern void initialize_job_signals __P((void));
extern int give_terminal_to __P((pid_t, int));

extern void set_sigwinch_handler __P((void));
extern void unset_sigwinch_handler __P((void));

extern void unfreeze_jobs_list __P((void));
extern int set_job_control __P((int));
extern void without_job_control __P((void));
extern void end_job_control __P((void));
extern void restart_job_control __P((void));
extern void set_sigchld_handler __P((void));
extern void ignore_tty_job_signals __P((void));
extern void default_tty_job_signals __P((void));

#if defined (JOB_CONTROL)
extern int job_control;
#endif

#endif /* _JOBS_H_ */

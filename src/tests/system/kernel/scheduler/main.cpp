#include "override_types.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <List.h>
#include <Locker.h>
#include <String.h>
#include <TLS.h>

#include <cpu.h>
#include <kscheduler.h>
#include <timer.h>


const static option kOptions[] = {
	{"time", required_argument, NULL, 't'},
	{"cpu", required_argument, NULL, 'c'},
};

const bigtime_t kQuantum = 3000;
const uint32 kMaxCPUCount = 64;

class Thread {
public:
	Thread(const char* name, int32 priority);
	virtual ~Thread();

	struct thread* GetThread() { return &fThread; }
	virtual void Rescheduled() { fOnCPU[fThread.cpu->cpu_num]++; }

	virtual bigtime_t NextQuantum() { return kQuantum; }
	virtual bigtime_t NextState() { return B_THREAD_READY; }
	virtual bigtime_t NextReady() { return 0; }

protected:
	struct thread	fThread;
	int32			fOnCPU[kMaxCPUCount];
};

class IdleThread : public Thread {
public:
	IdleThread();
	virtual ~IdleThread();
};

class CPU {
public:
	CPU(int32 num);
	~CPU();

	cpu_ent* GetCPU() { return &fCPU; }
	int32 Index() { return fCPU.cpu_num; }

	struct thread* CurrentThread()
		{ return fCurrentThread->GetThread(); }
	void SetCurrentThread(struct thread* thread)
		{ fCurrentThread = thread->object; }

	void Quit();

private:
	void _Run();
	static status_t _Run(void* self);

	struct cpu_ent	fCPU;
	thread_id		fThread;
	Thread*			fCurrentThread;
	IdleThread		fIdleThread;
	int32			fRescheduleCount;
	bool			fQuit;
};

class Timer {
public:
	Timer();
	~Timer();

	void AddTimer(bigtime_t time);
};

thread_queue dead_q;

static BLocker sThreadLock;
static BList sThreads;
static int32 sNextThreadID = 1;
static int32 sCPUIndexSlot;
static CPU* sCPU[kMaxCPUCount];
static size_t sCPUCount;


//	#pragma mark -


Thread::Thread(const char* name, int32 priority)
{
	memset(&fThread, 0, sizeof(struct thread));
	fThread.name = strdup(name);
	fThread.id = sNextThreadID++;
	fThread.priority = fThread.next_priority = priority;
	fThread.state = fThread.next_state = B_THREAD_READY;
	fThread.object = this;
	fThread.last_time = system_time();
	
	memset(&fOnCPU, 0, sizeof(fOnCPU));
}


Thread::~Thread()
{
	printf("  %p %10Ld  %s, prio %ld, cpu:", &fThread, fThread.kernel_time,
		fThread.name, fThread.priority);
	for (uint32 i = 0; i < kMaxCPUCount; i++) {
		if (fOnCPU[i])
			printf(" [%ld] %ld", i, fOnCPU[i]);
	}
	putchar('\n');
	free(fThread.name);
}


//	#pragma mark -


IdleThread::IdleThread()
	: Thread("idle thread", B_IDLE_PRIORITY)
{
}


IdleThread::~IdleThread()
{
}


//	#pragma mark -


CPU::CPU(int32 num)
	:
	fRescheduleCount(0),
	fQuit(false)
{
	memset(&fCPU, 0, sizeof(struct cpu_ent));
	fCPU.cpu_num = num;

	fCurrentThread = &fIdleThread;
	fIdleThread.GetThread()->cpu = &fCPU;

	fThread = spawn_thread(&_Run, (BString("cpu ") << num).String(),
		B_NORMAL_PRIORITY, this);
	resume_thread(fThread);
}


CPU::~CPU()
{
}


void
CPU::Quit()
{
	fQuit = true;

	status_t status;
	wait_for_thread(fThread, &status);
}


void
CPU::_Run()
{
	tls_set(sCPUIndexSlot, this);
	printf("CPU %d has started.\n", fCPU.cpu_num);

	while (!fQuit) {
		fRescheduleCount++;

		grab_thread_lock();
		scheduler_reschedule();
		release_thread_lock();

		fCurrentThread->Rescheduled();
		snooze(fCurrentThread->NextQuantum());
		fCurrentThread->GetThread()->next_state = fCurrentThread->NextState();
	}

	thread_info info;
	get_thread_info(find_thread(NULL), &info);

	printf("CPU %d has halted, user %Ld, %ld rescheduled.\n",
		fCPU.cpu_num, info.user_time, fRescheduleCount);
	delete this;
}


status_t
CPU::_Run(void* self)
{
	CPU* cpu = (CPU*)self;
	cpu->_Run();
	return B_OK;
}


//	#pragma mark - Emulation


extern "C" void
kprintf(const char *format,...)
{
	va_list args;
	va_start(args, format);
	printf("\33[35m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


void
thread_enqueue(struct thread *t, struct thread_queue *q)
{
}


struct thread *
thread_get_current_thread(void)
{
	CPU* cpu = (CPU*)tls_get(sCPUIndexSlot);
	return cpu->CurrentThread();
}


void
grab_thread_lock(void)
{
	sThreadLock.Lock();
}


void
release_thread_lock(void)
{
	sThreadLock.Unlock();
}


void
arch_thread_context_switch(struct thread *t_from, struct thread *t_to)
{
}


void
arch_thread_set_current_thread(struct thread *thread)
{
	CPU* cpu = (CPU*)tls_get(sCPUIndexSlot);
	//printf("[%ld]  %p  %s\n", cpu->Index(), thread, thread->name);
	return cpu->SetCurrentThread(thread);
}


int
add_debugger_command(char *name, int (*func)(int, char **), char *desc)
{
	return B_OK;
}


cpu_status
disable_interrupts()
{
	return 0;
}


void
restore_interrupts(cpu_status status)
{
}


extern "C" status_t
_local_timer_cancel_event(int cpu, timer *event)
{
	return B_OK;
}


status_t
add_timer(timer *event, timer_hook hook, bigtime_t period, int32 flags)
{
	return B_OK;
}


int32
smp_get_current_cpu(void)
{
	return thread_get_current_thread()->cpu->cpu_num;
}


//	#pragma mark -


static void
start_cpus(uint32 count)
{
	sCPUIndexSlot = tls_allocate();
	sCPUCount = count;

	for (size_t i = 0; i < count; i++) {
		sCPU[i] = new CPU(i);
	}
}


static void
stop_cpus()
{
	for (size_t i = 0; i < sCPUCount; i++) {
		sCPU[i]->Quit();
	}
}


static void
add_thread(Thread* thread)
{
	grab_thread_lock();
	sThreads.AddItem(thread);
	scheduler_enqueue_in_run_queue(thread->GetThread());
	release_thread_lock();
}


static void
delete_threads()
{
	for (int32 i = 0; i < sThreads.CountItems(); i++) {
		delete (Thread*)sThreads.ItemAt(i);
	}
}


int
main(int argc, char** argv)
{
	bigtime_t runTime = 1000000;
	uint32 cpuCount = 1;

	char option;
	while ((option = getopt_long(argc, argv, "", kOptions, NULL)) != -1) {
		switch (option) {
			case 't':
				runTime *= strtol(optarg, 0, NULL);
				if (runTime <= 0) {
					fprintf(stderr, "Invalid run time.\n");
					exit(1);
				}
				break;
			case 'c':
				cpuCount = strtol(optarg, 0, NULL);
				if (cpuCount <= 0 || cpuCount > 64) {
					fprintf(stderr, "Invalid CPU count (allowed: 1-64).\n");
					exit(1);
				}
				break;
		}
	}

	start_cpus(cpuCount);

	add_thread(new Thread("test 1", 5));
	add_thread(new Thread("test 2", 10));
	add_thread(new Thread("test 3", 15));

	snooze(runTime);

	stop_cpus();
	delete_threads();
	return 0;
}

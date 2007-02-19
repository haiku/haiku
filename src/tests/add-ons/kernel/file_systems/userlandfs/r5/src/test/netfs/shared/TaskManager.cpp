// TaskManager.cpp

#include "TaskManager.h"

// #pragma mark -
// #pragma mark ----- Task -----

// constructor
Task::Task(const char* name)
	: fName(name),
	  fThread(-1),
	  fTerminating(false),
	  fDone(true)
{
}

// destructor
Task::~Task()
{
}

// Run
status_t
Task::Run()
{
	const char* name = (fName.GetLength() > 0 ? fName.GetString() : "task");
#if USER
	fThread = spawn_thread(&_ThreadEntry, name, B_NORMAL_PRIORITY, this);
#else
	fThread = spawn_kernel_thread(&_ThreadEntry, name, B_NORMAL_PRIORITY, this);
#endif
	if (fThread < 0)
		return fThread;
	fDone = false;
	resume_thread(fThread);
	return B_OK;
}

// Terminate
void
Task::Terminate()
{
	fTerminating = true;
	Stop();
	if (fThread >= 0) {
		int32 result;
		wait_for_thread(fThread, &result);
	}
}

// IsDone
bool
Task::IsDone() const
{
	return fDone;
}

// Stop();
void
Task::Stop()
{
}

// SetDone
void
Task::SetDone(bool done)
{
	fDone = 0;
}

// _ThreadEntry
int32
Task::_ThreadEntry(void* data)
{
	return ((Task*)data)->_Thread();
}

// _Thread
int32
Task::_Thread()
{
	Execute();
	return 0;
}


// #pragma mark -
// #pragma mark ----- TaskManager -----

// constructor
TaskManager::TaskManager()
	: fTasks()
{
}

// destructor
TaskManager::~TaskManager()
{
	TerminateAllTasks();
}

// RunTask
status_t
TaskManager::RunTask(Task* task)
{
	if (!task)
		return B_BAD_VALUE;
	fTasks.Insert(task);
	status_t error = task->Run();
	if (error != B_OK) {
		fTasks.Remove(task);
		delete task;
	}
	return error;
}

// RemoveDoneTasks
void
TaskManager::RemoveDoneTasks()
{
	Task* task = fTasks.GetFirst();
	while (task) {
		Task* next = fTasks.GetNext(task);
		if (task->IsDone()) {
			fTasks.Remove(task);
			task->Terminate();
			delete task;
		}
		task = next;
	}
}

// TerminateAllTasks
void
TaskManager::TerminateAllTasks()
{
	while (Task* task = fTasks.GetFirst()) {
		fTasks.Remove(task);
		task->Terminate();
		delete task;
	}
}


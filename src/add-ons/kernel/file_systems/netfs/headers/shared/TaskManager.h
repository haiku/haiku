// TaskManager.h

#ifndef NET_FS_TASK_MANAGER_H
#define NET_FS_TASK_MANAGER_H

#include <OS.h>

#include <HashString.h>
#include <util/DoublyLinkedList.h>

// Task
class Task : public DoublyLinkedListLinkImpl<Task> {
public:
								Task(const char* name);
	virtual						~Task();

			status_t			Run();
			void				Terminate();

			bool				IsDone() const;

	virtual	status_t			Execute() = 0;
	virtual	void				Stop();

protected:
			void				SetDone(bool done);

private:
	static	int32				_ThreadEntry(void* data);
			int32				_Thread();

private:
			HashString			fName;
			thread_id			fThread;
			bool				fTerminating;
			bool				fDone;
};

// TaskManager
class TaskManager {
public:
								TaskManager();
								~TaskManager();

			status_t			RunTask(Task* task);
			void				RemoveDoneTasks();
			void				TerminateAllTasks();

private:
			DoublyLinkedList<Task> fTasks;
};

#endif	// NET_FS_TASK_MANAGER_H

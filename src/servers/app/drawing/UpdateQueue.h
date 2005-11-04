// UpdateQueue.h

#ifndef UPDATE_QUEUE_H
#define UPDATE_QUEUE_H

#include <List.h>
#include <Locker.h>
#include <OS.h>
#include <Region.h>

class HWInterface;

class UpdateQueue : public BLocker {
 public:
 								UpdateQueue(HWInterface* interface);
 	virtual						~UpdateQueue();

			status_t			InitCheck();

			void				AddRect(const BRect& rect);

 private:
	static	int32				_execute_updates_(void *cookie);
			int32				_ExecuteUpdates();

			void				_Reschedule();

			HWInterface*		fInterface;

			BRegion				fUpdateRegion;
			thread_id			fUpdateExecutor;
			sem_id				fThreadControl;
			status_t			fStatus;
};

#endif	// UPDATE_QUEUE_H

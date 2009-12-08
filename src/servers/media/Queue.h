/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef QUEUE_H
#define QUEUE_H


#include <List.h>
#include <Locker.h>


class Queue : BLocker {
public:
								Queue();
								~Queue();

			status_t			Terminate();

			status_t			AddItem(void* item);
			void*				RemoveItem();

private:
			BList				fList;
			sem_id				fSem;
};


#endif	// QUEUE_H

/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

class Queue
{
public:
	Queue();
	~Queue();

	status_t	Terminate();
	
	status_t	AddItem(void *item);
	void *		RemoveItem();
	
private:
	BList		*fList;
	BLocker		*fLocker;
	sem_id		fSem;
};

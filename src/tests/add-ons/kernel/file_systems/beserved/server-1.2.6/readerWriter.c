// readerWriter.c

#include "betalk.h"
#include "readerWriter.h"


void btLock(sem_id semaphore, int32 *atomic)
{
	int32 previous = atomic_add(atomic, 1);
	if (previous >= 1)
		while (acquire_sem(semaphore) == B_INTERRUPTED);
}

void btUnlock(sem_id semaphore, int32 *atomic)
{
	int32 previous = atomic_add(atomic, -1);
	if (previous > 1)
		release_sem(semaphore);
}

bool initManagedData(bt_managed_data *data)
{
	data->readCount = 0;
	data->writeCount = 0;

	data->readCountVar = 0;
	data->writeCountVar = 0;
	data->readerQueueVar = 0;
	data->readerVar = 0;
	data->writerVar = 0;

	if ((data->readCountSem = create_sem(1, "Read Counter")) < 0)
		return false;

	if ((data->writeCountSem = create_sem(1, "Write Counter")) < 0)
	{
		delete_sem(data->readCountSem);
		return false;
	}

	if ((data->readerQueue = create_sem(1, "Read Queue")) < 0)
	{
		delete_sem(data->writeCountSem);
		delete_sem(data->readCountSem);
		return false;
	}

	if ((data->reader = create_sem(1, "Single Reader")) < 0)
	{
		delete_sem(data->readerQueue);
		delete_sem(data->writeCountSem);
		delete_sem(data->readCountSem);
		return false;
	}

	if ((data->writer = create_sem(1, "Writer")) < 0)
	{
		delete_sem(data->reader);
		delete_sem(data->readerQueue);
		delete_sem(data->writeCountSem);
		delete_sem(data->readCountSem);
		return false;
	}

	return true;
}

void closeManagedData(bt_managed_data *data)
{
	data->readCount = data->writeCount = 0;

	delete_sem(data->writer);
	delete_sem(data->reader);
	delete_sem(data->readerQueue);
	delete_sem(data->writeCountSem);
	delete_sem(data->readCountSem);
}

void beginReading(bt_managed_data *data)
{
	btLock(data->readerQueue, &data->readerQueueVar);
	btLock(data->reader, &data->readerVar);
	btLock(data->readCountSem, &data->readCountVar);

	data->readCount++;
	if (data->readCount == 1)
		btLock(data->writer, &data->writerVar);

	btUnlock(data->readCountSem, &data->readCountVar);
	btUnlock(data->reader, &data->readerVar);
	btUnlock(data->readerQueue, &data->readerQueueVar);
}

void endReading(bt_managed_data *data)
{
	btLock(data->readCountSem, &data->readCountVar);

	data->readCount--;
	if (data->readCount == 0)
		btUnlock(data->writer, &data->writerVar);

	btUnlock(data->readCountSem, &data->readCountVar);
}

void beginWriting(bt_managed_data *data)
{
	btLock(data->writeCountSem, &data->writeCountVar);

	data->writeCount++;
	if (data->writeCount == 1)
		btLock(data->reader, &data->readerVar);

	btUnlock(data->writeCountSem, &data->writeCountVar);
	btLock(data->writer, &data->writerVar);
}

void endWriting(bt_managed_data *data)
{
	btUnlock(data->writer, &data->writerVar);
	btLock(data->writeCountSem, &data->writeCountVar);

	data->writeCount--;
	if (data->writeCount == 0)
		btUnlock(data->reader, &data->readerVar);

	btUnlock(data->writeCountSem, &data->writeCountVar);
}

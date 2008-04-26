// readerWriter.h

#ifndef _READERWRITER_H_
#define _READERWRITER_H_

#include "beCompat.h"
#include "betalk.h"

typedef struct managedData
{
	int32 readCount;
	int32 writeCount;

	// These two semaphores control access to the readCount and writeCount
	// variables.
	sem_id readCountSem;
	sem_id writeCountSem;

	// The first process wishes to gain read access blocks on the reader
	// semaphore, but all other block on the readerQueue, so that writers
	// can effectively jump the queue.
	sem_id readerQueue;

	// Semaphores for holding waiting processes.
	sem_id reader;
	sem_id writer;
} bt_managed_data;


bool initManagedData(bt_managed_data *data);
void closeManagedData(bt_managed_data *data);
void beginReading(bt_managed_data *data);
void endReading(bt_managed_data *data);
void beginWriting(bt_managed_data *data);
void endWriting(bt_managed_data *data);

#endif

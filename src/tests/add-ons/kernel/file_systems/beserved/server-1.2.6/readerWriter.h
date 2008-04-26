// readerWriter.h

#include "betalk.h"

typedef struct managedData
{
	int32 readCount;
	int32 writeCount;

	// These two semaphores control access to the readCount and writeCount
	// variables.
	sem_id readCountSem;
	int32 readCountVar;
	sem_id writeCountSem;
	int32 writeCountVar;

	// The first process wishes to gain read access blocks on the reader
	// semaphore, but all other block on the readerQueue, so that writers
	// can effectively jump the queue.
	sem_id readerQueue;
	int32 readerQueueVar;

	// Semaphores for holding waiting processes.
	sem_id reader;
	int32 readerVar;
	sem_id writer;
	int32 writerVar;
} bt_managed_data;


bool initManagedData(bt_managed_data *data);
void closeManagedData(bt_managed_data *data);
void beginReading(bt_managed_data *data);
void endReading(bt_managed_data *data);
void beginWriting(bt_managed_data *data);
void endWriting(bt_managed_data *data);

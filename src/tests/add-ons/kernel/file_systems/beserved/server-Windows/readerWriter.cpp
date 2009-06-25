// readerWriter.c

#include "stdafx.h"

#include "beCompat.h"
#include "betalk.h"
#include "readerWriter.h"


bool initManagedData(bt_managed_data *data)
{
	data->readCount = 0;
	data->writeCount = 0;

	if (!(data->readCountSem = CreateSemaphore(NULL, 1, 1, NULL)))
		return false;

	if (!(data->writeCountSem = CreateSemaphore(NULL, 1, 1, NULL)))
	{
		CloseHandle(data->readCountSem);
		return false;
	}

	if (!(data->readerQueue = CreateSemaphore(NULL, 1, 1, NULL)))
	{
		CloseHandle(data->writeCountSem);
		CloseHandle(data->readCountSem);
		return false;
	}

	if (!(data->reader = CreateSemaphore(NULL, 1, 1, NULL)))
	{
		CloseHandle(data->readerQueue);
		CloseHandle(data->writeCountSem);
		CloseHandle(data->readCountSem);
		return false;
	}

	if (!(data->writer = CreateSemaphore(NULL, 1, 1, NULL)))
	{
		CloseHandle(data->reader);
		CloseHandle(data->readerQueue);
		CloseHandle(data->writeCountSem);
		CloseHandle(data->readCountSem);
		return false;
	}

	return true;
}

void closeManagedData(bt_managed_data *data)
{
	data->readCount = data->writeCount = 0;

	CloseHandle(data->writer);
	CloseHandle(data->reader);
	CloseHandle(data->readerQueue);
	CloseHandle(data->writeCountSem);
	CloseHandle(data->readCountSem);
}

void beginReading(bt_managed_data *data)
{
	WaitForSingleObject(data->readerQueue, INFINITE);
	WaitForSingleObject(data->reader, INFINITE);
	WaitForSingleObject(data->readCountSem, INFINITE);

	data->readCount++;
	if (data->readCount == 1)
		WaitForSingleObject(data->writer, INFINITE);

	ReleaseSemaphore(data->readCountSem, 1, NULL);
	ReleaseSemaphore(data->reader, 1, NULL);
	ReleaseSemaphore(data->readerQueue, 1, NULL);
}

void endReading(bt_managed_data *data)
{
	WaitForSingleObject(data->readCountSem, INFINITE);

	data->readCount--;
	if (data->readCount == 0)
		ReleaseSemaphore(data->writer, 1, NULL);

	ReleaseSemaphore(data->readCountSem, 1, NULL);
}

void beginWriting(bt_managed_data *data)
{
	WaitForSingleObject(data->writeCountSem, INFINITE);

	data->writeCount++;
	if (data->writeCount == 1)
		WaitForSingleObject(data->reader, INFINITE);

	ReleaseSemaphore(data->writeCountSem, 1, NULL);
	WaitForSingleObject(data->writer, INFINITE);
}

void endWriting(bt_managed_data *data)
{
	ReleaseSemaphore(data->writer, 1, NULL);
	WaitForSingleObject(data->writeCountSem, INFINITE);

	data->writeCount--;
	if (data->writeCount == 0)
		ReleaseSemaphore(data->reader, 1, NULL);

	ReleaseSemaphore(data->writeCountSem, 1, NULL);
}

#include <SemaphoreSyncObject.h>
#include <cppunit/Exception.h>

SemaphoreSyncObject::SemaphoreSyncObject()
	: fSemId(create_sem(1, "CppUnitSync"))
{
	if (fSemId < B_OK)
		throw CppUnit::Exception("SemaphoreSyncObject::SemaphoreSyncObject() -- Error creating semaphore");
}

SemaphoreSyncObject::~SemaphoreSyncObject() {
	delete_sem(fSemId);
}

void
SemaphoreSyncObject::lock() {
	if (acquire_sem(fSemId) < B_OK)
		throw CppUnit::Exception("SemaphoreSyncObject::lock() -- Error acquiring semaphore");
}

void
SemaphoreSyncObject::unlock() {
	if (release_sem(fSemId) < B_OK)
		throw CppUnit::Exception("SemaphoreSyncObject::unlock() -- Error releasing semaphore");
}

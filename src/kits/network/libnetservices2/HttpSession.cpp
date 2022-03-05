/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <deque>
#include <map>
#include <vector>

#include <ErrorsExt.h>
#include <HttpSession.h>
#include <Locker.h>
#include <OS.h>

using namespace BPrivate::Network;


class BHttpSession::Request {

};


class BHttpSession::Impl {
public:
												Impl();
												~Impl() noexcept;

private:
	// Thread functions
	static	status_t							ControlThreadFunc(void* arg);
	static	status_t							DataThreadFunc(void* arg);

private:
	// constants (can be accessed unlocked)
	const	sem_id								fControlQueueSem;
	const	sem_id								fDataQueueSem;
	const	thread_id							fControlThread;
	const	thread_id							fDataThread;

	// locking mechanism
			BLocker								fLock;
			int32								fQuitting;

	// queues
			std::deque<BHttpSession::Request>	fControlQueue;
			std::deque<BHttpSession::Request>	fDataQueue;
			std::vector<int32>					fCancelList;

	// data owned by the dataThread
			std::map<int,BHttpSession::Request>	connectionMap;
			std::vector<object_wait_info>		objectList;
};


BHttpSession::Impl::Impl()
	:
	fControlQueueSem(create_sem(0, "http:control")),
	fDataQueueSem(create_sem(0, "http:data")),
	fControlThread(spawn_thread(ControlThreadFunc, "http:control", B_NORMAL_PRIORITY, this)),
	fDataThread(spawn_thread(DataThreadFunc, "http:data", B_NORMAL_PRIORITY, this))
{
	// check initialization of semaphores
	if (fControlQueueSem < 0)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot create control queue semaphore");
	if (fDataQueueSem < 0)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot create data queue semaphore");

	// set up internal threads
	if (fControlThread < 0)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot create control thread");
	if (resume_thread(fControlThread) != B_OK)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot resume control thread");

	if (fDataThread < 0)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot create data thread");
	if (resume_thread(fDataThread) != B_OK)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot resume data thread");
}


BHttpSession::Impl::~Impl() noexcept
{
	atomic_set(&fQuitting, 1);
	delete_sem(fControlQueueSem);
	delete_sem(fDataQueueSem);
	status_t threadResult;
	wait_for_thread(fControlThread, &threadResult);
		// The control thread waits for the data thread
}


/*static*/ status_t
BHttpSession::Impl::ControlThreadFunc(void* arg)
{
	BHttpSession::Impl* impl = static_cast<BHttpSession::Impl*>(arg);

	// Cleanup: wait for data thread
	status_t threadResult;
	wait_for_thread(impl->fDataThread, &threadResult);
	return threadResult;
}


/*static*/ status_t
BHttpSession::Impl::DataThreadFunc(void* arg)
{
	// BHttpSession::Impl* data = static_cast<BHttpSession::Impl*>(arg);
	return B_OK;
}


BHttpSession::BHttpSession()
{
	fImpl = std::make_shared<BHttpSession::Impl>();
}


BHttpSession::~BHttpSession()
{

}


BHttpSession::BHttpSession(const BHttpSession&) noexcept = default;


BHttpSession&
BHttpSession::operator=(const BHttpSession&) noexcept = default;

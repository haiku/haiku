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

#include <AutoLocker.h>
#include <DynamicBuffer.h>
#include <ErrorsExt.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <HttpStream.h>
#include <Locker.h>
#include <Messenger.h>
#include <NetBuffer.h>
#include <NetServicesDefs.h>
#include <NetworkAddress.h>
#include <OS.h>
#include <SecureSocket.h>
#include <Socket.h>
#include <StackOrHeapArray.h>
#include <ZlibCompressionAlgorithm.h>

#include "HttpResultPrivate.h"
#include "NetServicesPrivate.h"

using namespace BPrivate::Network;


class BHttpSession::Request {
public:
									Request(BHttpRequest&& request,
													std::unique_ptr<BDataIO> target,
													BMessenger observer);

	// States
	enum RequestState {
		InitialState,
		Connected,
		StatusReceived,
		HeadersReceived,
		ContentReceived,
		TrailingHeadersReceived
	};
	RequestState					State() const noexcept { return fRequestStatus; }

	// Result Helpers
	std::shared_ptr<HttpResultPrivate>
									Result() { return fResult; }
	void							SetError(std::exception_ptr e) { fResult->SetError(e); }

	// Operational methods
	void							ResolveHostName();
	void							OpenConnection();
	void							TransferRequest();
	bool							ReceiveResult();
	void							Disconnect() noexcept;

	// Object information
	int							Socket() const noexcept { return fSocket->Socket(); }
	int32							Id() const noexcept { return fResult->id; }

private:
	BHttpRequest					fRequest;

	// Request state/events
	RequestState					fRequestStatus = InitialState;

	// Communication
	BMessenger						fObserver;
	std::shared_ptr<HttpResultPrivate> fResult;

	// Connection
	BNetworkAddress					fRemoteAddress;
	std::unique_ptr<BSocket>		fSocket;

	// Transfer state
	std::unique_ptr<BAbstractDataStream>
									fDataStream;

	// Receive state
/*	bool							receiveEnd = false;
	bool							parseEnd = false;
	BNetBuffer						inputBuffer;
	size_t							previousBufferSize = 0;
	off_t							bytesReceived = 0;
	off_t							bytesTotal = 0;
	BHttpFields						headers;
	bool							readByChunks = false;
	bool							decompress = false;
	DynamicBuffer					decompressorStorage;
	std::unique_ptr<BDataIO>		decompressingStream = nullptr;
	std::vector<char>				inputTempBuffer = std::vector<char>(4096);
	BHttpStatus						status; */
	// TODO: reset method to reset Connection and Receive State when redirected

};


class BHttpSession::Impl {
public:
												Impl();
												~Impl() noexcept;

			BHttpResult							Execute(BHttpRequest&& request,
													std::unique_ptr<BDataIO> target,
													BMessenger observer);

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


// #pragma mark -- BHttpSession::Impl


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


BHttpResult
BHttpSession::Impl::Execute(BHttpRequest&& request, std::unique_ptr<BDataIO> target,
	BMessenger observer)
{
	auto wRequest = Request(std::move(request), std::move(target), observer);

	auto retval = BHttpResult(wRequest.Result());
	auto lock = AutoLocker<BLocker>(fLock);
	fControlQueue.push_back(std::move(wRequest));
	release_sem(fControlQueueSem);
	return retval;
}


/*static*/ status_t
BHttpSession::Impl::ControlThreadFunc(void* arg)
{
	BHttpSession::Impl* impl = static_cast<BHttpSession::Impl*>(arg);

	// Outer loop to use the fControlQueueSem when new items have entered the queue
	while (true) {
		if (auto status = acquire_sem(impl->fControlQueueSem); status == B_INTERRUPTED)
			continue;
		else if (status != B_OK) {
			// Most likely B_BAD_SEM_ID indicating that the sem was deleted; go to cleanup
			break;
		}

		// Inner loop to process items on the queue
		while (true) {
			impl->fLock.Lock();
			if (impl->fControlQueue.empty() || atomic_get(&impl->fQuitting) == 1) {
				impl->fLock.Unlock();
				break;
			}
			auto request = std::move(impl->fControlQueue.front());
			impl->fControlQueue.pop_front();
			impl->fLock.Unlock();

			switch (request.State()) {
				case Request::InitialState:
				{
					bool hasError = false;
					try {
						request.ResolveHostName();
						request.OpenConnection();
					} catch (...) {
						request.SetError(std::current_exception());
						hasError = true;
					}

					if (hasError) {
						// Do not add the request back to the queue
						break;
					}

					// TODO: temporary end of the line here, as data thread not implemented
					try {
						throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
					} catch (...) {
						request.SetError(std::current_exception());
						break;
					}
					impl->fLock.Lock();
					impl->fDataQueue.push_back(std::move(request));
					impl->fLock.Unlock();
					release_sem(impl->fDataQueueSem);
					break;
				}
				default:
				{
					// not handled at this stage
					break;
				}
			}
		}
	}

	// Clean up and make sure we are quitting
	if (atomic_get(&impl->fQuitting) == 1) {
		// First wait for the data thread to complete
		status_t threadResult;
		wait_for_thread(impl->fDataThread, &threadResult);
		// Cancel all requests
		for (auto& request: impl->fControlQueue) {
			try {
				throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
			} catch (...) {
				request.SetError(std::current_exception());
			}
/*	TODO
			if (request.observer.IsValid()) {
				BMessage msg(UrlEvent::RequestCompleted);
				msg.AddInt32(UrlEventData::Id, request.result->id);
				msg.AddBool(UrlEventData::Success, false);
				request.observer.SendMessage(&msg);
			}
*/
		}
	} else {
		throw BRuntimeError(__PRETTY_FUNCTION__,
			"Unknown reason that the controlQueueSem is deleted");
	}

	// Cleanup: wait for data thread
	return B_OK;
}


static constexpr uint16 EVENT_CANCELLED = 0x4000;


/*static*/ status_t
BHttpSession::Impl::DataThreadFunc(void* arg)
{
	BHttpSession::Impl* data = static_cast<BHttpSession::Impl*>(arg);

	// initial initialization of wait list
	data->objectList.push_back(object_wait_info{data->fDataQueueSem,
		B_OBJECT_TYPE_SEMAPHORE, B_EVENT_ACQUIRE_SEMAPHORE});

	while (true) {
		if (auto status = wait_for_objects(data->objectList.data(), data->objectList.size());
			status < B_OK)
		{
			// Something went inexplicably wrong
			throw BSystemError("wait_for_objects()", status);
		}

		// First check if the change is in acquiring the sem, meaning that
		// there are new requests to be scheduled
		if (data->objectList[0].events == B_EVENT_ACQUIRE_SEMAPHORE) {
			if (auto status = acquire_sem(data->fDataQueueSem); status == B_INTERRUPTED)
				continue;
			else if (status != B_OK) {
				// Most likely B_BAD_SEM_ID indicating that the sem was deleted
				break;
			}

			// Process the cancelList and dataQueue. Note that there might
			// be a situation where a request is cancelled and added in the
			// same iteration, but that is taken care by this algorithm.
			data->fLock.Lock();
			while (!data->fDataQueue.empty()) {
				auto request = std::move(data->fDataQueue.front());
				data->fDataQueue.pop_front();
				auto socket = request.Socket();
				
				data->connectionMap.insert(std::make_pair(socket, std::move(request)));

				// Add to objectList
				data->objectList.push_back(object_wait_info{socket,
					B_OBJECT_TYPE_FD, B_EVENT_WRITE
				});
			}

			for (auto id: data->fCancelList) {
				// To cancel, we set a special event status on the
				// object_wait_info list so that we can handle it below.
				// Also: the first item in the waitlist is always the semaphore
				// so the fun starts at offset 1.
				size_t offset = 0;
				for (auto it = data->connectionMap.cbegin(); it != data->connectionMap.cend(); it++) {
					offset++;
					if (it->second.Id() == id) {
						data->objectList[offset].events = EVENT_CANCELLED;
						break;
					}
				}
			}
			data->fCancelList.clear();
			data->fLock.Unlock();
		} else if ((data->objectList[0].events & B_EVENT_INVALID) == B_EVENT_INVALID) {
			// The semaphore has been deleted. Start the cleanup
			break;
		}

		// Process all objects that are ready
		bool resizeObjectList = false;
		for(auto& item: data->objectList) {
			if (item.type != B_OBJECT_TYPE_FD)
				continue;
			if ((item.events & B_EVENT_WRITE) == B_EVENT_WRITE) {
				auto& request = data->connectionMap.find(item.object)->second;
				auto error = false;
				try {
					request.TransferRequest();
				} catch (...) {
					request.SetError(std::current_exception());
					error = true;
				}

				// End failed writes
				if (error) {
					request.Disconnect();
					data->connectionMap.erase(item.object);
					resizeObjectList = true;
				}
			} else if ((item.events & B_EVENT_READ) == B_EVENT_READ) {
				auto& request = data->connectionMap.find(item.object)->second;
				auto finished = false;
				// TODO: replace the 5 lines below
				try {
					throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
				} catch (...) {
					request.SetError(std::current_exception());
					finished = true;
				}

/* TODO with a properly implement read
				auto& request = data->connectionMap.find(item.object)->second;
				auto success = false;
				try {
					finished = _RequestRead(request);
					if (finished)
						request.result->SetBody();
					success = true;
				} catch (...) {
					request.result->SetError(std::current_exception());
					finished = true;
				}
*/

/*				if (request.result->CanCancel()) {
					// This could be done earlier, but this seems cleaner for the flow
					request.Disconnect();
					data->connectionMap.erase(item.object);
					resizeObjectList = true;
				} else */ if (finished) {
					request.Disconnect();
					/* TODO: implement this somewhere else
					if (request.observer.IsValid()) {
						BMessage msg(UrlEvent::RequestCompleted);
						msg.AddInt32(UrlEventData::Id, request.result->id);
						msg.AddBool(UrlEventData::Success, success);
						request.observer.SendMessage(&msg);
					}
					*/
					data->connectionMap.erase(item.object);
					resizeObjectList = true;
				}
			} else if ((item.events & B_EVENT_DISCONNECTED) == B_EVENT_DISCONNECTED) {
				auto& request = data->connectionMap.find(item.object)->second;
				try {
					throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::NetworkError);
				} catch (...) {
					request.SetError(std::current_exception());
				}
/*				TODO: move to BHttpSession::Request::SetError???
				if (request.observer.IsValid()) {
					BMessage msg(UrlEvent::RequestCompleted);
					msg.AddInt32(UrlEventData::Id, request.result->id);
					msg.AddBool(UrlEventData::Success, false);
					request.observer.SendMessage(&msg);
				} */
				data->connectionMap.erase(item.object);
				resizeObjectList = true;
			} else if ((item.events & EVENT_CANCELLED) == EVENT_CANCELLED) {
				auto& request = data->connectionMap.find(item.object)->second;
				request.Disconnect();
				try {
					throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
				} catch (...) {
					request.SetError(std::current_exception());
				}
				/* TODO: move to SetError()?
				if (request.observer.IsValid()) {
					BMessage msg(UrlEvent::RequestCompleted);
					msg.AddInt32(UrlEventData::Id, request.result->id);
					msg.AddBool(UrlEventData::Success, false);
					request.observer.SendMessage(&msg);
				}*/
				data->connectionMap.erase(item.object);
				resizeObjectList = true;
			} else {
				// Likely to be B_EVENT_INVALID. This should not happen
				throw BRuntimeError(__PRETTY_FUNCTION__, "Socket was deleted at an unexpected time");
			}
		}

		// Reset objectList
		data->objectList[0].events = B_EVENT_ACQUIRE_SEMAPHORE;
		if (resizeObjectList) {
			data->objectList.resize(data->connectionMap.size() + 1);
		}
		auto i = 1;
		for (auto it = data->connectionMap.cbegin(); it != data->connectionMap.cend(); it++) {
			data->objectList[i].object = it->first;
			if (it->second.State() == Request::InitialState)
				throw BRuntimeError(__PRETTY_FUNCTION__, "Invalid state of request");
			else if (it->second.State() == Request::Connected)
				data->objectList[i].events = B_EVENT_WRITE | B_EVENT_DISCONNECTED;
			else
				data->objectList[i].events = B_EVENT_READ | B_EVENT_DISCONNECTED;
			i++;
		}
	}
	// Clean up and make sure we are quitting
	if (atomic_get(&data->fQuitting) == 1) {
		// Cancel all requests
		for (auto it = data->connectionMap.begin(); it != data->connectionMap.end(); it++) {
			try {
				throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
			} catch (...) {
				it->second.SetError(std::current_exception());
			}
			/* TODO: should be part of SetError()
			if (it->second.observer.IsValid()) {
				BMessage msg(UrlEvent::RequestCompleted);
				msg.AddInt32(UrlEventData::Id, it->second.result->id);
				msg.AddBool(UrlEventData::Success, false);
				it->second.observer.SendMessage(&msg);
			}*/
	 	}
	} else {
		throw BRuntimeError(__PRETTY_FUNCTION__,
			"Unknown reason that the dataQueueSem is deleted");
	}

	return B_OK;
}


// #pragma mark -- BHttpSession (public interface)


BHttpSession::BHttpSession()
{
	fImpl = std::make_shared<BHttpSession::Impl>();
}


BHttpSession::~BHttpSession() = default;


BHttpSession::BHttpSession(const BHttpSession&) noexcept = default;


BHttpSession&
BHttpSession::operator=(const BHttpSession&) noexcept = default;


BHttpResult
BHttpSession::Execute(BHttpRequest&& request, std::unique_ptr<BDataIO> target, BMessenger observer)
{
	return fImpl->Execute(std::move(request), std::move(target), observer);
}


// #pragma mark -- BHttpSession::Request (helpers)

BHttpSession::Request::Request(BHttpRequest&& request, std::unique_ptr<BDataIO> target,
		BMessenger observer)
	: fRequest(std::move(request)), fObserver(observer)
{
	auto identifier = get_netservices_request_identifier();

	// create shared data
	fResult = std::make_shared<HttpResultPrivate>(identifier);
	fResult->owned_body = std::move(target);
}


/*!
	\brief Resolve the hostname for a request
*/
void
BHttpSession::Request::ResolveHostName()
{
	int port;
	if (fRequest.Url().HasPort())
		port = fRequest.Url().Port();
	else if (fRequest.Url().Protocol() == "https")
		port = 443;
	else
		port = 80;

	// TODO: proxy
	if (auto status = fRemoteAddress.SetTo(fRequest.Url().Host(), port); status != B_OK) {
		throw BNetworkRequestError("BNetworkAddress::SetTo()",
			BNetworkRequestError::HostnameError, status);
	}
}


/*!
	\brief Open the connection and make the socket non-blocking after opening it
*/
void
BHttpSession::Request::OpenConnection()
{
	// Set up the socket
	if (fRequest.Url().Protocol() == "https") {
		// To do: secure socket with callbacks to check certificates
		fSocket = std::make_unique<BSecureSocket>();
	} else {
		fSocket = std::make_unique<BSocket>();
	}

	// Open connection
	if (auto status = fSocket->Connect(fRemoteAddress); status != B_OK) {
		// TODO: inform listeners that the connection failed
		throw BNetworkRequestError("BSocket::Connect()",
			BNetworkRequestError::NetworkError, status);
	}

	// Make the rest of the interaction non-blocking
	auto flags = fcntl(fSocket->Socket(), F_GETFL, 0);
	if (flags == -1)
		throw BRuntimeError("fcntl()", "Error getting socket flags");
	if (fcntl(fSocket->Socket(), F_SETFL, flags | O_NONBLOCK) != 0)
		throw BRuntimeError("fcntl()", "Error setting non-blocking flag on socket");

	// TODO: inform the listeners that the connection was opened.

	fRequestStatus = Connected;
}


/*!
	\brief Transfer data from the request to the socket.

	\returns \c true if the request is complete, or false if there is more.
*/
void
BHttpSession::Request::TransferRequest()
{
	// Assert that we are in the right state
	if (fRequestStatus != Connected)
		throw BRuntimeError(__PRETTY_FUNCTION__,
			"Write request for object that is not in the Connected state");

	if (!fDataStream)
		fDataStream = std::make_unique<BHttpRequestStream>(fRequest);
	
	auto [currentBytesWritten, totalBytesWritten, totalSize, complete]
		= fDataStream->Transfer(fSocket.get());

	// TODO: notification
}


/*!
	\brief Disconnect the socket. Does not validate if it actually succeeded.
*/
void
BHttpSession::Request::Disconnect() noexcept
{
	fSocket->Disconnect();

	// TODO: inform listeners that the request has ended
}

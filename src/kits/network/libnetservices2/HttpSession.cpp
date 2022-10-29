/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <algorithm>
#include <atomic>
#include <deque>
#include <list>
#include <map>
#include <optional>
#include <vector>

#include <AutoLocker.h>
#include <DataIO.h>
#include <ErrorsExt.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <Locker.h>
#include <Messenger.h>
#include <NetBuffer.h>
#include <NetServicesDefs.h>
#include <NetworkAddress.h>
#include <OS.h>
#include <SecureSocket.h>
#include <Socket.h>
#include <ZlibCompressionAlgorithm.h>

#include "HttpBuffer.h"
#include "HttpParser.h"
#include "HttpResultPrivate.h"
#include "HttpSerializer.h"
#include "NetServicesPrivate.h"

using namespace std::literals;
using namespace BPrivate::Network;


/*!
	\brief Maximum size of the HTTP Header lines of the message.

	In the RFC there is no maximum, but we need to prevent the situation where we keep growing the
	internal buffer waiting for the end of line ('\r\n\') characters to occur.
*/
static constexpr ssize_t kMaxHeaderLineSize = 64 * 1024;


struct CounterDeleter {
	void operator()(int32* counter) const noexcept { atomic_add(counter, -1); }
};


class BHttpSession::Request
{
public:
	Request(BHttpRequest&& request, BBorrow<BDataIO> target, BMessenger observer);

	Request(Request& original, const Redirect& redirect);

	// States
	enum RequestState { InitialState, Connected, RequestSent, ContentReceived };
	RequestState State() const noexcept { return fRequestStatus; }

	// Result Helpers
	std::shared_ptr<HttpResultPrivate> Result() { return fResult; }
	void SetError(std::exception_ptr e);

	// Helpers for maintaining the connection count
	std::pair<BString, int> GetHost() const;
	void SetCounter(int32* counter) noexcept;

	// Operational methods
	void ResolveHostName();
	void OpenConnection();
	void TransferRequest();
	bool ReceiveResult();
	void Disconnect() noexcept;

	// Object information
	int Socket() const noexcept { return fSocket->Socket(); }
	int32 Id() const noexcept { return fResult->id; }
	bool CanCancel() const noexcept { return fResult->CanCancel(); }

	// Message helper
	void SendMessage(uint32 what, std::function<void(BMessage&)> dataFunc = nullptr) const;

private:
	BHttpRequest fRequest;

	// Request state/events
	RequestState fRequestStatus = InitialState;

	// Communication
	BMessenger fObserver;
	std::shared_ptr<HttpResultPrivate> fResult;

	// Connection
	BNetworkAddress fRemoteAddress;
	std::unique_ptr<BSocket> fSocket;

	// Sending and receiving
	HttpBuffer fBuffer;
	HttpSerializer fSerializer;
	HttpParser fParser;

	// Receive state
	BHttpStatus fStatus;
	BHttpFields fFields;

	// Redirection
	bool fMightRedirect = false;
	int8 fRemainingRedirects;

	// Connection counter
	std::unique_ptr<int32, CounterDeleter> fConnectionCounter;
};


class BHttpSession::Impl
{
public:
	Impl();
	~Impl() noexcept;

	BHttpResult Execute(BHttpRequest&& request, BBorrow<BDataIO> target, BMessenger observer);
	void Cancel(int32 identifier);
	void SetMaxConnectionsPerHost(size_t maxConnections);
	void SetMaxHosts(size_t maxConnections);

private:
		// Thread functions
	static status_t ControlThreadFunc(void* arg);
	static status_t DataThreadFunc(void* arg);

	// Helper functions
	std::vector<BHttpSession::Request> GetRequestsForControlThread();

private:
		// constants (can be accessed unlocked)
	const sem_id fControlQueueSem;
	const sem_id fDataQueueSem;
	const thread_id fControlThread;
	const thread_id fDataThread;

	// locking mechanism
	BLocker fLock;
	std::atomic<bool> fQuitting = false;

	// queues & shared data
	std::list<BHttpSession::Request> fControlQueue;
	std::deque<BHttpSession::Request> fDataQueue;
	std::vector<int32> fCancelList;

	// data owned by the controlThread
	using Host = std::pair<BString, int>;
	std::map<Host, int32> fConnectionCount;

	// data that can only be accessed atomically
	std::atomic<size_t> fMaxConnectionsPerHost = 2;
	std::atomic<size_t> fMaxHosts = 10;

	// data owned by the dataThread
	std::map<int, BHttpSession::Request> connectionMap;
	std::vector<object_wait_info> objectList;
};


struct BHttpSession::Redirect {
	BUrl url;
	bool redirectToGet;
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
	fQuitting.store(true);
	delete_sem(fControlQueueSem);
	delete_sem(fDataQueueSem);
	status_t threadResult;
	wait_for_thread(fControlThread, &threadResult);
		// The control thread waits for the data thread
}


BHttpResult
BHttpSession::Impl::Execute(BHttpRequest&& request, BBorrow<BDataIO> target, BMessenger observer)
{
	auto wRequest = Request(std::move(request), std::move(target), observer);

	auto retval = BHttpResult(wRequest.Result());
	auto lock = AutoLocker<BLocker>(fLock);
	fControlQueue.push_back(std::move(wRequest));
	release_sem(fControlQueueSem);
	return retval;
}


void
BHttpSession::Impl::Cancel(int32 identifier)
{
	auto lock = AutoLocker<BLocker>(fLock);
	// Check if the item is on the control queue
	fControlQueue.remove_if([&identifier](auto& request) {
		if (request.Id() == identifier) {
			try {
				throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
			} catch (...) {
				request.SetError(std::current_exception());
			}
			return true;
		}
		return false;
	});

	// Get it on the list for deletion in the data queue
	fCancelList.push_back(identifier);
	release_sem(fDataQueueSem);
}


void
BHttpSession::Impl::SetMaxConnectionsPerHost(size_t maxConnections)
{
	if (maxConnections <= 0 || maxConnections >= INT32_MAX) {
		throw BRuntimeError(
			__PRETTY_FUNCTION__, "MaxConnectionsPerHost must be between 1 and INT32_MAX");
	}
	fMaxConnectionsPerHost.store(maxConnections, std::memory_order_relaxed);
}


void
BHttpSession::Impl::SetMaxHosts(size_t maxConnections)
{
	if (maxConnections <= 0)
		throw BRuntimeError(__PRETTY_FUNCTION__, "MaxHosts must be 1 or more");
	fMaxHosts.store(maxConnections, std::memory_order_relaxed);
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

		// Check if we have woken up because we are quitting
		if (impl->fQuitting.load())
			break;

		// Get items to process (locking done by the helper)
		auto requests = impl->GetRequestsForControlThread();
		if (requests.size() == 0)
			continue;

		for (auto& request: requests) {
			bool hasError = false;
			try {
				request.ResolveHostName();
				request.OpenConnection();
			} catch (...) {
				request.SetError(std::current_exception());
				hasError = true;
			}

			if (hasError) {
				// Do not add the request back to the queue; release the sem to do another round
				// in case there is another item waiting because the limits of concurrent requests
				// were reached
				release_sem(impl->fControlQueueSem);
				continue;
			}

			impl->fLock.Lock();
			impl->fDataQueue.push_back(std::move(request));
			impl->fLock.Unlock();
			release_sem(impl->fDataQueueSem);
		}
	}

	// Clean up and make sure we are quitting
	if (impl->fQuitting.load()) {
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
		}
	} else {
		throw BRuntimeError(
			__PRETTY_FUNCTION__, "Unknown reason that the controlQueueSem is deleted");
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
	data->objectList.push_back(
		object_wait_info{data->fDataQueueSem, B_OBJECT_TYPE_SEMAPHORE, B_EVENT_ACQUIRE_SEMAPHORE});

	while (true) {
		if (auto status = wait_for_objects(data->objectList.data(), data->objectList.size());
			status == B_INTERRUPTED)
			continue;
		else if (status < 0) {
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
				data->objectList.push_back(
					object_wait_info{socket, B_OBJECT_TYPE_FD, B_EVENT_WRITE});
			}

			for (auto id: data->fCancelList) {
				// To cancel, we set a special event status on the
				// object_wait_info list so that we can handle it below.
				// Also: the first item in the waitlist is always the semaphore
				// so the fun starts at offset 1.
				size_t offset = 0;
				for (auto it = data->connectionMap.cbegin(); it != data->connectionMap.cend();
					it++) {
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
		for (auto& item: data->objectList) {
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
					release_sem(data->fControlQueueSem);
						// wake up control thread; there may queued requests unblocked.
					resizeObjectList = true;
				}
			} else if ((item.events & B_EVENT_READ) == B_EVENT_READ) {
				auto& request = data->connectionMap.find(item.object)->second;
				auto finished = false;
				try {
					if (request.CanCancel())
						finished = true;
					else
						finished = request.ReceiveResult();
				} catch (const Redirect& r) {
					// Request is redirected, send back to the controlThread
					// Move existing request into a new request and hand over to the control queue
					auto lock = AutoLocker<BLocker>(data->fLock);
					data->fControlQueue.emplace_back(request, r);
					release_sem(data->fControlQueueSem);

					finished = true;
				} catch (...) {
					request.SetError(std::current_exception());
					finished = true;
				}

				if (finished) {
					// Clean up finished requests; including redirected requests
					request.Disconnect();
					data->connectionMap.erase(item.object);
					release_sem(data->fControlQueueSem);
						// wake up control thread; there may queued requests unblocked.
					resizeObjectList = true;
				}
			} else if ((item.events & B_EVENT_DISCONNECTED) == B_EVENT_DISCONNECTED) {
				auto& request = data->connectionMap.find(item.object)->second;
				try {
					throw BNetworkRequestError(
						__PRETTY_FUNCTION__, BNetworkRequestError::NetworkError);
				} catch (...) {
					request.SetError(std::current_exception());
				}
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
				data->connectionMap.erase(item.object);
				release_sem(data->fControlQueueSem);
					// wake up control thread; there may queued requests unblocked.
				resizeObjectList = true;
			} else if (item.events == 0) {
				// No events for this item, skip
				continue;
			} else {
				// Likely to be B_EVENT_INVALID. This should not happen
				auto& request = data->connectionMap.find(item.object)->second;
				request.SendMessage(UrlEvent::DebugMessage, [](BMessage& msg) {
					msg.AddUInt32(UrlEventData::DebugType, UrlEventData::DebugError);
					msg.AddString(UrlEventData::DebugMessage, "Unexpected event; socket deleted?");
				});
				throw BRuntimeError(
					__PRETTY_FUNCTION__, "Socket was deleted at an unexpected time");
			}
		}

		// Reset objectList
		data->objectList[0].events = B_EVENT_ACQUIRE_SEMAPHORE;
		if (resizeObjectList)
			data->objectList.resize(data->connectionMap.size() + 1);

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
	if (data->fQuitting.load()) {
		// Cancel all requests
		for (auto it = data->connectionMap.begin(); it != data->connectionMap.end(); it++) {
			try {
				throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::Canceled);
			} catch (...) {
				it->second.SetError(std::current_exception());
			}
		}
	} else {
		throw BRuntimeError(__PRETTY_FUNCTION__, "Unknown reason that the dataQueueSem is deleted");
	}

	return B_OK;
}


/*!
	\brief Internal helper that filters the lists of requests to guard against the concurrent
		requests limit.

	This method will do the locking of the internal structure.
*/
std::vector<BHttpSession::Request>
BHttpSession::Impl::GetRequestsForControlThread()
{
	std::vector<BHttpSession::Request> requests;

	// Clean up connection list if it is at the max number of hosts
	if (fConnectionCount.size() >= fMaxHosts.load()) {
		for (auto it = fConnectionCount.begin(); it != fConnectionCount.end();) {
			if (atomic_get(std::addressof(it->second)) == 0) {
				it = fConnectionCount.erase(it);
			} else {
				it++;
			}
		}
	}

	// Process the list of pending requests and review if they can be started.
	auto lock = AutoLocker<BLocker>(fLock);
	fControlQueue.remove_if([this, &requests](auto& request) {
		auto host = request.GetHost();
		auto it = fConnectionCount.find(host);
		if (it != fConnectionCount.end()) {
			if (static_cast<size_t>(atomic_get(std::addressof(it->second)))
				>= fMaxConnectionsPerHost.load(std::memory_order_relaxed)) {
				request.SendMessage(UrlEvent::DebugMessage, [](BMessage& msg) {
					msg.AddUInt32(UrlEventData::DebugType, UrlEventData::DebugWarning);
					msg.AddString(UrlEventData::DebugMessage,
						"Request is queued: too many active connections for host");
				});
				return false;
			} else {
				atomic_add(std::addressof(it->second), 1);
				request.SetCounter(std::addressof(it->second));
			}
		} else {
			if (fConnectionCount.size() == fMaxHosts.load()) {
				request.SendMessage(UrlEvent::DebugMessage, [](BMessage& msg) {
					msg.AddUInt32(UrlEventData::DebugType, UrlEventData::DebugWarning);
					msg.AddString(UrlEventData::DebugMessage,
						"Request is queued: maximum number of concurrent hosts");
				});
				return false;
			}
			auto [newIt, success] = fConnectionCount.insert({host, 1});
			if (!success) {
				throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot insert into fConnectionCount");
			}
			request.SetCounter(std::addressof(newIt->second));
		}
		requests.emplace_back(std::move(request));
		return true;
	});
	return requests;
}


// #pragma mark -- BHttpSession (public interface)


BHttpSession::BHttpSession()
{
	fImpl = std::make_shared<BHttpSession::Impl>();
}


BHttpSession::~BHttpSession() = default;


BHttpSession::BHttpSession(const BHttpSession&) noexcept = default;


BHttpSession& BHttpSession::operator=(const BHttpSession&) noexcept = default;


BHttpResult
BHttpSession::Execute(BHttpRequest&& request, BBorrow<BDataIO> target, BMessenger observer)
{
	return fImpl->Execute(std::move(request), std::move(target), observer);
}


void
BHttpSession::Cancel(int32 identifier)
{
	fImpl->Cancel(identifier);
}


void
BHttpSession::Cancel(const BHttpResult& request)
{
	fImpl->Cancel(request.Identity());
}


void
BHttpSession::SetMaxConnectionsPerHost(size_t maxConnections)
{
	fImpl->SetMaxConnectionsPerHost(maxConnections);
}


void
BHttpSession::SetMaxHosts(size_t maxConnections)
{
	fImpl->SetMaxHosts(maxConnections);
}


// #pragma mark -- BHttpSession::Request (helpers)
BHttpSession::Request::Request(BHttpRequest&& request, BBorrow<BDataIO> target, BMessenger observer)
	:
	fRequest(std::move(request)),
	fObserver(observer)
{
	auto identifier = get_netservices_request_identifier();

	// interpret the remaining redirects
	fRemainingRedirects = fRequest.MaxRedirections();

	// create shared data
	fResult = std::make_shared<HttpResultPrivate>(identifier);

	// check if there is a target
	if (target.HasValue())
		fResult->bodyTarget = std::move(target);

	// inform the parser when we do a HEAD request, so not to expect content
	if (fRequest.Method() == BHttpMethod::Head)
		fParser.SetNoContent();
}


BHttpSession::Request::Request(Request& original, const BHttpSession::Redirect& redirect)
	:
	fRequest(std::move(original.fRequest)),
	fObserver(original.fObserver),
	fResult(original.fResult)
{
	// update the original request with the new location
	fRequest.SetUrl(redirect.url);

	if (redirect.redirectToGet
		&& (fRequest.Method() != BHttpMethod::Head && fRequest.Method() != BHttpMethod::Get)) {
		fRequest.SetMethod(BHttpMethod::Get);
		fRequest.ClearRequestBody();
	}

	fRemainingRedirects = original.fRemainingRedirects--;

	// inform the parser when we do a HEAD request, so not to expect content
	if (fRequest.Method() == BHttpMethod::Head)
		fParser.SetNoContent();
}


/*!
	\brief Helper that sets the error in the result to \a e and notifies the listeners.
*/
void
BHttpSession::Request::SetError(std::exception_ptr e)
{
	fResult->SetError(e);
	SendMessage(UrlEvent::DebugMessage, [&e](BMessage& msg) {
		msg.AddUInt32(UrlEventData::DebugType, UrlEventData::DebugError);
		try {
			std::rethrow_exception(e);
		} catch (BError& error) {
			msg.AddString(UrlEventData::DebugMessage, error.DebugMessage());
		} catch (std::exception& error) {
			msg.AddString(UrlEventData::DebugMessage, error.what());
		} catch (...) {
			msg.AddString(UrlEventData::DebugMessage, "Unknown exception");
		}
	});
	SendMessage(UrlEvent::RequestCompleted,
		[](BMessage& msg) { msg.AddBool(UrlEventData::Success, false); });
}


std::pair<BString, int>
BHttpSession::Request::GetHost() const
{
	return {fRequest.Url().Host(), fRequest.Url().Port()};
}


void
BHttpSession::Request::SetCounter(int32* counter) noexcept
{
	fConnectionCounter = std::unique_ptr<int32, CounterDeleter>(counter);
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
		throw BNetworkRequestError(
			"BNetworkAddress::SetTo()", BNetworkRequestError::HostnameError, status);
	}

	SendMessage(UrlEvent::HostNameResolved,
		[this](BMessage& msg) { msg.AddString(UrlEventData::HostName, fRequest.Url().Host()); });
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

	// Set timeout
	fSocket->SetTimeout(fRequest.Timeout());

	// Open connection
	if (auto status = fSocket->Connect(fRemoteAddress); status != B_OK) {
		// TODO: inform listeners that the connection failed
		throw BNetworkRequestError(
			"BSocket::Connect()", BNetworkRequestError::NetworkError, status);
	}

	// Make the rest of the interaction non-blocking
	auto flags = fcntl(fSocket->Socket(), F_GETFL, 0);
	if (flags == -1)
		throw BRuntimeError("fcntl()", "Error getting socket flags");
	if (fcntl(fSocket->Socket(), F_SETFL, flags | O_NONBLOCK) != 0)
		throw BRuntimeError("fcntl()", "Error setting non-blocking flag on socket");

	SendMessage(UrlEvent::ConnectionOpened);

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
		throw BRuntimeError(
			__PRETTY_FUNCTION__, "Write request for object that is not in the Connected state");

	if (!fSerializer.IsInitialized())
		fSerializer.SetTo(fBuffer, fRequest);

	auto currentBytesWritten = fSerializer.Serialize(fBuffer, fSocket.get());

	if (currentBytesWritten > 0) {
		SendMessage(UrlEvent::UploadProgress, [this](BMessage& msg) {
			msg.AddInt64(UrlEventData::NumBytes, fSerializer.BodyBytesTransferred());
			if (auto totalSize = fSerializer.BodyBytesTotal())
				msg.AddInt64(UrlEventData::TotalBytes, totalSize.value());
		});
	}

	if (fSerializer.Complete())
		fRequestStatus = RequestSent;
}


/*!
	\brief Transfer data from the socket and parse the result.

	\returns \c true if the request is complete, or false if there is more.
*/
bool
BHttpSession::Request::ReceiveResult()
{
	// First: stream data from the socket
	auto bytesRead = fBuffer.ReadFrom(fSocket.get());

	if (bytesRead == B_WOULD_BLOCK || bytesRead == B_INTERRUPTED)
		return false;

	auto readEnd = bytesRead == 0;

	// Parse the content in the buffer
	switch (fParser.State()) {
		case HttpInputStreamState::StatusLine:
		{
			if (fBuffer.RemainingBytes() == static_cast<size_t>(bytesRead)) {
				// In the initial run, the bytes in the buffer will match the bytes read to indicate
				// the response has started.
				SendMessage(UrlEvent::ResponseStarted);
			}

			if (fParser.ParseStatus(fBuffer, fStatus)) {
				// the status headers are now received, decide what to do next

				// Determine if we can handle redirects; else notify of receiving status
				if (fRemainingRedirects > 0) {
					switch (fStatus.StatusCode()) {
						case BHttpStatusCode::MovedPermanently:
						case BHttpStatusCode::TemporaryRedirect:
						case BHttpStatusCode::PermanentRedirect:
							// These redirects require the request body to be sent again. It this is
							// possible, BHttpRequest::RewindBody() will return true in which case
							// we can handle the redirect.
							if (!fRequest.RewindBody())
								break;
							[[fallthrough]];
						case BHttpStatusCode::Found:
						case BHttpStatusCode::SeeOther:
							// These redirects redirect to GET, so we don't care if we can rewind
							// the body; in this case redirect
							fMightRedirect = true;
							break;
						default:
							break;
					}
				}

				if ((fStatus.StatusClass() == BHttpStatusClass::ClientError
						|| fStatus.StatusClass() == BHttpStatusClass::ServerError)
					&& fRequest.StopOnError()) {
					fRequestStatus = ContentReceived;
					fResult->SetStatus(std::move(fStatus));
					fResult->SetFields(BHttpFields());
					fResult->SetBody();
					return true;
				}

				if (!fMightRedirect) {
					// we are not redirecting and there is no error, so inform listeners
					SendMessage(UrlEvent::HttpStatus, [this](BMessage& msg) {
						msg.AddInt16(UrlEventData::HttpStatusCode, fStatus.code);
					});
					fResult->SetStatus(BHttpStatus{fStatus.code, std::move(fStatus.text)});
				}
			} else {
				// We do not have enough data for the status line yet
				if (readEnd) {
					throw BNetworkRequestError(__PRETTY_FUNCTION__,
						BNetworkRequestError::ProtocolError,
						"Response did not include a complete status line");
				}
				return false;
			}
			[[fallthrough]];
		}
		case HttpInputStreamState::Fields:
		{
			if (!fParser.ParseFields(fBuffer, fFields)) {
				// there may be more headers to receive, throw an error if there will be no more
				if (readEnd) {
					throw BNetworkRequestError(__PRETTY_FUNCTION__,
						BNetworkRequestError::ProtocolError,
						"Response did not include a complete header section");
				}
				break;
			}

			// The headers have been received, now set up the rest of the response handling

			// Handle redirects
			if (fMightRedirect) {
				auto redirectToGet = false;
				switch (fStatus.StatusCode()) {
					case BHttpStatusCode::Found:
					case BHttpStatusCode::SeeOther:
						// 302 and 303 redirections convert all requests to GET request, except for
						// HEAD
						redirectToGet = true;
						[[fallthrough]];
					case BHttpStatusCode::MovedPermanently:
					case BHttpStatusCode::TemporaryRedirect:
					case BHttpStatusCode::PermanentRedirect:
					{
						auto locationField = fFields.FindField("Location");
						if (locationField == fFields.end()) {
							throw BNetworkRequestError(__PRETTY_FUNCTION__,
								BNetworkRequestError::ProtocolError,
								"Redirect; the Location field must be present and cannot be found");
						}
						auto locationString = BString(
							(*locationField).Value().data(), (*locationField).Value().size());
						auto redirect = BHttpSession::Redirect{
							BUrl(fRequest.Url(), locationString), redirectToGet};
						if (!redirect.url.IsValid()) {
							throw BNetworkRequestError(__PRETTY_FUNCTION__,
								BNetworkRequestError::ProtocolError,
								"Redirect; invalid URL in the Location field");
						}

						// Notify of redirect
						SendMessage(UrlEvent::HttpRedirect, [&locationString](BMessage& msg) {
							msg.AddString(UrlEventData::HttpRedirectUrl, locationString);
						});
						throw redirect;
					}
					default:
						// ignore other status codes and continue regular processing
						SendMessage(UrlEvent::HttpStatus, [this](BMessage& msg) {
							msg.AddInt16(UrlEventData::HttpStatusCode, fStatus.code);
						});
						fResult->SetStatus(BHttpStatus{fStatus.code, std::move(fStatus.text)});
						break;
				}
			}

			// TODO: Parse received cookies

			// Move headers to the result and inform listener
			fResult->SetFields(std::move(fFields));
			SendMessage(UrlEvent::HttpFields);

			if (!fParser.HasContent()) {
				// Any requests with not content are finished
				fResult->SetBody();
				SendMessage(UrlEvent::RequestCompleted,
					[](BMessage& msg) { msg.AddBool(UrlEventData::Success, true); });
				fRequestStatus = ContentReceived;
				return true;
			}
			[[fallthrough]];
		}
		case HttpInputStreamState::Body:
		{
			size_t bytesWrittenToBody;
				// The bytesWrittenToBody may differ from the bytes parsed from the buffer when
				// there is compression on the incoming stream.
			bytesRead = fParser.ParseBody(
				fBuffer,
				[this, &bytesWrittenToBody](const std::byte* buffer, size_t size) {
					bytesWrittenToBody = fResult->WriteToBody(buffer, size);
					return bytesWrittenToBody;
				},
				readEnd);

			SendMessage(UrlEvent::DownloadProgress, [this, bytesRead](BMessage& msg) {
				msg.AddInt64(UrlEventData::NumBytes, bytesRead);
				if (fParser.BodyBytesTotal())
					msg.AddInt64(UrlEventData::TotalBytes, fParser.BodyBytesTotal().value());
			});

			if (bytesWrittenToBody > 0) {
				SendMessage(UrlEvent::BytesWritten, [bytesWrittenToBody](BMessage& msg) {
					msg.AddInt64(UrlEventData::NumBytes, bytesWrittenToBody);
				});
			}

			if (fParser.Complete()) {
				fResult->SetBody();
				SendMessage(UrlEvent::RequestCompleted,
					[](BMessage& msg) { msg.AddBool(UrlEventData::Success, true); });
				fRequestStatus = ContentReceived;
				return true;
			} else if (readEnd) {
				// the parsing of the body is not complete but we are at the end of the data
				throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError,
					"Unexpected end of data: more data was expected");
			}

			break;
		}
		default:
			throw BRuntimeError(__PRETTY_FUNCTION__, "Not reachable");
	}

	// There is more to receive
	return false;
}


/*!
	\brief Disconnect the socket. Does not validate if it actually succeeded.
*/
void
BHttpSession::Request::Disconnect() noexcept
{
	fSocket->Disconnect();
}


/*!
	\brief Send a message to the observer, if one is present

	\param what The code of the message to be sent
	\param dataFunc Optional function that adds additional data to the message.
*/
void
BHttpSession::Request::SendMessage(uint32 what, std::function<void(BMessage&)> dataFunc) const
{
	if (fObserver.IsValid()) {
		BMessage msg(what);
		msg.AddInt32(UrlEventData::Id, fResult->id);
		if (dataFunc)
			dataFunc(msg);
		fObserver.SendMessage(&msg);
	}
}


// #pragma mark -- Message constants


namespace BPrivate::Network::UrlEventData {
const char* HttpStatusCode = "url:httpstatuscode";
const char* SSLCertificate = "url:sslcertificate";
const char* SSLMessage = "url:sslmessage";
const char* HttpRedirectUrl = "url:httpredirecturl";
} // namespace BPrivate::Network::UrlEventData

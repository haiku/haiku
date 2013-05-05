/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 */


#include "TCPEndpoint.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <new>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <Select.h>

#include <net_buffer.h>
#include <net_datalink.h>
#include <net_stat.h>
#include <NetBufferUtilities.h>
#include <NetUtilities.h>

#include <lock.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/khash.h>
#include <util/list.h>

#include "EndpointManager.h"


// References:
//  - RFC 793 - Transmission Control Protocol
//  - RFC 813 - Window and Acknowledgement Strategy in TCP
//	- RFC 1337 - TIME_WAIT Assassination Hazards in TCP
//
// Things this implementation currently doesn't implement:
//	- TCP Slow Start, Congestion Avoidance, Fast Retransmit, and Fast Recovery,
//	  RFC 2001, RFC 2581, RFC 3042
//	- NewReno Modification to TCP's Fast Recovery, RFC 2582
//	- Explicit Congestion Notification (ECN), RFC 3168
//	- SYN-Cache
//	- TCP Extensions for High Performance, RFC 1323
//	- SACK, Selective Acknowledgment - RFC 2018, RFC 2883, RFC 3517
//	- Forward RTO-Recovery, RFC 4138
//	- Time-Wait hash instead of keeping sockets alive

#define PrintAddress(address) \
	AddressString(Domain(), address, true).Data()

//#define TRACE_TCP
//#define PROBE_TCP

#ifdef TRACE_TCP
// the space before ', ##args' is important in order for this to work with cpp 2.95
#	define TRACE(format, args...)	dprintf("%ld: TCP [%llu] %p (%12s) " \
		format "\n", find_thread(NULL), system_time(), this, \
		name_for_state(fState) , ##args)
#else
#	define TRACE(args...)			do { } while (0)
#endif

#ifdef PROBE_TCP
#	define PROBE(buffer, window) \
	dprintf("TCP PROBE %llu %s %s %ld snxt %lu suna %lu cw %lu sst %lu win %lu swin %lu smax-suna %lu savail %lu sqused %lu rto %llu\n", \
		system_time(), PrintAddress(buffer->source), \
		PrintAddress(buffer->destination), buffer->size, fSendNext.Number(), \
		fSendUnacknowledged.Number(), fCongestionWindow, fSlowStartThreshold, \
		window, fSendWindow, (fSendMax - fSendUnacknowledged).Number(), \
		fSendQueue.Available(fSendNext), fSendQueue.Used(), fRetransmitTimeout)
#else
#	define PROBE(buffer, window)	do { } while (0)
#endif

#if TCP_TRACING
namespace TCPTracing {

class Receive : public AbstractTraceEntry {
public:
	Receive(TCPEndpoint* endpoint, tcp_segment_header& segment, uint32 window,
			net_buffer* buffer)
		:
		fEndpoint(endpoint),
		fBuffer(buffer),
		fBufferSize(buffer->size),
		fSequence(segment.sequence),
		fAcknowledge(segment.acknowledge),
		fWindow(window),
		fState(endpoint->State()),
		fFlags(segment.flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:%p (%12s) receive buffer %p (%lu bytes), flags %x, "
			"seq %lu, ack %lu, wnd %lu", fEndpoint, name_for_state(fState),
			fBuffer, fBufferSize, fFlags, fSequence, fAcknowledge, fWindow);
	}

protected:
	TCPEndpoint*	fEndpoint;
	net_buffer*		fBuffer;
	uint32			fBufferSize;
	uint32			fSequence;
	uint32			fAcknowledge;
	uint32			fWindow;
	tcp_state		fState;
	uint8			fFlags;
};

class Send : public AbstractTraceEntry {
public:
	Send(TCPEndpoint* endpoint, tcp_segment_header& segment, net_buffer* buffer,
			tcp_sequence firstSequence, tcp_sequence lastSequence)
		:
		fEndpoint(endpoint),
		fBuffer(buffer),
		fBufferSize(buffer->size),
		fSequence(segment.sequence),
		fAcknowledge(segment.acknowledge),
		fFirstSequence(firstSequence.Number()),
		fLastSequence(lastSequence.Number()),
		fState(endpoint->State()),
		fFlags(segment.flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:%p (%12s) send buffer %p (%lu bytes), flags %x, "
			"seq %lu, ack %lu, first %lu, last %lu",
			fEndpoint, name_for_state(fState), fBuffer, fBufferSize, fFlags,
			fSequence, fAcknowledge, fFirstSequence, fLastSequence);
	}

protected:
	TCPEndpoint*	fEndpoint;
	net_buffer*		fBuffer;
	uint32			fBufferSize;
	uint32			fSequence;
	uint32			fAcknowledge;
	uint32			fFirstSequence;
	uint32			fLastSequence;
	tcp_state		fState;
	uint8			fFlags;
};

class State : public AbstractTraceEntry {
public:
	State(TCPEndpoint* endpoint)
		:
		fEndpoint(endpoint),
		fState(endpoint->State())
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:%p (%12s) state change", fEndpoint,
			name_for_state(fState));
	}

protected:
	TCPEndpoint*	fEndpoint;
	tcp_state		fState;
};

class Spawn : public AbstractTraceEntry {
public:
	Spawn(TCPEndpoint* listeningEndpoint, TCPEndpoint* spawnedEndpoint)
		:
		fListeningEndpoint(listeningEndpoint),
		fSpawnedEndpoint(spawnedEndpoint)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:%p spawns %p", fListeningEndpoint, fSpawnedEndpoint);
	}

protected:
	TCPEndpoint*	fListeningEndpoint;
	TCPEndpoint*	fSpawnedEndpoint;
};

class Error : public AbstractTraceEntry {
public:
	Error(TCPEndpoint* endpoint, const char* error, int32 line)
		:
		fEndpoint(endpoint),
		fLine(line),
		fError(error),
		fState(endpoint->State())
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:%p (%12s) error at line %ld: %s", fEndpoint,
			name_for_state(fState), fLine, fError);
	}

protected:
	TCPEndpoint*	fEndpoint;
	int32			fLine;
	const char*		fError;
	tcp_state		fState;
};

class Timer : public AbstractTraceEntry {
public:
	Timer(TCPEndpoint* endpoint, const char* which)
		:
		fEndpoint(endpoint),
		fWhich(which),
		fState(endpoint->State())
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:%p (%12s) %s timer", fEndpoint,
			name_for_state(fState), fWhich);
	}

protected:
	TCPEndpoint*	fEndpoint;
	const char*		fWhich;
	tcp_state		fState;
};

}	// namespace TCPTracing

#	define T(x)	new(std::nothrow) TCPTracing::x
#else
#	define T(x)
#endif	// TCP_TRACING

// Initial estimate for packet round trip time (RTT)
#define TCP_INITIAL_RTT		2000000

// constants for the fFlags field
enum {
	FLAG_OPTION_WINDOW_SCALE	= 0x01,
	FLAG_OPTION_TIMESTAMP		= 0x02,
	// TODO: Should FLAG_NO_RECEIVE apply as well to received connections?
	//       That is, what is expected from accept() after a shutdown()
	//       is performed on a listen()ing socket.
	FLAG_NO_RECEIVE				= 0x04,
	FLAG_CLOSED					= 0x08,
	FLAG_DELETE_ON_CLOSE		= 0x10,
	FLAG_LOCAL					= 0x20
};


static const int kTimestampFactor = 1024;


static inline bigtime_t
absolute_timeout(bigtime_t timeout)
{
	if (timeout == 0 || timeout == B_INFINITE_TIMEOUT)
		return timeout;

	return timeout + system_time();
}


static inline status_t
posix_error(status_t error)
{
	if (error == B_TIMED_OUT)
		return B_WOULD_BLOCK;

	return error;
}


static inline bool
in_window(const tcp_sequence& sequence, const tcp_sequence& receiveNext,
	uint32 receiveWindow)
{
	return sequence >= receiveNext && sequence < (receiveNext + receiveWindow);
}


static inline bool
segment_in_sequence(const tcp_segment_header& segment, int size,
	const tcp_sequence& receiveNext, uint32 receiveWindow)
{
	tcp_sequence sequence(segment.sequence);
	if (size == 0) {
		if (receiveWindow == 0)
			return sequence == receiveNext;
		return in_window(sequence, receiveNext, receiveWindow);
	} else {
		if (receiveWindow == 0)
			return false;
		return in_window(sequence, receiveNext, receiveWindow)
			|| in_window(sequence + size - 1, receiveNext, receiveWindow);
	}
}


static inline bool
is_writable(tcp_state state)
{
	return state == SYNCHRONIZE_SENT || state == SYNCHRONIZE_RECEIVED
		|| state == ESTABLISHED || state == FINISH_RECEIVED;
}


static inline uint32 tcp_now()
{
	return system_time() / kTimestampFactor;
}


static inline uint32 tcp_diff_timestamp(uint32 base)
{
	uint32 now = tcp_now();

	if (now > base)
		return now - base;

	return now + UINT_MAX - base;
}


static inline bool
state_needs_finish(int32 state)
{
	return state == WAIT_FOR_FINISH_ACKNOWLEDGE
		|| state == FINISH_SENT || state == CLOSING;
}


//	#pragma mark -


WaitList::WaitList(const char* name)
{
	fCondition = 0;
	fSem = create_sem(0, name);
}


WaitList::~WaitList()
{
	delete_sem(fSem);
}


status_t
WaitList::InitCheck() const
{
	return fSem;
}


status_t
WaitList::Wait(MutexLocker& locker, bigtime_t timeout)
{
	locker.Unlock();

	status_t status = B_OK;

	while (!atomic_test_and_set(&fCondition, 0, 1)) {
		status = acquire_sem_etc(fSem, 1, B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT,
			timeout);
		if (status != B_OK)
			break;
	}

	locker.Lock();
	return status;
}


void
WaitList::Signal()
{
	atomic_or(&fCondition, 1);
#ifdef __HAIKU__
	release_sem_etc(fSem, 1, B_DO_NOT_RESCHEDULE | B_RELEASE_ALL);
#else
	int32 count;
	if (get_sem_count(fSem, &count) == B_OK && count < 0)
		release_sem_etc(fSem, -count, B_DO_NOT_RESCHEDULE);
#endif
}


//	#pragma mark -


TCPEndpoint::TCPEndpoint(net_socket* socket)
	:
	ProtocolSocket(socket),
	fManager(NULL),
	fReceiveList("tcp receive"),
	fSendList("tcp send"),
	fOptions(0),
	fSendWindowShift(0),
	fReceiveWindowShift(0),
	fSendUnacknowledged(0),
	fSendNext(0),
	fSendMax(0),
	fSendUrgentOffset(0),
	fSendWindow(0),
	fSendMaxWindow(0),
	fSendMaxSegmentSize(TCP_DEFAULT_MAX_SEGMENT_SIZE),
	fSendQueue(socket->send.buffer_size),
	fInitialSendSequence(0),
	fDuplicateAcknowledgeCount(0),
	fRoute(NULL),
	fReceiveNext(0),
	fReceiveMaxAdvertised(0),
	fReceiveWindow(socket->receive.buffer_size),
	fReceiveMaxSegmentSize(TCP_DEFAULT_MAX_SEGMENT_SIZE),
	fReceiveQueue(socket->receive.buffer_size),
	fRoundTripTime(TCP_INITIAL_RTT / kTimestampFactor),
	fRoundTripDeviation(TCP_INITIAL_RTT / kTimestampFactor),
	fRetransmitTimeout(TCP_INITIAL_RTT),
	fReceivedTimestamp(0),
	fCongestionWindow(0),
	fSlowStartThreshold(0),
	fState(CLOSED),
	fFlags(FLAG_OPTION_WINDOW_SCALE | FLAG_OPTION_TIMESTAMP)
{
	// TODO: to be replaced with a real read/write locking strategy!
	mutex_init(&fLock, "tcp lock");

	gStackModule->init_timer(&fPersistTimer, TCPEndpoint::_PersistTimer, this);
	gStackModule->init_timer(&fRetransmitTimer, TCPEndpoint::_RetransmitTimer,
		this);
	gStackModule->init_timer(&fDelayedAcknowledgeTimer,
		TCPEndpoint::_DelayedAcknowledgeTimer, this);
	gStackModule->init_timer(&fTimeWaitTimer, TCPEndpoint::_TimeWaitTimer,
		this);
}


TCPEndpoint::~TCPEndpoint()
{
	mutex_lock(&fLock);

	_CancelConnectionTimers();
	gStackModule->cancel_timer(&fTimeWaitTimer);

	if (fManager != NULL) {
		fManager->Unbind(this);
		put_endpoint_manager(fManager);
	}

	mutex_destroy(&fLock);

	// we need to wait for all timers to return
	gStackModule->wait_for_timer(&fRetransmitTimer);
	gStackModule->wait_for_timer(&fPersistTimer);
	gStackModule->wait_for_timer(&fDelayedAcknowledgeTimer);
	gStackModule->wait_for_timer(&fTimeWaitTimer);

	gDatalinkModule->put_route(Domain(), fRoute);
}


status_t
TCPEndpoint::InitCheck() const
{
	if (fReceiveList.InitCheck() < B_OK)
		return fReceiveList.InitCheck();

	if (fSendList.InitCheck() < B_OK)
		return fSendList.InitCheck();

	return B_OK;
}


//	#pragma mark - protocol API


status_t
TCPEndpoint::Open()
{
	TRACE("Open()");

	status_t status = ProtocolSocket::Open();
	if (status < B_OK)
		return status;

	fManager = get_endpoint_manager(Domain());
	if (fManager == NULL)
		return EAFNOSUPPORT;

	return B_OK;
}


status_t
TCPEndpoint::Close()
{
	TRACE("Close()");

	MutexLocker locker(fLock);

	if (fState == LISTEN)
		delete_sem(fAcceptSemaphore);

	if (fState == SYNCHRONIZE_SENT || fState == LISTEN) {
		// TODO: what about linger in case of SYNCHRONIZE_SENT?
		fState = CLOSED;
		T(State(this));
		return B_OK;
	}

	status_t status = _Disconnect(true);
	if (status != B_OK)
		return status;

	if (socket->options & SO_LINGER) {
		TRACE("Close(): Lingering for %i secs", socket->linger);

		bigtime_t maximum = absolute_timeout(socket->linger * 1000000LL);

		while (fSendQueue.Used() > 0) {
			status = fSendList.Wait(locker, maximum);
			if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
				break;
			else if (status < B_OK)
				return status;
		}

		TRACE("Close(): after waiting, the SendQ was left with %lu bytes.",
			fSendQueue.Used());
	}
	return B_OK;
}


void
TCPEndpoint::Free()
{
	TRACE("Free()");

	MutexLocker _(fLock);

	if (fState <= SYNCHRONIZE_SENT)
		return;

	// we are only interested in the timer, not in changing state
	_EnterTimeWait();

	fFlags |= FLAG_CLOSED;
	if ((fFlags & FLAG_DELETE_ON_CLOSE) == 0) {
		// we'll be freed later when the 2MSL timer expires
		gSocketModule->acquire_socket(socket);
	}
}


/*!	Creates and sends a synchronize packet to /a address, and then waits
	until the connection has been established or refused.
*/
status_t
TCPEndpoint::Connect(const sockaddr* address)
{
	TRACE("Connect() on address %s", PrintAddress(address));

	if (!AddressModule()->is_same_family(address))
		return EAFNOSUPPORT;

	MutexLocker locker(fLock);

	if (gStackModule->is_restarted_syscall()) {
		bigtime_t timeout = gStackModule->restore_syscall_restart_timeout();
		status_t status = _WaitForEstablished(locker, timeout);
		TRACE("  Connect(): Connection complete: %s (timeout was %llu)",
			strerror(status), timeout);
		return posix_error(status);
	}

	// Can only call connect() from CLOSED or LISTEN states
	// otherwise endpoint is considered already connected
	if (fState == LISTEN) {
		// this socket is about to connect; remove pending connections in the backlog
		gSocketModule->set_max_backlog(socket, 0);
	} else if (fState == ESTABLISHED) {
		return EISCONN;
	} else if (fState != CLOSED)
		return EINPROGRESS;

	// consider destination address INADDR_ANY as INADDR_LOOPBACK
	sockaddr_storage _address;
	if (AddressModule()->is_empty_address(address, false)) {
		AddressModule()->get_loopback_address((sockaddr *)&_address);
		// for IPv4 and IPv6 the port is at the same offset
		((sockaddr_in &)_address).sin_port = ((sockaddr_in *)address)->sin_port;
		address = (sockaddr *)&_address;
	}

	status_t status = _PrepareSendPath(address);
	if (status < B_OK)
		return status;

	TRACE("  Connect(): starting 3-way handshake...");

	fState = SYNCHRONIZE_SENT;
	T(State(this));

	// send SYN
	status = _SendQueued();
	if (status != B_OK) {
		_Close();
		return status;
	}

	// If we are running over Loopback, after _SendQueued() returns we
	// may be in ESTABLISHED already.
	if (fState == ESTABLISHED) {
		TRACE("  Connect() completed after _SendQueued()");
		return B_OK;
	}

	// wait until 3-way handshake is complete (if needed)
	bigtime_t timeout = min_c(socket->send.timeout, TCP_CONNECTION_TIMEOUT);
	if (timeout == 0) {
		// we're a non-blocking socket
		TRACE("  Connect() delayed, return EINPROGRESS");
		return EINPROGRESS;
	}

	bigtime_t absoluteTimeout = absolute_timeout(timeout);
	gStackModule->store_syscall_restart_timeout(absoluteTimeout);

	status = _WaitForEstablished(locker, absoluteTimeout);
	TRACE("  Connect(): Connection complete: %s (timeout was %llu)",
		strerror(status), timeout);
	return posix_error(status);
}


status_t
TCPEndpoint::Accept(struct net_socket** _acceptedSocket)
{
	TRACE("Accept()");

	MutexLocker locker(fLock);

	status_t status;
	bigtime_t timeout = absolute_timeout(socket->receive.timeout);
	if (gStackModule->is_restarted_syscall())
		timeout = gStackModule->restore_syscall_restart_timeout();
	else
		gStackModule->store_syscall_restart_timeout(timeout);

	do {
		locker.Unlock();

		status = acquire_sem_etc(fAcceptSemaphore, 1, B_ABSOLUTE_TIMEOUT
			| B_CAN_INTERRUPT, timeout);
		if (status != B_OK) {
			if (status == B_TIMED_OUT && socket->receive.timeout == 0)
				return B_WOULD_BLOCK;

			return status;
		}

		locker.Lock();
		status = gSocketModule->dequeue_connected(socket, _acceptedSocket);
#ifdef TRACE_TCP
		if (status == B_OK)
			TRACE("  Accept() returning %p", (*_acceptedSocket)->first_protocol);
#endif
	} while (status != B_OK);

	return status;
}


status_t
TCPEndpoint::Bind(const sockaddr *address)
{
	if (address == NULL)
		return B_BAD_VALUE;

	MutexLocker lock(fLock);

	TRACE("Bind() on address %s", PrintAddress(address));

	if (fState != CLOSED)
		return EISCONN;

	return fManager->Bind(this, address);
}


status_t
TCPEndpoint::Unbind(struct sockaddr *address)
{
	TRACE("Unbind()");

	MutexLocker _(fLock);
	return fManager->Unbind(this);
}


status_t
TCPEndpoint::Listen(int count)
{
	TRACE("Listen()");

	MutexLocker _(fLock);

	if (fState != CLOSED)
		return B_BAD_VALUE;

	fAcceptSemaphore = create_sem(0, "tcp accept");
	if (fAcceptSemaphore < B_OK)
		return ENOBUFS;

	status_t status = fManager->SetPassive(this);
	if (status != B_OK) {
		delete_sem(fAcceptSemaphore);
		fAcceptSemaphore = -1;
		return status;
	}

	gSocketModule->set_max_backlog(socket, count);

	fState = LISTEN;
	T(State(this));
	return B_OK;
}


status_t
TCPEndpoint::Shutdown(int direction)
{
	TRACE("Shutdown(%i)", direction);

	MutexLocker lock(fLock);

	if (direction == SHUT_RD || direction == SHUT_RDWR)
		fFlags |= FLAG_NO_RECEIVE;

	if (direction == SHUT_WR || direction == SHUT_RDWR) {
		// TODO: That's not correct. After read/write shutting down the socket
		// one should still be able to read previously arrived data.
		_Disconnect(false);
	}

	return B_OK;
}


/*!	Puts data contained in \a buffer into send buffer */
status_t
TCPEndpoint::SendData(net_buffer *buffer)
{
	MutexLocker lock(fLock);

	TRACE("SendData(buffer %p, size %lu, flags %lx) [total %lu bytes, has %lu]",
		  buffer, buffer->size, buffer->flags, fSendQueue.Size(),
		  fSendQueue.Free());

	if (fState == CLOSED)
		return ENOTCONN;
	if (fState == LISTEN)
		return EDESTADDRREQ;
	if (!is_writable(fState)) {
		// we only send signals when called from userland
		if (gStackModule->is_syscall())
			send_signal(find_thread(NULL), SIGPIPE);
		return EPIPE;
	}

	uint32 flags = buffer->flags;
	size_t left = buffer->size;

	bigtime_t timeout = absolute_timeout(socket->send.timeout);
	if (gStackModule->is_restarted_syscall())
		timeout = gStackModule->restore_syscall_restart_timeout();
	else
		gStackModule->store_syscall_restart_timeout(timeout);

	while (left > 0) {
		while (fSendQueue.Free() < socket->send.low_water_mark) {
			// wait until enough space is available
			status_t status = fSendList.Wait(lock, timeout);
			if (status < B_OK) {
				TRACE("  SendData() returning %s (%d)",
					strerror(posix_error(status)), (int)posix_error(status));
				return posix_error(status);
			}

			if (!is_writable(fState)) {
				// we only send signals when called from userland
				if (gStackModule->is_syscall())
					send_signal(find_thread(NULL), SIGPIPE);
				return EPIPE;
			}
		}

		size_t size = fSendQueue.Free();
		if (size < left) {
			// we need to split the original buffer
			net_buffer* clone = gBufferModule->clone(buffer, false);
				// TODO: add offset/size parameter to net_buffer::clone() or
				// even a move_data() function, as this is a bit inefficient
			if (clone == NULL)
				return ENOBUFS;

			status_t status = gBufferModule->trim(clone, size);
			if (status != B_OK) {
				gBufferModule->free(clone);
				return status;
			}

			gBufferModule->remove_header(buffer, size);
			left -= size;
			fSendQueue.Add(clone);
		} else {
			left -= buffer->size;
			fSendQueue.Add(buffer);
		}
	}

	TRACE("  SendData(): %lu bytes used.", fSendQueue.Used());

	bool force = false;
	if ((flags & MSG_OOB) != 0) {
		fSendUrgentOffset = fSendQueue.LastSequence();
			// RFC 961 specifies that the urgent offset points to the last
			// byte of urgent data. However, this is commonly implemented as
			// here, ie. it points to the first byte after the urgent data.
		force = true;
	}
	if ((flags & MSG_EOF) != 0)
		_Disconnect(false);

	if (fState == ESTABLISHED || fState == FINISH_RECEIVED)
		_SendQueued(force);

	return B_OK;
}


ssize_t
TCPEndpoint::SendAvailable()
{
	MutexLocker locker(fLock);

	ssize_t available;

	if (is_writable(fState))
		available = fSendQueue.Free();
	else
		available = EPIPE;

	TRACE("SendAvailable(): %li", available);
	return available;
}


status_t
TCPEndpoint::FillStat(net_stat *stat)
{
	MutexLocker _(fLock);

	strlcpy(stat->state, name_for_state(fState), sizeof(stat->state));
	stat->receive_queue_size = fReceiveQueue.Available();
	stat->send_queue_size = fSendQueue.Used();

	return B_OK;
}


status_t
TCPEndpoint::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	TRACE("ReadData(%lu bytes, flags 0x%x)", numBytes, (unsigned int)flags);

	MutexLocker locker(fLock);

	*_buffer = NULL;

	if (fState == CLOSED)
		return ENOTCONN;

	bigtime_t timeout = absolute_timeout(socket->receive.timeout);
	if (gStackModule->is_restarted_syscall())
		timeout = gStackModule->restore_syscall_restart_timeout();
	else
		gStackModule->store_syscall_restart_timeout(timeout);

	if (fState == SYNCHRONIZE_SENT || fState == SYNCHRONIZE_RECEIVED) {
		if (flags & MSG_DONTWAIT)
			return B_WOULD_BLOCK;

		status_t status = _WaitForEstablished(locker, timeout);
		if (status < B_OK)
			return posix_error(status);
	}

	size_t dataNeeded = socket->receive.low_water_mark;

	// When MSG_WAITALL is set then the function should block
	// until the full amount of data can be returned.
	if (flags & MSG_WAITALL)
		dataNeeded = numBytes;

	// TODO: add support for urgent data (MSG_OOB)

	while (true) {
		if (fState == CLOSING || fState == WAIT_FOR_FINISH_ACKNOWLEDGE
			|| fState == TIME_WAIT) {
			// ``Connection closing''.
			return B_OK;
		}

		if (fReceiveQueue.Available() > 0) {
			if (fReceiveQueue.Available() >= dataNeeded
				|| (fReceiveQueue.PushedData() > 0
					&& fReceiveQueue.PushedData() >= fReceiveQueue.Available()))
				break;
		} else if (fState == FINISH_RECEIVED) {
			// ``If no text is awaiting delivery, the RECEIVE will
			//   get a Connection closing''.
			return B_OK;
		}

		if ((flags & MSG_DONTWAIT) != 0 || socket->receive.timeout == 0)
			return B_WOULD_BLOCK;

		if ((fFlags & FLAG_NO_RECEIVE) != 0)
			return B_OK;

		status_t status = fReceiveList.Wait(locker, timeout);
		if (status < B_OK) {
			// The Open Group base specification mentions that EINTR should be
			// returned if the recv() is interrupted before _any data_ is
			// available. So we actually check if there is data, and if so,
			// push it to the user.
			if ((status == B_TIMED_OUT || status == B_INTERRUPTED)
				&& fReceiveQueue.Available() > 0)
				break;

			return posix_error(status);
		}
	}

	TRACE("  ReadData(): %lu are available.", fReceiveQueue.Available());

	if (numBytes < fReceiveQueue.Available())
		fReceiveList.Signal();

	bool clone = (flags & MSG_PEEK) != 0;

	ssize_t receivedBytes = fReceiveQueue.Get(numBytes, !clone, _buffer);

	TRACE("  ReadData(): %lu bytes kept.", fReceiveQueue.Available());

	// if we are opening the window, check if we should send an ACK
	if (!clone)
		SendAcknowledge(false);

	return receivedBytes;
}


ssize_t
TCPEndpoint::ReadAvailable()
{
	MutexLocker locker(fLock);

	TRACE("ReadAvailable(): %li", _AvailableData());

	return _AvailableData();
}


status_t
TCPEndpoint::SetSendBufferSize(size_t length)
{
	MutexLocker _(fLock);
	fSendQueue.SetMaxBytes(length);
	return B_OK;
}


status_t
TCPEndpoint::SetReceiveBufferSize(size_t length)
{
	MutexLocker _(fLock);
	fReceiveQueue.SetMaxBytes(length);
	return B_OK;
}


status_t
TCPEndpoint::GetOption(int option, void* _value, int* _length)
{
	if (*_length != sizeof(int))
		return B_BAD_VALUE;

	int* value = (int*)_value;

	switch (option) {
		case TCP_NODELAY:
			if ((fOptions & TCP_NODELAY) != 0)
				*value = 1;
			else
				*value = 0;
			return B_OK;

		case TCP_MAXSEG:
			*value = fReceiveMaxSegmentSize;
			return B_OK;

		default:
			return B_BAD_VALUE;
	}
}


status_t
TCPEndpoint::SetOption(int option, const void* _value, int length)
{
	if (option != TCP_NODELAY)
		return B_BAD_VALUE;

	if (length != sizeof(int))
		return B_BAD_VALUE;

	const int* value = (const int*)_value;

	MutexLocker _(fLock);
	if (*value)
		fOptions |= TCP_NODELAY;
	else
		fOptions &= ~TCP_NODELAY;

	return B_OK;
}


//	#pragma mark - misc


bool
TCPEndpoint::IsBound() const
{
	return !LocalAddress().IsEmpty(true);
}


bool
TCPEndpoint::IsLocal() const
{
	return (fFlags & FLAG_LOCAL) != 0;
}


status_t
TCPEndpoint::DelayedAcknowledge()
{
	if (gStackModule->cancel_timer(&fDelayedAcknowledgeTimer)) {
		// timer was active, send an ACK now (with the exception above,
		// we send every other ACK)
		return SendAcknowledge(true);
	}

	gStackModule->set_timer(&fDelayedAcknowledgeTimer,
		TCP_DELAYED_ACKNOWLEDGE_TIMEOUT);
	return B_OK;
}


status_t
TCPEndpoint::SendAcknowledge(bool force)
{
	return _SendQueued(force, 0);
}


void
TCPEndpoint::_StartPersistTimer()
{
	gStackModule->set_timer(&fPersistTimer, 1000000LL);
}


void
TCPEndpoint::_EnterTimeWait()
{
	TRACE("_EnterTimeWait()\n");

	_CancelConnectionTimers();

	if (fState == TIME_WAIT && IsLocal()) {
		// we do not use TIME_WAIT state for local connections
		fFlags |= FLAG_DELETE_ON_CLOSE;
		return;
	}

	_UpdateTimeWait();
}


void
TCPEndpoint::_UpdateTimeWait()
{
	gStackModule->set_timer(&fTimeWaitTimer, TCP_MAX_SEGMENT_LIFETIME << 1);
}


void
TCPEndpoint::_CancelConnectionTimers()
{
	gStackModule->cancel_timer(&fRetransmitTimer);
	gStackModule->cancel_timer(&fPersistTimer);
	gStackModule->cancel_timer(&fDelayedAcknowledgeTimer);
}


/*!	Sends the FIN flag to the peer when the connection is still open.
	Moves the endpoint to the next state depending on where it was.
*/
status_t
TCPEndpoint::_Disconnect(bool closing)
{
	tcp_state previousState = fState;

	if (fState == SYNCHRONIZE_RECEIVED || fState == ESTABLISHED)
		fState = FINISH_SENT;
	else if (fState == FINISH_RECEIVED)
		fState = WAIT_FOR_FINISH_ACKNOWLEDGE;
	else
		return B_OK;

	T(State(this));

	status_t status = _SendQueued();
	if (status != B_OK) {
		fState = previousState;
		T(State(this));
		return status;
	}

	return B_OK;
}


void
TCPEndpoint::_MarkEstablished()
{
	fState = ESTABLISHED;
	T(State(this));

	if (gSocketModule->has_parent(socket)) {
		gSocketModule->set_connected(socket);
		release_sem_etc(fAcceptSemaphore, 1, B_DO_NOT_RESCHEDULE);
	}

	fSendList.Signal();
}


status_t
TCPEndpoint::_WaitForEstablished(MutexLocker &locker, bigtime_t timeout)
{
	// TODO: Checking for CLOSED seems correct, but breaks several neon tests.
	// When investigating this, also have a look at _Close() and _HandleReset().
	while (fState < ESTABLISHED/* && fState != CLOSED*/) {
		if (socket->error != B_OK)
			return socket->error;

		status_t status = fSendList.Wait(locker, timeout);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


//	#pragma mark - receive


void
TCPEndpoint::_Close()
{
	_CancelConnectionTimers();
	fState = CLOSED;
	T(State(this));

	fFlags |= FLAG_DELETE_ON_CLOSE;

	fSendList.Signal();
	_NotifyReader();

	if (gSocketModule->has_parent(socket)) {
		// We still have a parent - obviously, we haven't been accepted yet,
		// so no one could ever close us.
		_CancelConnectionTimers();
		gSocketModule->set_aborted(socket);
	}
}


void
TCPEndpoint::_HandleReset(status_t error)
{
	socket->error = error;
	_Close();

	gSocketModule->notify(socket, B_SELECT_WRITE, error);
	gSocketModule->notify(socket, B_SELECT_ERROR, error);
}


void
TCPEndpoint::_DuplicateAcknowledge(tcp_segment_header &segment)
{
	if (++fDuplicateAcknowledgeCount < 3)
		return;

	if (fDuplicateAcknowledgeCount == 3) {
		_ResetSlowStart();
		fCongestionWindow = fSlowStartThreshold + 3
			* fSendMaxSegmentSize;
		fSendNext = segment.acknowledge;
	} else if (fDuplicateAcknowledgeCount > 3)
		fCongestionWindow += fSendMaxSegmentSize;

	_SendQueued();
}


void
TCPEndpoint::_UpdateTimestamps(tcp_segment_header& segment,
	size_t segmentLength)
{
	if (fFlags & FLAG_OPTION_TIMESTAMP) {
		tcp_sequence sequence(segment.sequence);

		if (fLastAcknowledgeSent >= sequence
			&& fLastAcknowledgeSent < (sequence + segmentLength))
			fReceivedTimestamp = segment.timestamp_value;
	}
}


ssize_t
TCPEndpoint::_AvailableData() const
{
	// TODO: Refer to the FLAG_NO_RECEIVE comment above regarding
	//       the application of FLAG_NO_RECEIVE in listen()ing
	//       sockets.
	if (fState == LISTEN)
		return gSocketModule->count_connected(socket);
	if (fState == SYNCHRONIZE_SENT)
		return 0;

	ssize_t availableData = fReceiveQueue.Available();

	if (availableData == 0 && !_ShouldReceive())
		return ENOTCONN;

	return availableData;
}


void
TCPEndpoint::_NotifyReader()
{
	fReceiveList.Signal();
	gSocketModule->notify(socket, B_SELECT_READ, _AvailableData());
}


bool
TCPEndpoint::_AddData(tcp_segment_header& segment, net_buffer* buffer)
{
	if ((segment.flags & TCP_FLAG_FINISH) != 0) {
		// Remember the position of the finish received flag
		fFinishReceived = true;
		fFinishReceivedAt = segment.sequence + buffer->size;
	}

	fReceiveQueue.Add(buffer, segment.sequence);
	fReceiveNext = fReceiveQueue.NextSequence();

	if (fFinishReceived) {
		// Set or reset the finish flag on the current segment
		if (fReceiveNext < fFinishReceivedAt)
			segment.flags &= ~TCP_FLAG_FINISH;
		else
			segment.flags |= TCP_FLAG_FINISH;
	}

	TRACE("  _AddData(): adding data, receive next = %lu. Now have %lu bytes.",
		fReceiveNext.Number(), fReceiveQueue.Available());

	if ((segment.flags & TCP_FLAG_PUSH) != 0)
		fReceiveQueue.SetPushPointer();

	return fReceiveQueue.Available() > 0;
}


void
TCPEndpoint::_PrepareReceivePath(tcp_segment_header& segment)
{
	fInitialReceiveSequence = segment.sequence;
	fFinishReceived = false;

	// count the received SYN
	segment.sequence++;

	fReceiveNext = segment.sequence;
	fReceiveQueue.SetInitialSequence(segment.sequence);

	if ((fOptions & TCP_NOOPT) == 0) {
		if (segment.max_segment_size > 0)
			fSendMaxSegmentSize = segment.max_segment_size;

		if (segment.options & TCP_HAS_WINDOW_SCALE) {
			fFlags |= FLAG_OPTION_WINDOW_SCALE;
			fSendWindowShift = segment.window_shift;
		} else {
			fFlags &= ~FLAG_OPTION_WINDOW_SCALE;
			fReceiveWindowShift = 0;
		}

		if (segment.options & TCP_HAS_TIMESTAMPS) {
			fFlags |= FLAG_OPTION_TIMESTAMP;
			fReceivedTimestamp = segment.timestamp_value;
		} else
			fFlags &= ~FLAG_OPTION_TIMESTAMP;
	}

	fCongestionWindow = 2 * fSendMaxSegmentSize;
	fSlowStartThreshold = (uint32)segment.advertised_window << fSendWindowShift;
}


bool
TCPEndpoint::_ShouldReceive() const
{
	if ((fFlags & FLAG_NO_RECEIVE) != 0)
		return false;

	return fState == ESTABLISHED || fState == FINISH_SENT
		|| fState == FINISH_ACKNOWLEDGED;
}


int32
TCPEndpoint::_Spawn(TCPEndpoint* parent, tcp_segment_header& segment,
	net_buffer* buffer)
{
	MutexLocker _(fLock);

	// TODO error checking
	ProtocolSocket::Open();

	fState = SYNCHRONIZE_RECEIVED;
	T(Spawn(parent, this));

	fManager = parent->fManager;

	LocalAddress().SetTo(buffer->destination);
	PeerAddress().SetTo(buffer->source);

	TRACE("Spawn()");

	// TODO: proper error handling!
	if (fManager->BindChild(this) != B_OK) {
		T(Error(this, "binding failed", __LINE__));
		return DROP;
	}
	if (_PrepareSendPath(*PeerAddress()) != B_OK) {
		T(Error(this, "prepare send faild", __LINE__));
		return DROP;
	}

	fOptions = parent->fOptions;
	fAcceptSemaphore = parent->fAcceptSemaphore;

	_PrepareReceivePath(segment);

	// send SYN+ACK
	if (_SendQueued() != B_OK) {
		T(Error(this, "sending failed", __LINE__));
		return DROP;
	}

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return _Receive(segment, buffer);
}


int32
TCPEndpoint::_ListenReceive(tcp_segment_header& segment, net_buffer* buffer)
{
	TRACE("ListenReceive()");

	// Essentially, we accept only TCP_FLAG_SYNCHRONIZE in this state,
	// but the error behaviour differs
	if (segment.flags & TCP_FLAG_RESET)
		return DROP;
	if (segment.flags & TCP_FLAG_ACKNOWLEDGE)
		return DROP | RESET;
	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) == 0)
		return DROP;

	// TODO: drop broadcast/multicast

	// spawn new endpoint for accept()
	net_socket* newSocket;
	if (gSocketModule->spawn_pending_socket(socket, &newSocket) < B_OK) {
		T(Error(this, "spawning failed", __LINE__));
		return DROP;
	}

	return ((TCPEndpoint *)newSocket->first_protocol)->_Spawn(this,
		segment, buffer);
}


int32
TCPEndpoint::_SynchronizeSentReceive(tcp_segment_header &segment,
	net_buffer *buffer)
{
	TRACE("_SynchronizeSentReceive()");

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
		&& (fInitialSendSequence >= segment.acknowledge
			|| fSendMax < segment.acknowledge))
		return DROP | RESET;

	if (segment.flags & TCP_FLAG_RESET) {
		_HandleReset(ECONNREFUSED);
		return DROP;
	}

	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) == 0)
		return DROP;

	fSendUnacknowledged = segment.acknowledge;
	_PrepareReceivePath(segment);

	if (segment.flags & TCP_FLAG_ACKNOWLEDGE) {
		_MarkEstablished();
	} else {
		// simultaneous open
		fState = SYNCHRONIZE_RECEIVED;
		T(State(this));
	}

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return _Receive(segment, buffer) | IMMEDIATE_ACKNOWLEDGE;
}


int32
TCPEndpoint::_Receive(tcp_segment_header& segment, net_buffer* buffer)
{
	uint32 advertisedWindow = (uint32)segment.advertised_window
		<< fSendWindowShift;
	size_t segmentLength = buffer->size;

	// First, handle the most common case for uni-directional data transfer
	// (known as header prediction - the segment must not change the window,
	// and must be the expected sequence, and contain no control flags)

	if (fState == ESTABLISHED
		&& segment.AcknowledgeOnly()
		&& fReceiveNext == segment.sequence
		&& advertisedWindow > 0 && advertisedWindow == fSendWindow
		&& fSendNext == fSendMax) {
		_UpdateTimestamps(segment, segmentLength);

		if (segmentLength == 0) {
			// this is a pure acknowledge segment - we're on the sending end
			if (fSendUnacknowledged < segment.acknowledge
				&& fSendMax >= segment.acknowledge) {
				_Acknowledged(segment);
				return DROP;
			}
		} else if (segment.acknowledge == fSendUnacknowledged
			&& fReceiveQueue.IsContiguous()
			&& fReceiveQueue.Free() >= segmentLength
			&& (fFlags & FLAG_NO_RECEIVE) == 0) {
			if (_AddData(segment, buffer))
				_NotifyReader();

			return KEEP | ((segment.flags & TCP_FLAG_PUSH) != 0
				? IMMEDIATE_ACKNOWLEDGE : ACKNOWLEDGE);
		}
	}

	// The fast path was not applicable, so we continue with the standard
	// processing of the incoming segment

	ASSERT(fState != SYNCHRONIZE_SENT && fState != LISTEN);

	if (fState != CLOSED && fState != TIME_WAIT) {
		// Check sequence number
		if (!segment_in_sequence(segment, segmentLength, fReceiveNext,
				fReceiveWindow)) {
			TRACE("  Receive(): segment out of window, next: %lu wnd: %lu",
				fReceiveNext.Number(), fReceiveWindow);
			if ((segment.flags & TCP_FLAG_RESET) != 0) {
				// TODO: this doesn't look right - review!
				return DROP;
			}
			return DROP | IMMEDIATE_ACKNOWLEDGE;
		}
	}

	if ((segment.flags & TCP_FLAG_RESET) != 0) {
		// Is this a valid reset?
		// We generally ignore resets in time wait state (see RFC 1337)
		if (fLastAcknowledgeSent <= segment.sequence
			&& tcp_sequence(segment.sequence) < (fLastAcknowledgeSent
				+ fReceiveWindow)
			&& fState != TIME_WAIT) {
			status_t error;
			if (fState == SYNCHRONIZE_RECEIVED)
				error = ECONNREFUSED;
			else if (fState == CLOSING || fState == WAIT_FOR_FINISH_ACKNOWLEDGE)
				error = ENOTCONN;
			else
				error = ECONNRESET;

			_HandleReset(error);
		}

		return DROP;
	}

	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
		|| (fState == SYNCHRONIZE_RECEIVED
			&& (fInitialReceiveSequence > segment.sequence
				|| ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
					&& (fSendUnacknowledged > segment.acknowledge
						|| fSendMax < segment.acknowledge))))) {
		// reset the connection - either the initial SYN was faulty, or we
		// received a SYN within the data stream
		return DROP | RESET;
	}

	// TODO: Check this! Why do we advertize a window outside of what we should
	// buffer?
	fReceiveWindow = max_c(fReceiveQueue.Free(), fReceiveWindow);
		// the window must not shrink
	
	// trim buffer to be within the receive window
	int32 drop = (int32)(fReceiveNext - segment.sequence).Number();
	if (drop > 0) {
		if ((uint32)drop > buffer->size
			|| ((uint32)drop == buffer->size
				&& (segment.flags & TCP_FLAG_FINISH) == 0)) {
			// don't accidently remove a FIN we shouldn't remove
			segment.flags &= ~TCP_FLAG_FINISH;
			drop = buffer->size;
		}

		// remove duplicate data at the start
		TRACE("* remove %ld bytes from the start", drop);
		gBufferModule->remove_header(buffer, drop);
		segment.sequence += drop;
	}

	int32 action = KEEP;

	drop = (int32)(segment.sequence + buffer->size
		- (fReceiveNext + fReceiveWindow)).Number();
	if (drop > 0) {
		// remove data exceeding our window
		if ((uint32)drop >= buffer->size) {
			// if we can accept data, or the segment is not what we'd expect,
			// drop the segment (an immediate acknowledge is always triggered)
			if (fReceiveWindow != 0 || segment.sequence != fReceiveNext)
				return DROP | IMMEDIATE_ACKNOWLEDGE;

			action |= IMMEDIATE_ACKNOWLEDGE;
		}

		if ((segment.flags & TCP_FLAG_FINISH) != 0) {
			// we need to remove the finish, too, as part of the data
			drop--;
		}

		segment.flags &= ~(TCP_FLAG_FINISH | TCP_FLAG_PUSH);
		TRACE("* remove %ld bytes from the end", drop);
		gBufferModule->remove_trailer(buffer, drop);
	}

#ifdef TRACE_TCP
	if (advertisedWindow > fSendWindow) {
		TRACE("  Receive(): Window update %lu -> %lu", fSendWindow,
			advertisedWindow);
	}
#endif

	fSendWindow = advertisedWindow;
	if (advertisedWindow > fSendMaxWindow)
		fSendMaxWindow = advertisedWindow;

	// Then look at the acknowledgement for any updates

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0) {
		// process acknowledged data
		if (fState == SYNCHRONIZE_RECEIVED)
			_MarkEstablished();

		if (fSendMax < segment.acknowledge)
			return DROP | IMMEDIATE_ACKNOWLEDGE;

		if (segment.acknowledge < fSendUnacknowledged) {
			if (buffer->size == 0 && advertisedWindow == fSendWindow
				&& (segment.flags & TCP_FLAG_FINISH) == 0) {
				TRACE("Receive(): duplicate ack!");

				_DuplicateAcknowledge(segment);
			}

			return DROP;
		} else {
			// this segment acknowledges in flight data

			if (fDuplicateAcknowledgeCount >= 3) {
				// deflate the window.
				fCongestionWindow = fSlowStartThreshold;
			}

			fDuplicateAcknowledgeCount = 0;

			if (fSendMax == segment.acknowledge)
				TRACE("Receive(): all inflight data ack'd!");

			if (segment.acknowledge > fSendQueue.LastSequence()
					&& fState > ESTABLISHED) {
				TRACE("Receive(): FIN has been acknowledged!");

				switch (fState) {
					case FINISH_SENT:
						fState = FINISH_ACKNOWLEDGED;
						T(State(this));
						break;
					case CLOSING:
						fState = TIME_WAIT;
						T(State(this));
						_EnterTimeWait();
						return DROP;
					case WAIT_FOR_FINISH_ACKNOWLEDGE:
						_Close();
						break;

					default:
						break;
				}
			}

			if (fState != CLOSED)
				_Acknowledged(segment);
		}
	}

	if (segment.flags & TCP_FLAG_URGENT) {
		if (fState == ESTABLISHED || fState == FINISH_SENT
			|| fState == FINISH_ACKNOWLEDGED) {
			// TODO: Handle urgent data:
			//  - RCV.UP <- max(RCV.UP, SEG.UP)
			//  - signal the user that urgent data is available (SIGURG)
		}
	}

	bool notify = false;

	// The buffer may be freed if its data is added to the queue, so cache
	// the size as we still need it later.
	uint32 bufferSize = buffer->size;

	if ((bufferSize > 0 || (segment.flags & TCP_FLAG_FINISH) != 0)
		&& _ShouldReceive())
		notify = _AddData(segment, buffer);
	else {
		if ((fFlags & FLAG_NO_RECEIVE) != 0)
			fReceiveNext += buffer->size;

		action = (action & ~KEEP) | DROP;
	}

	if ((segment.flags & TCP_FLAG_FINISH) != 0) {
		segmentLength++;
		if (fState != CLOSED && fState != LISTEN && fState != SYNCHRONIZE_SENT) {
			TRACE("Receive(): peer is finishing connection!");
			fReceiveNext++;
			notify = true;

			// FIN implies PUSH
			fReceiveQueue.SetPushPointer();

			// we'll reply immediately to the FIN if we are not
			// transitioning to TIME WAIT so we immediatly ACK it.
			action |= IMMEDIATE_ACKNOWLEDGE;

			// other side is closing connection; change states
			switch (fState) {
				case ESTABLISHED:
				case SYNCHRONIZE_RECEIVED:
					fState = FINISH_RECEIVED;
					T(State(this));
					break;
				case FINISH_SENT:
					// simultaneous close
					fState = CLOSING;
					T(State(this));
					break;
				case FINISH_ACKNOWLEDGED:
					fState = TIME_WAIT;
					T(State(this));
					_EnterTimeWait();
					break;
				case TIME_WAIT:
					_UpdateTimeWait();
					break;

				default:
					break;
			}
		}
	}

	if (notify)
		_NotifyReader();

	if (bufferSize > 0 || (segment.flags & TCP_FLAG_SYNCHRONIZE) != 0)
		action |= ACKNOWLEDGE;

	_UpdateTimestamps(segment, segmentLength);

	TRACE("Receive() Action %ld", action);

	return action;
}


int32
TCPEndpoint::SegmentReceived(tcp_segment_header& segment, net_buffer* buffer)
{
	MutexLocker locker(fLock);

	TRACE("SegmentReceived(): buffer %p (%lu bytes) address %s to %s\n"
		"\tflags 0x%x, seq %lu, ack %lu, wnd %lu",
		buffer, buffer->size, PrintAddress(buffer->source),
		PrintAddress(buffer->destination), segment.flags, segment.sequence,
		segment.acknowledge,
		(uint32)segment.advertised_window << fSendWindowShift);
	T(Receive(this, segment,
		(uint32)segment.advertised_window << fSendWindowShift, buffer));
	int32 segmentAction = DROP;

	switch (fState) {
		case LISTEN:
			segmentAction = _ListenReceive(segment, buffer);
			break;

		case SYNCHRONIZE_SENT:
			segmentAction = _SynchronizeSentReceive(segment, buffer);
			break;

		case SYNCHRONIZE_RECEIVED:
		case ESTABLISHED:
		case FINISH_RECEIVED:
		case WAIT_FOR_FINISH_ACKNOWLEDGE:
		case FINISH_SENT:
		case FINISH_ACKNOWLEDGED:
		case CLOSING:
		case TIME_WAIT:
		case CLOSED:
			segmentAction = _Receive(segment, buffer);
			break;
	}

	// process acknowledge action as asked for by the *Receive() method
	if (segmentAction & IMMEDIATE_ACKNOWLEDGE)
		SendAcknowledge(true);
	else if (segmentAction & ACKNOWLEDGE)
		DelayedAcknowledge();

	if ((fFlags & (FLAG_CLOSED | FLAG_DELETE_ON_CLOSE))
			== (FLAG_CLOSED | FLAG_DELETE_ON_CLOSE)) {
		locker.Unlock();
		gSocketModule->release_socket(socket);
	}

	return segmentAction;
}


//	#pragma mark - send


inline uint8
TCPEndpoint::_CurrentFlags()
{
	// we don't set FLAG_FINISH here, instead we do it
	// conditionally below depending if we are sending
	// the last bytes of the send queue.

	switch (fState) {
		case CLOSED:
			return TCP_FLAG_RESET | TCP_FLAG_ACKNOWLEDGE;

		case SYNCHRONIZE_SENT:
			return TCP_FLAG_SYNCHRONIZE;
		case SYNCHRONIZE_RECEIVED:
			return TCP_FLAG_SYNCHRONIZE | TCP_FLAG_ACKNOWLEDGE;

		case ESTABLISHED:
		case FINISH_RECEIVED:
		case FINISH_ACKNOWLEDGED:
		case TIME_WAIT:
		case WAIT_FOR_FINISH_ACKNOWLEDGE:
		case FINISH_SENT:
		case CLOSING:
			return TCP_FLAG_ACKNOWLEDGE;

		default:
			return 0;
	}
}


inline bool
TCPEndpoint::_ShouldSendSegment(tcp_segment_header& segment, uint32 length,
	uint32 segmentMaxSize, uint32 flightSize)
{
	if (length > 0) {
		// Avoid the silly window syndrome - we only send a segment in case:
		// - we have a full segment to send, or
		// - we're at the end of our buffer queue, or
		// - the buffer is at least larger than half of the maximum send window,
		//   or
		// - we're retransmitting data
		if (length == segmentMaxSize
			|| (fOptions & TCP_NODELAY) != 0
			|| tcp_sequence(fSendNext + length) == fSendQueue.LastSequence()
			|| (fSendMaxWindow > 0 && length >= fSendMaxWindow / 2))
			return true;
	}

	// check if we need to send a window update to the peer
	if (segment.advertised_window > 0) {
		// correct the window to take into account what already has been advertised
		uint32 window = (segment.advertised_window << fReceiveWindowShift)
			- (fReceiveMaxAdvertised - fReceiveNext).Number();

		// if we can advertise a window larger than twice the maximum segment
		// size, or half the maximum buffer size we send a window update
		if (window >= (fReceiveMaxSegmentSize << 1)
			|| window >= (socket->receive.buffer_size >> 1))
			return true;
	}

	if ((segment.flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH
			| TCP_FLAG_RESET)) != 0)
		return true;

	// We do have urgent data pending
	if (fSendUrgentOffset > fSendNext)
		return true;

	// there is no reason to send a segment just now
	return false;
}


status_t
TCPEndpoint::_SendQueued(bool force)
{
	return _SendQueued(force, fSendWindow);
}


/*!	Sends one or more TCP segments with the data waiting in the queue, or some
	specific flags that need to be sent.
*/
status_t
TCPEndpoint::_SendQueued(bool force, uint32 sendWindow)
{
	if (fRoute == NULL)
		return B_ERROR;

	// in passive state?
	if (fState == LISTEN)
		return B_ERROR;

	tcp_segment_header segment(_CurrentFlags());

	if ((fOptions & TCP_NOOPT) == 0) {
		if ((fFlags & FLAG_OPTION_TIMESTAMP) != 0) {
			segment.options |= TCP_HAS_TIMESTAMPS;
			segment.timestamp_reply = fReceivedTimestamp;
			segment.timestamp_value = tcp_now();
		}

		if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
			&& fSendNext == fInitialSendSequence) {
			// add connection establishment options
			segment.max_segment_size = fReceiveMaxSegmentSize;
			if (fFlags & FLAG_OPTION_WINDOW_SCALE) {
				segment.options |= TCP_HAS_WINDOW_SCALE;
				segment.window_shift = fReceiveWindowShift;
			}
		}
	}

	size_t availableBytes = fReceiveQueue.Free();
	if (fFlags & FLAG_OPTION_WINDOW_SCALE)
		segment.advertised_window = availableBytes >> fReceiveWindowShift;
	else
		segment.advertised_window = min_c(TCP_MAX_WINDOW, availableBytes);

	segment.acknowledge = fReceiveNext.Number();

	// Process urgent data
	if (fSendUrgentOffset > fSendNext) {
		segment.flags |= TCP_FLAG_URGENT;
		segment.urgent_offset = (fSendUrgentOffset - fSendNext).Number();
	} else {
		fSendUrgentOffset = fSendUnacknowledged.Number();
			// Keep urgent offset updated, so that it doesn't reach into our
			// send window on overlap
		segment.urgent_offset = 0;
	}

	if (fCongestionWindow > 0 && fCongestionWindow < sendWindow)
		sendWindow = fCongestionWindow;

	// fSendUnacknowledged
	//  |    fSendNext      fSendMax
	//  |        |              |
	//  v        v              v
	//  -----------------------------------
	//  | effective window           |
	//  -----------------------------------

	// Flight size represents the window of data which is currently in the
	// ether. We should never send data such as the flight size becomes larger
	// than the effective window. Note however that the effective window may be
	// reduced (by congestion for instance), so at some point in time flight
	// size may be larger than the currently calculated window.

	uint32 flightSize = (fSendMax - fSendUnacknowledged).Number();
	uint32 consumedWindow = (fSendNext - fSendUnacknowledged).Number();

	if (consumedWindow > sendWindow) {
		sendWindow = 0;
		// TODO: enter persist state? try to get a window update.
	} else
		sendWindow -= consumedWindow;

	if (force && sendWindow == 0 && fSendNext <= fSendQueue.LastSequence()) {
		// send one byte of data to ask for a window update
		// (triggered by the persist timer)
		sendWindow = 1;
	}

	uint32 length = min_c(fSendQueue.Available(fSendNext), sendWindow);
	tcp_sequence previousSendNext = fSendNext;

	do {
		uint32 segmentMaxSize = fSendMaxSegmentSize
			- tcp_options_length(segment);
		uint32 segmentLength = min_c(length, segmentMaxSize);

		if (fSendNext + segmentLength == fSendQueue.LastSequence()) {
			if (state_needs_finish(fState))
				segment.flags |= TCP_FLAG_FINISH;
			if (length > 0)
				segment.flags |= TCP_FLAG_PUSH;
		}

		// Determine if we should really send this segment
		if (!force && !_ShouldSendSegment(segment, segmentLength,
				segmentMaxSize, flightSize)) {
			if (fSendQueue.Available()
				&& !gStackModule->is_timer_active(&fPersistTimer)
				&& !gStackModule->is_timer_active(&fRetransmitTimer))
				_StartPersistTimer();
			break;
		}

		net_buffer *buffer = gBufferModule->create(256);
		if (buffer == NULL)
			return B_NO_MEMORY;

		status_t status = B_OK;
		if (segmentLength > 0)
			status = fSendQueue.Get(buffer, fSendNext, segmentLength);
		if (status < B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		LocalAddress().CopyTo(buffer->source);
		PeerAddress().CopyTo(buffer->destination);

		uint32 size = buffer->size;
		segment.sequence = fSendNext.Number();

		TRACE("SendQueued(): buffer %p (%lu bytes) address %s to %s\n"
			"\tflags 0x%x, seq %lu, ack %lu, rwnd %hu, cwnd %lu, ssthresh %lu\n"
			"\tlen %lu first %lu last %lu",
			buffer, buffer->size, PrintAddress(buffer->source),
			PrintAddress(buffer->destination), segment.flags, segment.sequence,
			segment.acknowledge, segment.advertised_window,
			fCongestionWindow, fSlowStartThreshold, segmentLength,
			fSendQueue.FirstSequence().Number(),
			fSendQueue.LastSequence().Number());
		T(Send(this, segment, buffer, fSendQueue.FirstSequence(),
			fSendQueue.LastSequence()));

		PROBE(buffer, sendWindow);
		sendWindow -= buffer->size;

		status = add_tcp_header(AddressModule(), segment, buffer);
		if (status != B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		// Update send status - we need to do this before we send the data
		// for local connections as the answer is directly handled

		if (segment.flags & TCP_FLAG_SYNCHRONIZE) {
			segment.options &= ~TCP_HAS_WINDOW_SCALE;
			segment.max_segment_size = 0;
			size++;
		}

		if (segment.flags & TCP_FLAG_FINISH)
			size++;

		uint32 sendMax = fSendMax.Number();
		fSendNext += size;
		if (fSendMax < fSendNext)
			fSendMax = fSendNext;

		fReceiveMaxAdvertised = fReceiveNext
			+ ((uint32)segment.advertised_window << fReceiveWindowShift);

		status = next->module->send_routed_data(next, fRoute, buffer);
		if (status < B_OK) {
			gBufferModule->free(buffer);

			fSendNext = segment.sequence;
			fSendMax = sendMax;
				// restore send status
			return status;
		}

		if (segment.flags & TCP_FLAG_ACKNOWLEDGE)
			fLastAcknowledgeSent = segment.acknowledge;

		length -= segmentLength;
		segment.flags &= ~(TCP_FLAG_SYNCHRONIZE | TCP_FLAG_RESET
			| TCP_FLAG_FINISH);
	} while (length > 0);

	// if we sent data from the beggining of the send queue,
	// start the retransmition timer
	if (previousSendNext == fSendUnacknowledged
		&& fSendNext > previousSendNext) {
		TRACE("  SendQueue(): set retransmit timer with rto %llu",
			fRetransmitTimeout);

		gStackModule->set_timer(&fRetransmitTimer, fRetransmitTimeout);
	}

	return B_OK;
}


int
TCPEndpoint::_MaxSegmentSize(const sockaddr* address) const
{
	return next->module->get_mtu(next, address) - sizeof(tcp_header);
}


status_t
TCPEndpoint::_PrepareSendPath(const sockaddr* peer)
{
	if (fRoute == NULL) {
		fRoute = gDatalinkModule->get_route(Domain(), peer);
		if (fRoute == NULL)
			return ENETUNREACH;

		if ((fRoute->flags & RTF_LOCAL) != 0)
			fFlags |= FLAG_LOCAL;
	}

	// make sure connection does not already exist
	status_t status = fManager->SetConnection(this, *LocalAddress(), peer,
		fRoute->interface_address->local);
	if (status < B_OK)
		return status;

	fInitialSendSequence = system_time() >> 4;
	fSendNext = fInitialSendSequence;
	fSendUnacknowledged = fInitialSendSequence;
	fSendMax = fInitialSendSequence;
	fSendUrgentOffset = fInitialSendSequence;

	// we are counting the SYN here
	fSendQueue.SetInitialSequence(fSendNext + 1);

	fReceiveMaxSegmentSize = _MaxSegmentSize(peer);

	// Compute the window shift we advertise to our peer - if it doesn't support
	// this option, this will be reset to 0 (when its SYN is received)
	fReceiveWindowShift = 0;
	while (fReceiveWindowShift < TCP_MAX_WINDOW_SHIFT
		&& (0xffffUL << fReceiveWindowShift) < socket->receive.buffer_size) {
		fReceiveWindowShift++;
	}

	return B_OK;
}


void
TCPEndpoint::_Acknowledged(tcp_segment_header& segment)
{
	size_t previouslyUsed = fSendQueue.Used();

	fSendQueue.RemoveUntil(segment.acknowledge);
	fSendUnacknowledged = segment.acknowledge;

	if (fSendNext < fSendUnacknowledged)
		fSendNext = fSendUnacknowledged;

	if (fSendUnacknowledged == fSendMax)
		gStackModule->cancel_timer(&fRetransmitTimer);

	if (fSendQueue.Used() < previouslyUsed) {
		// this ACK acknowledged data

		if (segment.options & TCP_HAS_TIMESTAMPS)
			_UpdateRoundTripTime(tcp_diff_timestamp(segment.timestamp_reply));
		else {
			// TODO: Fallback to RFC 793 type estimation
		}

		if (is_writable(fState)) {
			// notify threads waiting on the socket to become writable again
			fSendList.Signal();
			gSocketModule->notify(socket, B_SELECT_WRITE, fSendQueue.Used());
		}

		if (fCongestionWindow < fSlowStartThreshold)
			fCongestionWindow += fSendMaxSegmentSize;
	}

	if (fCongestionWindow >= fSlowStartThreshold) {
		uint32 increment = fSendMaxSegmentSize * fSendMaxSegmentSize;

		if (increment < fCongestionWindow)
			increment = 1;
		else
			increment /= fCongestionWindow;

		fCongestionWindow += increment;
	}

	// if there is data left to be send, send it now
	if (fSendQueue.Used() > 0)
		_SendQueued();
}


void
TCPEndpoint::_Retransmit()
{
	TRACE("Retransmit()");
	_ResetSlowStart();
	fSendNext = fSendUnacknowledged;
	_SendQueued();
}


void
TCPEndpoint::_UpdateRoundTripTime(int32 roundTripTime)
{
	int32 rtt = roundTripTime;

	// "smooth" round trip time as per Van Jacobson
	rtt -= fRoundTripTime / 8;
	fRoundTripTime += rtt;
	if (rtt < 0)
		rtt = -rtt;
	rtt -= fRoundTripDeviation / 4;
	fRoundTripDeviation += rtt;

	fRetransmitTimeout = ((fRoundTripTime / 4 + fRoundTripDeviation) / 2)
		* kTimestampFactor;

	TRACE("  RTO is now %llu (after rtt %ldms)", fRetransmitTimeout,
		roundTripTime);
}


void
TCPEndpoint::_ResetSlowStart()
{
	fSlowStartThreshold = max_c((fSendMax - fSendUnacknowledged).Number() / 2,
		2 * fSendMaxSegmentSize);
	fCongestionWindow = fSendMaxSegmentSize;
}


//	#pragma mark - timer


/*static*/ void
TCPEndpoint::_RetransmitTimer(net_timer* timer, void* _endpoint)
{
	TCPEndpoint* endpoint = (TCPEndpoint*)_endpoint;
	T(Timer(endpoint, "retransmit"));

	MutexLocker locker(endpoint->fLock);
	if (!locker.IsLocked())
		return;

	endpoint->_Retransmit();
}


/*static*/ void
TCPEndpoint::_PersistTimer(net_timer* timer, void* _endpoint)
{
	TCPEndpoint* endpoint = (TCPEndpoint*)_endpoint;
	T(Timer(endpoint, "persist"));

	MutexLocker locker(endpoint->fLock);
	if (!locker.IsLocked())
		return;

	// the timer might not have been canceled early enough
	if (endpoint->State() == CLOSED)
		return;

	endpoint->_SendQueued(true);
}


/*static*/ void
TCPEndpoint::_DelayedAcknowledgeTimer(net_timer* timer, void* _endpoint)
{
	TCPEndpoint* endpoint = (TCPEndpoint*)_endpoint;
	T(Timer(endpoint, "delayed ack"));

	MutexLocker locker(endpoint->fLock);
	if (!locker.IsLocked())
		return;

	// the timer might not have been canceled early enough
	if (endpoint->State() == CLOSED)
		return;

	endpoint->SendAcknowledge(true);
}


/*static*/ void
TCPEndpoint::_TimeWaitTimer(net_timer* timer, void* _endpoint)
{
	TCPEndpoint* endpoint = (TCPEndpoint*)_endpoint;
	T(Timer(endpoint, "time-wait"));

	MutexLocker locker(endpoint->fLock);
	if (!locker.IsLocked())
		return;

	if ((endpoint->fFlags & FLAG_CLOSED) == 0) {
		endpoint->fFlags |= FLAG_DELETE_ON_CLOSE;
		return;
	}

	locker.Unlock();

	gSocketModule->release_socket(endpoint->socket);
}


//	#pragma mark -


void
TCPEndpoint::Dump() const
{
	kprintf("TCP endpoint %p\n", this);
	kprintf("  state: %s\n", name_for_state(fState));
	kprintf("  flags: 0x%" B_PRIx32 "\n", fFlags);
#if KDEBUG
	kprintf("  lock: { %p, holder: %" B_PRId32 " }\n", &fLock, fLock.holder);
#endif
	kprintf("  accept sem: %" B_PRId32 "\n", fAcceptSemaphore);
	kprintf("  options: 0x%" B_PRIx32 "\n", (uint32)fOptions);
	kprintf("  send\n");
	kprintf("    window shift: %u\n", fSendWindowShift);
	kprintf("    unacknowledged: %" B_PRIu32 "\n",
		fSendUnacknowledged.Number());
	kprintf("    next: %" B_PRIu32 "\n", fSendNext.Number());
	kprintf("    max: %" B_PRIu32 "\n", fSendMax.Number());
	kprintf("    urgent offset: %" B_PRIu32 "\n", fSendUrgentOffset.Number());
	kprintf("    window: %" B_PRIu32 "\n", fSendWindow);
	kprintf("    max window: %" B_PRIu32 "\n", fSendMaxWindow);
	kprintf("    max segment size: %" B_PRIu32 "\n", fSendMaxSegmentSize);
	kprintf("    queue: %lu / %lu\n", fSendQueue.Used(), fSendQueue.Size());
#if DEBUG_BUFFER_QUEUE
	fSendQueue.Dump();
#endif
	kprintf("    last acknowledge sent: %" B_PRIu32 "\n",
		fLastAcknowledgeSent.Number());
	kprintf("    initial sequence: %" B_PRIu32 "\n",
		fInitialSendSequence.Number());
	kprintf("  receive\n");
	kprintf("    window shift: %u\n", fReceiveWindowShift);
	kprintf("    next: %" B_PRIu32 "\n", fReceiveNext.Number());
	kprintf("    max advertised: %" B_PRIu32 "\n",
		fReceiveMaxAdvertised.Number());
	kprintf("    window: %" B_PRIu32 "\n", fReceiveWindow);
	kprintf("    max segment size: %" B_PRIu32 "\n", fReceiveMaxSegmentSize);
	kprintf("    queue: %lu / %lu\n", fReceiveQueue.Available(),
		fReceiveQueue.Size());
#if DEBUG_BUFFER_QUEUE
	fReceiveQueue.Dump();
#endif
	kprintf("    initial sequence: %" B_PRIu32 "\n",
		fInitialReceiveSequence.Number());
	kprintf("    duplicate acknowledge count: %" B_PRIu32 "\n",
		fDuplicateAcknowledgeCount);
	kprintf("  round trip time: %" B_PRId32 " (deviation %" B_PRId32 ")\n",
		fRoundTripTime, fRoundTripDeviation);
	kprintf("  retransmit timeout: %" B_PRId64 "\n", fRetransmitTimeout);
	kprintf("  congestion window: %" B_PRIu32 "\n", fCongestionWindow);
	kprintf("  slow start threshold: %" B_PRIu32 "\n", fSlowStartThreshold);
}


/*
 * Copyright 2007-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef PROTOCOL_UTILITIES_H
#define PROTOCOL_UTILITIES_H


#include <lock.h>
#include <Select.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <AddressUtilities.h>
#include <net_buffer.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>


class MutexLocking {
public:
	typedef mutex Type;
	typedef MutexLocker AutoLocker;

	static status_t Init(mutex* lock, const char* name)
		{ mutex_init_etc(lock, name, MUTEX_FLAG_CLONE_NAME); return B_OK; }
	static void Destroy(mutex* lock) { mutex_destroy(lock); }
	static status_t Lock(mutex* lock) { return mutex_lock(lock); }
	static status_t Unlock(mutex* lock) { mutex_unlock(lock); return B_OK; }
};


extern net_buffer_module_info* gBufferModule;
extern net_stack_module_info* gStackModule;

class NetModuleBundleGetter {
public:
	static net_stack_module_info* Stack() { return gStackModule; }
	static net_buffer_module_info* Buffer() { return gBufferModule; }
};


class ProtocolSocket {
public:
								ProtocolSocket(net_socket* socket);

			status_t			Open();

			SocketAddress		LocalAddress()
									{ return SocketAddress(
										fDomain->address_module,
										&fSocket->address); }
			ConstSocketAddress	LocalAddress() const
									{ return ConstSocketAddress(
										fDomain->address_module,
										&fSocket->address); }

			SocketAddress		PeerAddress()
									{ return SocketAddress(
										fDomain->address_module,
										&fSocket->peer); }
			ConstSocketAddress	PeerAddress() const
									{ return ConstSocketAddress(
										fDomain->address_module,
										&fSocket->peer); }

			net_domain*			Domain() const { return fDomain; }
			net_address_module_info* AddressModule() const
									{ return fDomain->address_module; }

			net_socket*			Socket() const { return fSocket; }

protected:
			net_socket*			fSocket;
			net_domain*			fDomain;
};


inline
ProtocolSocket::ProtocolSocket(net_socket* socket)
	:
	fSocket(socket),
	fDomain(NULL)
{
}


inline status_t
ProtocolSocket::Open()
{
	fDomain = fSocket->first_protocol->module->get_domain(
		fSocket->first_protocol);

	if (fDomain == NULL || fDomain->address_module == NULL)
		return EAFNOSUPPORT;

	return B_OK;
}


template<typename LockingBase = MutexLocking,
	typename ModuleBundle = NetModuleBundleGetter>
class DatagramSocket : public ProtocolSocket {
public:
								DatagramSocket(const char* name,
									net_socket* socket);
	virtual						~DatagramSocket();

			status_t			InitCheck() const;

			status_t			Enqueue(net_buffer* buffer);
			status_t			EnqueueClone(net_buffer* buffer);

			status_t			Dequeue(uint32 flags, net_buffer** _buffer);
			net_buffer*			Dequeue(bool clone);
			status_t			BlockingDequeue(bool peek, bigtime_t timeout,
									net_buffer** _buffer);

			void				Clear();

			bool				IsEmpty() const { return fBuffers.IsEmpty(); }
			ssize_t				AvailableData() const;

			void				WakeAll();
			void				NotifyOne();

protected:
	virtual	status_t			SocketStatus(bool peek) const;

private:
			status_t			_Enqueue(net_buffer* buffer);
			net_buffer*			_Dequeue(bool peek);
			void				_Clear();

			status_t			_Wait(bigtime_t timeout);
			void				_NotifyOneReader(bool notifySocket);

			bigtime_t			_SocketTimeout(uint32 flags) const;

protected:
	typedef typename LockingBase::Type LockType;
	typedef typename LockingBase::AutoLocker AutoLocker;
	typedef DoublyLinkedListCLink<net_buffer> NetBufferLink;
	typedef DoublyLinkedList<net_buffer, NetBufferLink> BufferList;

			sem_id				fNotify;
			BufferList			fBuffers;
			size_t				fCurrentBytes;
	mutable	LockType			fLock;
};


#define DECL_DATAGRAM_SOCKET(args) \
	template<typename LockingBase, typename ModuleBundle> args \
	DatagramSocket<LockingBase, ModuleBundle>


DECL_DATAGRAM_SOCKET(inline)::DatagramSocket(const char* name,
	net_socket* socket)
	:
	ProtocolSocket(socket), fCurrentBytes(0)
{
	status_t status = LockingBase::Init(&fLock, name);
	if (status != B_OK)
		fNotify = status;
	else
		fNotify = create_sem(0, name);
}


DECL_DATAGRAM_SOCKET(inline)::~DatagramSocket()
{
	_Clear();
	delete_sem(fNotify);
	LockingBase::Destroy(&fLock);
}


DECL_DATAGRAM_SOCKET(inline status_t)::InitCheck() const
{
	return fNotify >= 0 ? B_OK : fNotify;
}


DECL_DATAGRAM_SOCKET(inline status_t)::Enqueue(net_buffer* buffer)
{
	AutoLocker _(fLock);
	return _Enqueue(buffer);
}


DECL_DATAGRAM_SOCKET(inline status_t)::EnqueueClone(net_buffer* _buffer)
{
	AutoLocker _(fLock);

	net_buffer* buffer = ModuleBundle::Buffer()->clone(_buffer, false);
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = _Enqueue(buffer);
	if (status != B_OK)
		ModuleBundle::Buffer()->free(buffer);

	return status;
}


DECL_DATAGRAM_SOCKET(inline status_t)::Dequeue(uint32 flags,
	net_buffer** _buffer)
{
	return BlockingDequeue((flags & MSG_PEEK) != 0, _SocketTimeout(flags),
		_buffer);
}


DECL_DATAGRAM_SOCKET(inline net_buffer*)::Dequeue(bool peek)
{
	AutoLocker _(fLock);
	return _Dequeue(peek);
}


DECL_DATAGRAM_SOCKET(inline status_t)::BlockingDequeue(bool peek,
	bigtime_t timeout, net_buffer** _buffer)
{
	AutoLocker _(fLock);

	bool waited = false;
	while (fBuffers.IsEmpty()) {
		status_t status = SocketStatus(peek);
		if (status != B_OK) {
			if (peek)
				_NotifyOneReader(false);
			return status;
		}

		status = _Wait(timeout);
		if (status != B_OK)
			return status;

		waited = true;
	}

	*_buffer = _Dequeue(peek);
	if (peek && waited) {
		// There is a new buffer in the list; but since we are only peeking,
		// notify the next waiting reader.
		_NotifyOneReader(false);
	}

	if (*_buffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


DECL_DATAGRAM_SOCKET(inline void)::Clear()
{
	AutoLocker _(fLock);
	_Clear();
}


DECL_DATAGRAM_SOCKET(inline ssize_t)::AvailableData() const
{
	AutoLocker _(fLock);
	status_t status = SocketStatus(true);
	if (status < B_OK)
		return status;

	return fCurrentBytes;
}


DECL_DATAGRAM_SOCKET(inline void)::WakeAll()
{
	release_sem_etc(fNotify, 0, B_RELEASE_ALL);
}


DECL_DATAGRAM_SOCKET(inline void)::NotifyOne()
{
	release_sem_etc(fNotify, 1, B_RELEASE_IF_WAITING_ONLY
		| B_DO_NOT_RESCHEDULE);
}


DECL_DATAGRAM_SOCKET(inline status_t)::SocketStatus(bool peek) const
{
	if (peek)
		return fSocket->error;
	
	status_t status = fSocket->error;
	fSocket->error = B_OK;

	return status;
}


DECL_DATAGRAM_SOCKET(inline status_t)::_Enqueue(net_buffer* buffer)
{
	if (fSocket->receive.buffer_size > 0
		&& (fCurrentBytes + buffer->size) > fSocket->receive.buffer_size)
		return ENOBUFS;

	fBuffers.Add(buffer);
	fCurrentBytes += buffer->size;

	_NotifyOneReader(true);

	return B_OK;
}


DECL_DATAGRAM_SOCKET(inline net_buffer*)::_Dequeue(bool peek)
{
	if (fBuffers.IsEmpty())
		return NULL;

	if (peek)
		return ModuleBundle::Buffer()->clone(fBuffers.Head(), false);

	net_buffer* buffer = fBuffers.RemoveHead();
	fCurrentBytes -= buffer->size;

	return buffer;
}


DECL_DATAGRAM_SOCKET(inline void)::_Clear()
{
	BufferList::Iterator it = fBuffers.GetIterator();
	while (it.HasNext())
		ModuleBundle::Buffer()->free(it.Next());
	fCurrentBytes = 0;
}


DECL_DATAGRAM_SOCKET(inline status_t)::_Wait(bigtime_t timeout)
{
	LockingBase::Unlock(&fLock);
	status_t status = acquire_sem_etc(fNotify, 1, B_CAN_INTERRUPT
		| B_ABSOLUTE_TIMEOUT, timeout);
	LockingBase::Lock(&fLock);

	return status;
}


DECL_DATAGRAM_SOCKET(inline void)::_NotifyOneReader(bool notifySocket)
{
	release_sem_etc(fNotify, 1, B_RELEASE_IF_WAITING_ONLY
		| B_DO_NOT_RESCHEDULE);

	if (notifySocket) {
		ModuleBundle::Stack()->notify_socket(fSocket, B_SELECT_READ,
			fCurrentBytes);
	}
}


DECL_DATAGRAM_SOCKET(inline bigtime_t)::_SocketTimeout(uint32 flags) const
{
	bigtime_t timeout = fSocket->receive.timeout;

	if ((flags & MSG_DONTWAIT) != 0)
		timeout = 0;
	else if (timeout != 0 && timeout != B_INFINITE_TIMEOUT)
		timeout += system_time();

	if (ModuleBundle::Stack()->is_restarted_syscall())
		timeout = ModuleBundle::Stack()->restore_syscall_restart_timeout();
	else
		ModuleBundle::Stack()->store_syscall_restart_timeout(timeout);

	return timeout;
}


#endif	// PROTOCOL_UTILITIES_H

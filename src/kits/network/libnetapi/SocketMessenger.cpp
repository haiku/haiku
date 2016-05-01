/*
 * Copyright 2009-2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 */


#include <SocketMessenger.h>

#include <Message.h>
#include <Messenger.h>

#include <AutoDeleter.h>


BSocketMessenger::BSocketMessenger()
	:
	fSocket(),
	fInitStatus(B_NO_INIT)
{
}


BSocketMessenger::BSocketMessenger(const BNetworkAddress& address,
	bigtime_t timeout)
{
	SetTo(address, timeout);
}


BSocketMessenger::BSocketMessenger(const BSocket& socket)
	:
	fSocket(socket)
{
	fInitStatus = socket.InitCheck();
}


BSocketMessenger::~BSocketMessenger()
{
	Unset();
}


void
BSocketMessenger::Unset()
{
	fSocket.Disconnect();
	fInitStatus = B_NO_INIT;
}


status_t
BSocketMessenger::SetTo(const BNetworkAddress& address, bigtime_t timeout)
{
	return fInitStatus = fSocket.Connect(address, timeout);
}


status_t
BSocketMessenger::SetTo(const BSocketMessenger& target, bigtime_t timeout)
{
	return SetTo(target.Address(), timeout);
}


status_t
BSocketMessenger::SendMessage(const BMessage& message)
{
	return _SendMessage(message);
}


status_t
BSocketMessenger::SendMessage(const BMessage& message, BMessage& _reply,
	bigtime_t timeout)
{
	status_t error = _SendMessage(message);
	if (error != B_OK)
		return error;

	return _ReadMessage(_reply, timeout);
}


status_t
BSocketMessenger::SendMessage(const BMessage& message,
	BMessenger& replyTarget, bigtime_t timeout)
{
	BMessage reply;
	status_t error = SendMessage(message, reply, timeout);
	if (error != B_OK)
		return error;

	return replyTarget.SendMessage(&reply);
}


status_t
BSocketMessenger::ReceiveMessage(BMessage& _message, bigtime_t timeout)
{
	return _ReadMessage(_message, timeout);
}


status_t
BSocketMessenger::_SendMessage(const BMessage& message)
{
	ssize_t flatSize = message.FlattenedSize();
	ssize_t totalSize = flatSize + sizeof(ssize_t);

	char* buffer = new(std::nothrow) char[totalSize];
	if (buffer == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<char> bufferDeleter(buffer);
	*(ssize_t*)buffer = flatSize;
	char* messageBuffer = buffer + sizeof(ssize_t);
	status_t error = message.Flatten(messageBuffer, flatSize);
	if (error != B_OK)
		return error;

	ssize_t size = fSocket.Write(buffer, totalSize);
	if (size < 0)
		return size;

	return B_OK;
}


status_t
BSocketMessenger::_ReadMessage(BMessage& _message, bigtime_t timeout)
{
	status_t error = fSocket.WaitForReadable(timeout);
	if (error != B_OK)
		return error;

	ssize_t size = 0;
	ssize_t readSize = fSocket.Read(&size, sizeof(ssize_t));
	if (readSize < 0)
		return readSize;

	if (readSize != sizeof(ssize_t))
		return B_BAD_VALUE;

	if (size <= 0)
		return B_MISMATCHED_VALUES;

	char* buffer = new(std::nothrow) char[size];
	if (buffer == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<char> bufferDeleter(buffer);
	readSize = fSocket.Read(buffer, size);
	if (readSize < 0)
		return readSize;

	if (readSize != size)
		return B_MISMATCHED_VALUES;

	return _message.Unflatten(buffer);
}

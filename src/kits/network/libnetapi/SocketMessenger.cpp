/*
 * Copyright 2009-2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 */


#include <SocketMessenger.h>

#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <HashMap.h>


static const char* kReplySenderIDField = "socket_messenger:sender_reply_id";
static const char* kReplyReceiverIDField = "socket_messenger:reply_id";


// #pragma mark - BSocketMessenger::Private


struct BSocketMessenger::Private {
			typedef SynchronizedHashMap<HashKey64<int64>,
									BMessage> ReplyMessageMap;

								Private();
	virtual						~Private();

			void				ClearMessages();

			sem_id				fMessageWaiters;
			thread_id			fReplyReader;
			ReplyMessageMap		fReceivedReplies;
			BMessageQueue		fReceivedMessages;
			int64				fReplyIDCounter;
};


BSocketMessenger::Private::Private()
	:
	fMessageWaiters(-1),
	fReplyReader(-1),
	fReceivedReplies(),
	fReceivedMessages(),
	fReplyIDCounter(0)
{
}


BSocketMessenger::Private::~Private()
{
	if (fMessageWaiters > 0)
		delete_sem(fMessageWaiters);
	if (fReplyReader > 0)
		wait_for_thread(fReplyReader, NULL);

	ClearMessages();
}


void
BSocketMessenger::Private::ClearMessages()
{
	fReceivedReplies.Clear();
	AutoLocker<BMessageQueue> queueLocker(fReceivedMessages);
	while (!fReceivedMessages.IsEmpty())
		delete fReceivedMessages.NextMessage();
}


// #pragma mark - BSocketMessenger


BSocketMessenger::BSocketMessenger()
	:
	fPrivateData(NULL),
	fSocket(),
	fInitStatus(B_NO_INIT)
{
	_Init();
}


BSocketMessenger::BSocketMessenger(const BNetworkAddress& address,
	bigtime_t timeout)
	:
	fPrivateData(NULL),
	fSocket(),
	fInitStatus(B_NO_INIT)
{
	_Init();
	SetTo(address, timeout);
}


BSocketMessenger::BSocketMessenger(const BSocket& socket)
	:
	fPrivateData(NULL),
	fSocket(socket),
	fInitStatus(B_NO_INIT)
{
	_Init();
	if (fPrivateData == NULL)
		return;

	fInitStatus = socket.InitCheck();
	if (fInitStatus != B_OK)
		return;

	fPrivateData->fReplyReader = spawn_thread(&_MessageReader,
		"Message Reader", B_NORMAL_PRIORITY, this);
	if (fPrivateData->fReplyReader < 0)
		fInitStatus = fPrivateData->fReplyReader;
	if (fInitStatus != B_OK) {
		exit_thread(fPrivateData->fReplyReader);
		fPrivateData->fReplyReader = -1;
		return;
	}

	fInitStatus = resume_thread(fPrivateData->fReplyReader);
}


BSocketMessenger::~BSocketMessenger()
{
	Unset();

	delete fPrivateData;
}


void
BSocketMessenger::Unset()
{
	if (fPrivateData == NULL)
		return;

	fSocket.Disconnect();
	wait_for_thread(fPrivateData->fReplyReader, NULL);
	fPrivateData->fReplyReader = -1;
	fPrivateData->ClearMessages();

	release_sem_etc(fPrivateData->fMessageWaiters, 1, B_RELEASE_ALL);

	fInitStatus = B_NO_INIT;
}


status_t
BSocketMessenger::SetTo(const BNetworkAddress& address, bigtime_t timeout)
{
	Unset();

	if (fPrivateData == NULL)
		return B_NO_MEMORY;

	fPrivateData->fReplyReader = spawn_thread(&_MessageReader,
		"Message Reader", B_NORMAL_PRIORITY, this);
	if (fPrivateData->fReplyReader < 0)
		return fPrivateData->fReplyReader;
	status_t error = fSocket.Connect(address, timeout);
	if (error != B_OK) {
		Unset();
		return error;
	}

	return fInitStatus = resume_thread(fPrivateData->fReplyReader);
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
	int64 replyID = atomic_add64(&fPrivateData->fReplyIDCounter, 1);
	BMessage temp(message);
	temp.AddInt64(kReplySenderIDField, replyID);
	status_t error = _SendMessage(temp);
	if (error != B_OK)
		return error;

	return _ReadReply(replyID, _reply, timeout);
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
BSocketMessenger::SendReply(const BMessage& message, const BMessage& reply)
{
	int64 replyID;
	if (message.FindInt64(kReplySenderIDField, &replyID) != B_OK)
		return B_NOT_ALLOWED;

	BMessage replyMessage(reply);
	replyMessage.AddInt64(kReplyReceiverIDField, replyID);
	return SendMessage(replyMessage);
}


status_t
BSocketMessenger::ReceiveMessage(BMessage& _message, bigtime_t timeout)
{
	status_t error = B_OK;
	AutoLocker<BMessageQueue> queueLocker(fPrivateData->fReceivedMessages);
	for (;;) {
		if (!fPrivateData->fReceivedMessages.IsEmpty()) {
			BMessage* nextMessage
				= fPrivateData->fReceivedMessages.NextMessage();
			_message = *nextMessage;
			delete nextMessage;
			break;
		}

		queueLocker.Unlock();
		error = _WaitForMessage(timeout);
		if (error != B_OK)
			break;
		if (!fSocket.IsConnected()) {
			error = B_CANCELED;
			break;
		}
		queueLocker.Lock();
	}

	return error;
}


void
BSocketMessenger::_Init()
{
	if (fPrivateData != NULL)
		return;

	BSocketMessenger::Private* data
		= new(std::nothrow) BSocketMessenger::Private;

	if (data == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	data->fMessageWaiters = create_sem(0, "message waiters");
	if (data->fMessageWaiters < 0) {
		fInitStatus = data->fMessageWaiters;
		delete data;
		return;
	}

	fPrivateData = data;
}


status_t
BSocketMessenger::_WaitForMessage(bigtime_t timeout)
{
	for (;;) {
		status_t error = acquire_sem_etc(fPrivateData->fMessageWaiters, 1,
			B_RELATIVE_TIMEOUT, timeout);
		if (error == B_INTERRUPTED) {
			if (timeout != B_INFINITE_TIMEOUT)
				timeout -= system_time();
			continue;
		}
		if (error != B_OK)
			return error;
		break;
	}

	return B_OK;
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
BSocketMessenger::_ReadMessage(BMessage& _message)
{
	status_t error = fSocket.WaitForReadable(B_INFINITE_TIMEOUT);
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


status_t
BSocketMessenger::_ReadReply(const int64 replyID, BMessage& reply,
	bigtime_t timeout)
{
	status_t error = B_OK;
	for (;;) {
		if (fPrivateData->fReceivedReplies.ContainsKey(replyID)) {
			reply = fPrivateData->fReceivedReplies.Remove(replyID);
			break;
		}

		error = _WaitForMessage(timeout);
		if (error != B_OK)
			break;
		if (!fSocket.IsConnected()) {
			error = B_CANCELED;
			break;
		}
	}

	return error;
}


status_t
BSocketMessenger::_MessageReader(void* arg)
{
	BSocketMessenger* messenger = (BSocketMessenger*)arg;
	BSocketMessenger::Private* data = messenger->fPrivateData;
	status_t error = B_OK;

	for (;;) {
		BMessage message;
		error = messenger->_ReadMessage(message);
		if (error != B_OK)
			break;

		int64 replyID;
		if (message.FindInt64(kReplyReceiverIDField, &replyID) == B_OK) {
			error = data->fReceivedReplies.Put(replyID, message);
			if (error != B_OK)
				break;
		} else {
			BMessage* queueMessage = new(std::nothrow) BMessage(message);
			if (queueMessage == NULL) {
				error = B_NO_MEMORY;
				break;
			}

			AutoLocker<BMessageQueue> queueLocker(
				data->fReceivedMessages);
			data->fReceivedMessages.AddMessage(queueMessage);
		}


		release_sem_etc(data->fMessageWaiters, 1, B_RELEASE_ALL);
	}

	// if we exit our message loop, ensure everybody wakes up and knows
	// we're no longer receiving messages.
	messenger->fSocket.Disconnect();
	release_sem_etc(data->fMessageWaiters, 1, B_RELEASE_ALL);
	return error;
}

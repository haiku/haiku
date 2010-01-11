#include "FrameInterface.h"

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "Frame"
#define SUBMODULE_COLOR 31
#include <btDebug.h>

#include <lock.h>

L2capFrame*
SignalByIdent(HciConnection* conn, uint8 ident)
{
	L2capFrame*	frame;

	mutex_lock(&conn->fLockExpected);
	DoublyLinkedList<L2capFrame>::Iterator iterator
		= conn->ExpectedResponses.GetIterator();

	while (iterator.HasNext()) {

		frame = iterator.Next();
		if (frame->type == L2CAP_C_FRAME && frame->ident == ident) {
			mutex_unlock(&frame->conn->fLockExpected);
			return frame;
		}
	}

	mutex_unlock(&conn->fLockExpected);

	return NULL;
}


status_t
TimeoutSignal(L2capFrame* frame, uint32 timeo)
{
	if (frame != NULL)
		return B_OK;

	return B_ERROR;
}


status_t
unTimeoutSignal(L2capFrame* frame)
{
	if (frame != NULL)
		return B_OK;

	return B_ERROR;
}


L2capFrame*
SpawmFrame(HciConnection* conn, L2capChannel* channel, net_buffer* buffer,
	frame_type type)
{
	if (buffer == NULL)
		panic("Null Buffer to outgoing queue");

	L2capFrame* frame = new (std::nothrow) L2capFrame;

	frame->conn = conn;
	frame->channel = channel; // TODO: maybe only scid needed

	frame->buffer = buffer;
	frame->type = type;

	mutex_lock(&conn->fLock);

	conn->OutGoingFrames.Add(frame, true);

	mutex_unlock(&conn->fLock);

	return frame;
}


L2capFrame*
SpawmSignal(HciConnection* conn, L2capChannel* channel, net_buffer* buffer,
	uint8 ident, uint8 code)
{
	if (buffer == NULL)
		panic("Null Buffer to outgoing queue");

	L2capFrame* frame = new (std::nothrow) L2capFrame;

	frame->conn = conn;
	frame->channel = channel; // TODO: not specific descriptor should be required

	frame->buffer = buffer;
	frame->type = L2CAP_C_FRAME;
	frame->ident = ident;
	frame->code = code;

	mutex_lock(&conn->fLock);

	conn->OutGoingFrames.Add(frame, true);

	mutex_unlock(&conn->fLock);

	return frame;
}


status_t
AcknowledgeSignal(L2capFrame* frame)
{

	if (frame != NULL) {

		if (frame->type == L2CAP_C_FRAME) {
			HciConnection* connection = frame->conn;

			unTimeoutSignal(frame);
			mutex_lock(&connection->fLockExpected);
			connection->ExpectedResponses.Remove(frame);
			mutex_unlock(&connection->fLockExpected);
		}

		// NO! This will be deleted by lower layers while being sent!
		// gBufferModule->free(frame->buffer);
		delete frame;

		return B_OK;
	}

	return B_ERROR;
}


status_t
QueueSignal(L2capFrame* frame)
{
	if (frame != NULL) {

		if (frame->type == L2CAP_C_FRAME) {
			mutex_lock(&frame->conn->fLockExpected);
			frame->conn->ExpectedResponses.Add(frame);
			mutex_unlock(&frame->conn->fLockExpected);
			return B_OK;
		}
	}

	return B_ERROR;
}

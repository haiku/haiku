#include "FrameInterface.h"

#define BT_DEBUG_THIS_MODULE
#define SUBMODULE_NAME "Frame"
#define SUBMODULE_COLOR 31
#include <btDebug.h>


L2capFrame*
SignalByIdent(HciConnection* conn, uint8 ident)
{
	L2capFrame*	frame;

	DoublyLinkedList<L2capFrame>::Iterator iterator = conn->ExpectedResponses.GetIterator();

	while (iterator.HasNext()) {

		frame = iterator.Next();
		if (frame->type == L2CAP_C_FRAME && frame->ident == ident) {
			return frame;
		}
	}

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
SpawmFrame(HciConnection* conn, net_buffer* buffer, frame_type type)
{
	if (buffer == NULL)
		panic("Null Buffer to outgoing queue");

	L2capFrame* frame = new (std::nothrow) L2capFrame;

	frame->conn = conn;
	frame->channel = NULL; // TODO: needed?

	frame->buffer = buffer;
	frame->type = type;

	conn->OutGoingFrames.Add(frame, true);

	return frame;
}


L2capFrame*
SpawmSignal(HciConnection* conn, L2capChannel* channel, net_buffer* buffer, uint8 ident, uint8 code)
{
	if (buffer == NULL)
		panic("Null Buffer to outgoing queue");

	L2capFrame* frame = new (std::nothrow) L2capFrame;

	frame->conn = conn;
	frame->channel = channel; // TODO: needed?

	frame->buffer = buffer;
	frame->type = L2CAP_C_FRAME;
	frame->ident = ident;
	frame->code = code;

	conn->OutGoingFrames.Add(frame, true);

	return frame;
}


status_t
AcknowledgeSignal(L2capFrame* frame)
{

	if (frame != NULL) {
		
		if (frame->type == L2CAP_C_FRAME) {
			unTimeoutSignal(frame);
			frame->conn->ExpectedResponses.Remove(frame);
		}
		
		gBufferModule->free(frame->buffer);
		delete frame;
		
		return B_OK;
	}

	return B_ERROR;
}

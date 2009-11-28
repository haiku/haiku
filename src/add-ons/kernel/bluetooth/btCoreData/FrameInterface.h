#ifndef _FRAME_INTERFACE_H
#define _FRAME_INTERFACE_H

#include <net_buffer.h>

#include <btCoreData.h>

extern net_buffer_module_info* gBufferModule;

L2capFrame*
SignalByIdent(HciConnection* conn, uint8 ident);

status_t
TimeoutSignal(L2capFrame* frame, uint32 timeo);

status_t
unTimeoutSignal(L2capFrame* frame);

L2capFrame*
SpawmFrame(HciConnection* conn, L2capChannel* channel, net_buffer* buffer, frame_type frame);

L2capFrame*
SpawmSignal(HciConnection* conn, L2capChannel* channel, net_buffer* buffer, uint8 ident, uint8 code);

status_t
AcknowledgeSignal(L2capFrame* frame);

status_t
QueueSignal(L2capFrame* frame);

#endif

#ifndef UTILS_H_
#define UTILS_H_

#include <Message.h>
#include <OS.h>

void SendMessage(port_id port, BMessage *message, int32 target=-1);

#endif

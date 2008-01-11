#ifndef _DUMP_MESSAGE_H
#define _DUMP_MESSAGE_H

class BMessage;
class BDataIO;

status_t DumpMessageToStream(BMessage *message, BDataIO &stream, int tabCount = 0, BMessage *names = NULL);

#endif /* _DUMP_MESSAGE_H */


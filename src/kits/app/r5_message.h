#ifndef _R5_MESSAGE_H_
#define _R5_MESSAGE_H_

class BMessage;
class BDataIO;

namespace BPrivate {

ssize_t		R5MessageFlattenedSize(const BMessage *message);
status_t	R5MessageFlatten(const BMessage *message, char *buffer, ssize_t size);
status_t	R5MessageFlatten(const BMessage *message, BDataIO *stream, ssize_t *size);
status_t	R5MessageUnflatten(BMessage *message, const char *flatBuffer);
status_t	R5MessageUnflatten(BMessage *message, BDataIO *stream);

}

#endif

#ifndef _PARSE_MESSAGE_H
#define _PARSE_MESSAGE_H

class BMessage;
class BDataIO;

status_t ParseMessageFromStream(BMessage **message, BDataIO &stream);

#endif /* _PARSE_MESSAGE_H */



#include <SupportDefs.h>

class BMessage;
class BDataIO;

namespace BPrivate {
	status_t unflatten_dano_message(uint32 magic, BDataIO& stream, BMessage& message);
	size_t dano_message_size(const char* buffer);
}


#include <File.h>
#include <Message.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


namespace BPrivate {
	status_t unflatten_dano_message(uint32 magic, BDataIO& stream, BMessage& message);
	size_t dano_message_size(const char* buffer);
}


extern const char* __progname;

static const uint32 kMessageFormat = 'FOB2';
static const uint32 kMessageFormatSwapped = '2BOF';


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <flattened dano message>\n", __progname);
		return -1;
	}

	for (int32 i = 1; i < argc; i++) {
		BFile file(argv[i], B_READ_ONLY);
		if (file.InitCheck() != B_OK) {
			fprintf(stderr, "Could not open message \"%s\": %s\n", argv[i], strerror(file.InitCheck()));
			continue;
		}

		off_t size;
		if (file.GetSize(&size) != B_OK)
			continue;

		uint32 magic;
		if (file.Read(&magic, sizeof(uint32)) != sizeof(uint32))
			continue;

		if (magic != kMessageFormat && magic != kMessageFormatSwapped) {
			fprintf(stderr, "Not a dano message \"%s\"\n", argv[i]);
			continue;
		}

		BMessage message;
		status_t status = BPrivate::unflatten_dano_message(magic, file, message);
		if (status == B_OK)
			message.PrintToStream();
		else
			fprintf(stderr, "Could not unflatten message: %s\n", strerror(status));
	}

	return 0;
}


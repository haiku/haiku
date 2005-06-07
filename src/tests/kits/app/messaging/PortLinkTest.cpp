#include <PortLink.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int
main()
{
	port_id port = create_port(100, "portlink");

	BPortLink sender(port, -1);
	BPortLink receiver(-1, port);

	sender.StartMessage('tst1');
	sender.Attach<int32>(42);

	sender.StartMessage('tst2');
	sender.AttachString(NULL);
	sender.AttachString("");
	sender.AttachString("Gurkensalat");

	status_t status = sender.Flush();
	if (status != B_OK) {
		fprintf(stderr, "flushing messages failed: %ld, %s!\n",
			status, strerror(status));
		return -1;
	}

	int32 code;
	if (receiver.GetNextReply(&code) != B_OK) {
		fprintf(stderr, "get message failed!\n");
		return -1;
	}
	if (code != 'tst1')
		fprintf(stderr, "code is wrong (%ld)!\n", code);

	int32 value;
	if (receiver.Read<int32>(&value) != B_OK) {
		fprintf(stderr, "reading message failed!\n");
		return -1;
	}

	if (value != 42)
		fprintf(stderr, "value is wrong: %ld!\n", value);
	
	if (receiver.GetNextReply(&code) != B_OK) {
		fprintf(stderr, "get message failed!\n");
		return -1;
	}
	if (code != 'tst2')
		fprintf(stderr, "code is wrong (%ld)!\n", code);

	for (int32 i = 0; i < 4; i++) {
		char *string;
		if (receiver.ReadString(&string) != B_OK) {
			if (i == 3)
				continue;

			fprintf(stderr, "reading string failed!\n");
			return -1;
		} else if (i == 3) {
			fprintf(stderr, "reading string succeeded, but shouldn't!\n");
			return -1;
		}
		free(string);
	}

	status = receiver.GetNextReply(&code, 0);
	if (status != B_WOULD_BLOCK)
		fprintf(stderr, "reading would not block!\n");

	return 0;
}

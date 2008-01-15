#include <PortLink.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


const int32 kBufferSize = 2048;


void
get_next_message(BPrivate::PortLink &link, int32 expectedCode)
{
	int32 code;
	if (link.GetNextMessage(code) != B_OK) {
		fprintf(stderr, "get message failed!\n");
		exit(-1);
	}
	if (code != expectedCode) {
		fprintf(stderr, "code is wrong (%ld)!\n", code);
		exit(-1);
	}
}


int
main()
{
	port_id port = create_port(100, "portlink");

	BPrivate::PortLink sender(port, -1);
	BPrivate::PortLink receiver(-1, port);

	sender.StartMessage('tst1');
	sender.Attach<int32>(42);

	sender.StartMessage('tst2');
	sender.AttachString(NULL);
	sender.AttachString("");
	sender.AttachString("Gurkensalat");

	sender.StartMessage('tst3', 100000);
	sender.Attach(&port, 100000);
	if (sender.EndMessage() == B_OK) {
		fprintf(stderr, "attaching huge message succeded (it shouldn't)!\n");
		return -1;
	}

	// force overlap
	char test[kBufferSize + 2048];
	sender.StartMessage('tst4');
	sender.Attach(test, kBufferSize - 40);

	// force buffer grow
	sender.StartMessage('tst5');
	sender.Attach(test, sizeof(test));

	status_t status = sender.Flush();
	if (status != B_OK) {
		fprintf(stderr, "flushing messages failed: %ld, %s!\n",
			status, strerror(status));
		return -1;
	}

	get_next_message(receiver, 'tst1');

	int32 value;
	if (receiver.Read<int32>(&value) != B_OK) {
		fprintf(stderr, "reading message failed!\n");
		return -1;
	}

	if (value != 42) {
		fprintf(stderr, "value is wrong: %ld!\n", value);
		return -1;
	}

	get_next_message(receiver, 'tst2');

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

	get_next_message(receiver, 'tst4');
	get_next_message(receiver, 'tst5');

	int32 code;
	status = receiver.GetNextMessage(code, 0);
	if (status != B_WOULD_BLOCK) {
		fprintf(stderr, "reading would not block!\n");
		return -1;
	}

	puts("All OK!");
	return 0;
}

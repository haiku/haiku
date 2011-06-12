#include <SerialPort.h>

#include <stdio.h>
#include <string.h>


static int32
reader_thread(void *data)
{
	BSerialPort *port = (BSerialPort *)data;
	char buffer[128];

	while (true) {
		ssize_t read = port->Read(buffer, sizeof(buffer));
		if (read <= 0)
			continue;

		for (ssize_t i = 0; i < read; i++) {
			putc(buffer[i], stdout);
			fflush(stdout);
		}
	}

	return 0;
}


int
main(int argc, char *argv[])
{
	BSerialPort port;

	if (argc < 2) {
		printf("usage: %s <port>\n", argv[0]);

		int32 portCount = port.CountDevices();
		printf("\tports (%ld):\n", portCount);

		char nameBuffer[B_OS_NAME_LENGTH];
		for (int32 i = 0; i < portCount; i++) {
			if (port.GetDeviceName(i, nameBuffer, sizeof(nameBuffer)) != B_OK) {
				printf("\t\tfailed to retrieve name %ld\n", i);
				continue;
			}

			printf("\t\t%s\n", nameBuffer);
		}

		return 1;
	}

	status_t result = port.Open(argv[1]);
	if (result < B_OK) {
		printf("failed to open port \"%s\": %s\n", argv[1], strerror(result));
		return result;
	}

	port.SetDataRate(B_9600_BPS);
	port.SetDataBits(B_DATA_BITS_8);
	port.SetStopBits(B_STOP_BITS_1);
	port.SetParityMode(B_NO_PARITY);
	port.SetFlowControl(B_NOFLOW_CONTROL);

	thread_id reader = spawn_thread(reader_thread, "serial reader",
		B_NORMAL_PRIORITY, &port);
	if (reader < 0) {
		printf("failed to spawn reader thread\n");
		return reader;
	}

	resume_thread(reader);

	char buffer[128];
	while (true) {
		char *string = fgets(buffer, sizeof(buffer), stdin);
		if (string == NULL)
			continue;

		port.Write(buffer, strlen(string) - 1);
	}

	return 0;
}

// SMRemoteTargetApp.cpp

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "SMRemoteTargetApp.h"
#include "SMLooper.h"

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

int
main(int argc, char **argv)
{
DBG(OUT("REMOTE: main()\n"));
	assert(argc > 1);
	const char *portName = argv[1];
	bool preferred = (argc > 2 && !strcmp(argv[2], "preferred"));
	// create local port
	port_id localPort = create_port(5, "SMRemoteTargetApp port");
	assert(localPort >= 0);
DBG(OUT("REMOTE: local port created\n"));
	// create looper/handler
	SMHandler *handler = NULL;
	SMLooper *looper = new SMLooper;
	looper->Run();
	if (!preferred) {
		handler = new SMHandler;
		assert(looper->Lock());
		looper->AddHandler(handler);
		looper->Unlock();
	}
DBG(OUT("REMOTE: looper/handler created\n"));
	// find remote port
DBG(OUT("REMOTE: find remote port `%s'\n", portName));
	port_id remotePort = find_port(portName);
	assert(remotePort >= 0);
DBG(OUT("REMOTE: found remote port\n"));
	// send init message
	smrt_init initData;
	initData.port = localPort;
	initData.messenger = BMessenger(handler, looper);
	assert(write_port(remotePort, SMRT_INIT, &initData, sizeof(smrt_init))
		   == B_OK);
DBG(OUT("REMOTE: init message sent\n"));
	// loop until quit message
	bool running = true;
	while (running) {
		char buffer[sizeof(smrt_get_ready)];
		int32 code;
		ssize_t readSize = read_port(localPort, &code, buffer, sizeof(buffer));
		if (readSize < 0)
			running = readSize;
DBG(OUT("REMOTE: read port: %.4s\n", (char*)&code));
		switch (code) {
			case SMRT_GET_READY:
			{
DBG(OUT("REMOTE: SMRT_GET_READY\n"));
				assert(readSize == sizeof(smrt_get_ready));
				smrt_get_ready &data = *(smrt_get_ready*)buffer;
				looper->SetReplyDelay(data.reply_delay);
				looper->BlockUntil(data.unblock_time);
DBG(OUT("REMOTE: SMRT_GET_READY done\n"));
				break;
			}
			case SMRT_DELIVERY_SUCCESS_REQUEST:
			{
DBG(OUT("REMOTE: SMRT_DELIVERY_SUCCESS_REQUEST\n"));
				assert(readSize == 0);
				smrt_delivery_success data;
				data.success = looper->DeliverySuccess();
				assert(write_port(remotePort, SMRT_DELIVERY_SUCCESS_REPLY,
								  &data, sizeof(smrt_delivery_success))
					   == B_OK);
DBG(OUT("REMOTE: SMRT_DELIVERY_SUCCESS_REQUEST done\n"));
				break;
			}
			case SMRT_QUIT:
DBG(OUT("REMOTE: QUIT\n"));
			default:
if (code != SMRT_QUIT)
DBG(OUT("REMOTE: UNKNOWN COMMAND!\n"));
				running = false;
				break;
		}
	}
	// delete looper/handler
	if (looper) {
		looper->Lock();
		if (handler) {
			looper->RemoveHandler(handler);
			delete handler;
		}
		looper->Quit();
	}
DBG(OUT("REMOTE: looper/handler deleted\n"));
	// delete local port
	delete_port(localPort);
DBG(OUT("REMOTE: main() done\n"));
	return 0;
}



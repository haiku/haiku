#include <stdio.h>
#include <stdlib.h>
#include <OS.h>
#include <Message.h>

int main(int count, char **args) {

	sem_id portSem = atoi(args[2]);
	port_id port = atoi(args[1]);

	for (int i=0; i<10; i++) {

	acquire_sem(portSem);

	ssize_t size = port_buffer_size(port);
	printf("size : %ld\n", size);

	char buffer[size];
	int32 code;
	ssize_t newsize;
	if ((newsize = read_port(port, &code, buffer, size))==size) {
		BMessage msg;
		if (msg.Unflatten(buffer)!=B_OK) {
			printf("error \n");
		} else {
			msg.PrintToStream();
		}
	} else {
		printf("error %ld\n", newsize);
	}

	}
}

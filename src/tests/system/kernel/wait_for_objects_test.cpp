#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <OS.h>


static sem_id sSemaphore1;
static sem_id sSemaphore2;
static port_id sPort1;
static port_id sPort2;


static status_t
notifier_thread(void* data)
{
	snooze(1000000);
	release_sem(sSemaphore1);
	snooze(1000000);
	release_sem(sSemaphore2);
	snooze(1000000);
	write_port(sPort1, 0xcafe, "test", 4);
	snooze(1000000);
	int32 code;
	read_port(sPort2, &code, &code, sizeof(code));
	snooze(1000000);

	return B_OK;
}


static void
print_events(const object_wait_info* infos, int infoCount)
{
	printf("events:\n");
	for (int i = 0; i < infoCount; i++)
		printf("  %d: 0x%x\n", i, infos[i].events);
}


int
main()
{
	sSemaphore1 = create_sem(0L, "test semaphore 1");
	sSemaphore2 = create_sem(0L, "test semaphore 2");
	sPort1 = create_port(2L, "test port 1");
	sPort2 = create_port(1L, "test port 2");

	printf("semaphore 1: %ld\n", sSemaphore1);
	printf("semaphore 2: %ld\n", sSemaphore2);
	printf("port 1:      %ld\n", sPort1);
	printf("port 2:      %ld\n", sPort2);

	thread_id thread = spawn_thread(notifier_thread, "notifier",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(thread);

	printf("thread:      %ld\n", thread);

	object_wait_info initialInfos[] = {
		{
			sSemaphore1,
			B_OBJECT_TYPE_SEMAPHORE,
			B_EVENT_ACQUIRE_SEMAPHORE
		},
		{
			sSemaphore2,
			B_OBJECT_TYPE_SEMAPHORE,
			B_EVENT_ACQUIRE_SEMAPHORE
		},
		{
			sPort1,
			B_OBJECT_TYPE_PORT,
			B_EVENT_READ
		},
		{
			sPort2,
			B_OBJECT_TYPE_PORT,
			B_EVENT_WRITE
		},
		{
			0,
			B_OBJECT_TYPE_FD,
			B_EVENT_READ
		},
		{
			thread,
			B_OBJECT_TYPE_THREAD,
			B_EVENT_INVALID
		}
	};
	int infoCount = sizeof(initialInfos) / sizeof(object_wait_info);

	while (true) {
		object_wait_info infos[infoCount];
		memcpy(infos, initialInfos, sizeof(initialInfos));

		ssize_t result = wait_for_objects_etc(infos, infoCount,
			B_RELATIVE_TIMEOUT, 10000000LL);

		if (result >= 0)
			printf("\nwait_for_objects(): %ld\n", result);
		else
			printf("\nwait_for_objects(): %s\n", strerror(result));

		print_events(infos, infoCount);

		for (int i = 0; i < infoCount; i++) {
			int32 type = infos[i].type;
			int32 object = infos[i].object;
			uint16 events = infos[i].events;
			char buffer[256];

			if (type == B_OBJECT_TYPE_SEMAPHORE) {
				if (events & B_EVENT_ACQUIRE_SEMAPHORE) {
					status_t error = acquire_sem_etc(object, 1,
						B_RELATIVE_TIMEOUT, 0);
					printf("acquire_sem(%ld): %s\n", object, strerror(error));
				}
			} else if (type == B_OBJECT_TYPE_PORT) {
				if (events & B_EVENT_READ) {
					int32 code;
					ssize_t bytesRead = read_port_etc(object, &code,
						buffer, sizeof(buffer), B_RELATIVE_TIMEOUT, 0);
					printf("read_port(%ld): %ld\n", object, bytesRead);
				}
				if (events & B_EVENT_WRITE) {
					status_t error = write_port_etc(object, 0xc0de, buffer, 10,
						B_RELATIVE_TIMEOUT, 0);
					printf("write_port(%ld): %s\n", object, strerror(error));
				}
			} else if (type == B_OBJECT_TYPE_FD) {
				if (events & B_EVENT_READ) {
					ssize_t bytesRead = read(object, buffer, sizeof(buffer));
					printf("read(%ld): %ld\n", object, bytesRead);
				}
			} else if (type == B_OBJECT_TYPE_THREAD) {
				if (events & B_EVENT_INVALID) {
					printf("thread %ld quit\n", object);
					infoCount--;
				}
			}
		}
	}

	return 0;
}

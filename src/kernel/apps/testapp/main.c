/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syscalls.h>
#include <ktypes.h>
#include <resource.h>
#include <Errors.h>
#include <OS.h>

static void port_test(void);
static int port_test_thread_func(void* arg);

static int test_thread(void *args)
{
	int i = (int)args;

	sys_snooze(1000000);
	for(;;) {
		printf("%c", 'a' + i);
	}
	return 0;
}

static int dummy_thread(void *args)
{
	return 1;
}

static int cpu_eater_thread(void *args)
{
	for(;;)
		;
}

static int fpu_cruncher_thread(void *args)
{
	double y = *(double *)args;
	double z;

	for(;;) {
		z = y * 1.47;
		y = z / 1.47;
		if(y != *(double *)args)
			printf("error: y %f\n", y);
	}
}

int main(int argc, char **argv)
{
	int rc = 0;
	int cnt;

	printf("testapp\n");

	for(cnt = 0; cnt< argc; cnt++) {
		printf("arg %d = %s \n",cnt,argv[cnt]);
	}

	printf("my thread id is %d\n", find_thread(NULL));
#if 0
	{
		char c;
		printf("enter something: ");

		for(;;) {
			c = getchar();
			printf("%c", c);
		}
	}
#endif
#if 0
	for(;;) {
		sys_snooze(100000);
		printf("booyah!");
	}

	for(;;);
#endif
#if 0
	printf("waiting 5 seconds\n");
	sys_snooze(5000000);
#endif
#if 0
	fd = sys_open("/dev/net/rtl8139/0", "", STREAM_TYPE_DEVICE);
	if(fd >= 0) {
		for(;;) {
			size_t len;
			static char buf[] = {
				0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
				0x00, 0x08, 0x00, 0x45, 0x00, 0x00, 0x28, 0xF9, 0x55, 0x40, 0x00,
				0x40, 0x06, 0xC0, 0x02, 0xC0, 0xA8, 0x00, 0x01, 0xC0, 0xA8, 0x00,
				0x26, 0x00, 0x50, 0x0B, 0x5C, 0x81, 0xD6, 0xFA, 0x48, 0xBB, 0x17,
				0x03, 0xC9, 0x50, 0x10, 0x7B, 0xB0, 0x6C, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};
			len = sizeof(buf);
			sys_write(fd, buf, 0, &len);
		}
	}
#endif
#if 0
	fd = sys_open("/dev/net/rtl8139/0", "", STREAM_TYPE_DEVICE);
	if(fd >= 0) {
		int foo = 0;
		for(;;) {
			size_t len;
			char buf[1500];

			len = sizeof(buf);
			sys_read(fd, buf, 0, &len);
			printf("%d read %d bytes\n", foo++, len);
		}
	}
#endif

// XXX dangerous! This overwrites the beginning of your hard drive
#if 0
	{
		char buf[512];
		size_t bytes_read;

		printf("opening /dev/bus/ide/0/0/raw\n");
		fd = sys_open("/dev/bus/ide/0/0/raw", "", STREAM_TYPE_DEVICE);
		printf("fd = %d\n", fd);

		bytes_read = 512;
		rc = sys_read(fd, buf, 0, &bytes_read);
		printf("rc = %d, bytes_read = %d\n", rc, bytes_read);

		buf[0] = 'f';
		buf[1] = 'o';
		buf[2] = 'o';
		buf[3] = '2';
		buf[4] = 0;

		bytes_read = 512;
		rc = sys_write(fd, buf, 1024, &bytes_read);
		printf("rc = %d, bytes_read = %d\n", rc, bytes_read);

		sys_close(fd);
	}
#endif
#if 0
	{
		thread_id tids[10];
		int i;

		for(i=0; i<10; i++) {
			tids[i] = spawn_thread(&test_thread, "foo", THREAD_MEDIUM_PRIORITY, (void *)i);
			resume_thread(tids[i]);
		}

		sys_snooze(5000000);
		sys_proc_kill_proc(sys_get_current_proc_id());
/*
		sys_snooze(3000000);
		for(i=0; i<10; i++) {
			kill_thread(tids[i]);
		}
		printf("thread_is dead\n");
		sys_snooze(5000000);
*/
	}
#endif
#if 0
	{
		for(;;)
			sys_proc_create_proc("/boot/bin/true", "true", 32);
	}
#endif
#if 0
	{
		void *buf = (void *)0x60000000;
		int fd;
		int rc;
		int len = 512;

		fd = sys_open("/boot/testapp", "", STREAM_TYPE_FILE);

		rc = sys_read(fd, buf, 0, &len);
		printf("rc from read = 0x%x\n", rc);
		sys_close(fd);
	}
#endif
#if 0
	{
		char data;
		int fd;

		fd = sys_open("/dev/audio/pcbeep/1", STREAM_TYPE_DEVICE, 0);
		if(fd >= 0) {
			printf("writing to the speaker\n");
			data = 3;
			sys_write(fd, &data, 0, 1);
			sys_snooze(1000000);
			data = 0;
			sys_write(fd, &data, 0, 1);
			sys_close(fd);
		}
	}
#endif

#if 1
	{
		port_test();
	}
#endif
#if 0
	{
		int fd, bytes_read;
		char buf[3];

		fd = sys_open("/dev/ps2mouse", STREAM_TYPE_DEVICE, 0);
		if(fd < 0) {
			printf("failed to open device\n");
			return -1;
		}

		bytes_read = sys_read(fd, buf, -1, 3);
		if(bytes_read < 3) {
			printf("failed to read device\n");
			return -1;
		}

		printf("Status: %X\nDelta X: %d\nDelta Y: %d\n", buf[0], buf[1], buf[2]);
	}
#endif
#if 0
	{
		int fd;
		int buf[512/4];
		int i, j;

		fd = sys_open("/dev/disk/netblock/0/raw", STREAM_TYPE_DEVICE, 0);
		if(fd < 0) {
			printf("could not open netblock\n");
			return -1;
		}

		sys_ioctl(fd, 90001, NULL, 0);

		for(i=0; i<1*1024*1024; i += 512) {
			for(j=0; j<512/4; j++)
				buf[j] = i + j*4;
			sys_write(fd, buf, -1, sizeof(buf));
		}

//		sys_read(fd, buf, 0, sizeof(buf));
//		sys_write(fd, buf, 512, sizeof(buf));
	}
#endif
#if 0

#define NUM_FDS 150
	{
		int				fds[NUM_FDS], i;
		struct rlimit	rl;

		rc = getrlimit(RLIMIT_NOFILE, &rl);
		if (rc < 0) {
			printf("error in getrlimit\n");
			return -1;
		}

		printf("RLIMIT_NOFILE = %lu\n", rl.rlim_cur);

		for(i = 0; i < NUM_FDS; i++) {
			fds[i] = open("/boot/bin/testapp", 0);
			if (fds[i] < 0) {
				break;
			}
		}

		printf("opened %d files\n", i);

		rl.rlim_cur += 15;

		printf("Setting RLIMIT_NOFILE to %lu\n", rl.rlim_cur);
		rc = setrlimit(RLIMIT_NOFILE, &rl);
		if (rc < 0) {
			printf("error in setrlimit\n");
			return -1;
		}

		rl.rlim_cur = 0;
		rc = getrlimit(RLIMIT_NOFILE, &rl);
		if (rc < 0) {
			printf("error in getrlimit\n");
			return -1;
		}

		printf("RLIMIT_NOFILE = %lu\n", rl.rlim_cur);

		for(;i < NUM_FDS; i++) {
			fds[i] = open("/boot/bin/testapp", 0);
			if (fds[i] < 0) {
				break;
			}
		}

		printf("opened a total of %d files\n", i);
	}
#endif
#if 0
	{
		region_id rid;
		void *ptr;

		rid = sys_vm_map_file("netblock", &ptr, REGION_ADDR_ANY_ADDRESS, 16*1024, LOCK_RW,
			REGION_NO_PRIVATE_MAP, "/dev/disk/netblock/0/raw", 0);
		if(rid < 0) {
			printf("error mmaping device\n");
			return -1;
		}

		// play around with it
		printf("mmaped device at %p\n", ptr);
		printf("%d\n", *(int *)ptr);
		printf("%d\n", *((int *)ptr + 1));
		printf("%d\n", *((int *)ptr + 2));
		printf("%d\n", *((int *)ptr + 3));
	}
#endif
#if 0
	{
		int i;

//		printf("spawning %d copies of true\n", 10000);

		for(i=0; ; i++) {
			proc_id id;
			bigtime_t t;

			printf("%d...", i);

			t = sys_system_time();

			id = sys_proc_create_proc("/boot/bin/true", "true", NULL, 0, 20);
			if(id <= 0x2) {
				printf("new proc returned 0x%x!\n", id);
				return -1;
			}
			sys_proc_wait_on_proc(id, NULL);

			printf("done (%Ld usecs)\n", sys_system_time() - t);
		}
	}
#endif
#if 0
	{
		int i;

		printf("spawning two cpu eaters\n");

//		resume_thread(spawn_thread("cpu eater 1", &cpu_eater_thread, 0));
//		resume_thread(spawn_thread(&cpu_eater_thread, "cpu eater 2", &cpu_eater_thread, 0));

		printf("spawning %d threads\n", 10000);

		for(i=0; i<10000; i++) {
			thread_id id;
			bigtime_t t;

			printf("%d...", i);

			t = sys_system_time();

			id = spawn_thread(&dummy_thread, "testthread", THREAD_MEDIUM_PRIORITY, NULL);
			if (id > 0)
				resume_thread(id);

			sys_thread_wait_on_thread(id, NULL);

			printf("done (%Ld usecs)\n", sys_system_time() - t);
		}
	}
#endif
#if 1
	{
		thread_id id;
		static double f[5] = { 2.43, 5.23, 342.34, 234123.2, 1.4 };

		printf("spawning a few floating point crunchers\n");

		id = spawn_thread(&fpu_cruncher_thread, "fpu thread0", THREAD_MEDIUM_PRIORITY, &f[0]);
		resume_thread(id);

		id = spawn_thread(&fpu_cruncher_thread, "fpu thread1", THREAD_MEDIUM_PRIORITY, &f[1]);
		resume_thread(id);

		id = spawn_thread(&fpu_cruncher_thread, "fpu thread2", THREAD_MEDIUM_PRIORITY, &f[2]);
		resume_thread(id);

		id = spawn_thread(&fpu_cruncher_thread, "fpu thread3", THREAD_MEDIUM_PRIORITY, &f[3]);
		resume_thread(id);

		id = spawn_thread(&fpu_cruncher_thread, "fpu thread4", THREAD_MEDIUM_PRIORITY, &f[4]);
		resume_thread(id);

		getchar();
		printf("passed the test\n");
	}
#endif

	printf("exiting w/return code %d\n", rc);
	return rc;
}

/*
 * testcode ports
 */

port_id test_p1, test_p2, test_p3, test_p4;

static void port_test(void)
{
	char testdata[5];
	thread_id t;
	int res;
	int32 dummy;
	int32 dummy2;

	strcpy(testdata, "abcd");

	printf("porttest: port_create()\n");
	test_p1 = sys_port_create(1,    "test port #1");
	test_p2 = sys_port_create(10,   "test port #2");
	test_p3 = sys_port_create(1024, "test port #3");
	test_p4 = sys_port_create(1024, "test port #4");

	printf("porttest: port_find()\n");
	printf("'test port #1' has id %d (should be %d)\n", sys_port_find("test port #1"), test_p1);

	printf("porttest: port_write() on 1, 2 and 3\n");
	sys_port_write(test_p1, 1, &testdata, sizeof(testdata));
	sys_port_write(test_p2, 666, &testdata, sizeof(testdata));
	sys_port_write(test_p3, 999, &testdata, sizeof(testdata));
	printf("porttest: port_count(test_p1) = %d\n", sys_port_count(test_p1));

	printf("porttest: port_write() on 1 with timeout of 1 sec (blocks 1 sec)\n");
	sys_port_write_etc(test_p1, 1, &testdata, sizeof(testdata), B_TIMEOUT, 1000000);
	printf("porttest: port_write() on 2 with timeout of 1 sec (wont block)\n");
	res = sys_port_write_etc(test_p2, 777, &testdata, sizeof(testdata), B_TIMEOUT, 1000000);
	printf("porttest: res=%d, %s\n", res, res == 0 ? "ok" : "BAD");

	printf("porttest: port_read() on empty port 4 with timeout of 1 sec (blocks 1 sec)\n");
	res = sys_port_read_etc(test_p4, &dummy, &dummy2, sizeof(dummy2), B_TIMEOUT, 1000000);
	printf("porttest: res=%d, %s\n", res, res == ETIMEDOUT ? "ok" : "BAD");

	printf("porttest: spawning thread for port 1\n");
	t = spawn_thread(port_test_thread_func, "port_test", THREAD_MEDIUM_PRIORITY, NULL);
	// resume thread
	resume_thread(t);

	printf("porttest: write\n");
	sys_port_write(test_p1, 1, &testdata, sizeof(testdata));

	// now we can write more (no blocking)
	printf("porttest: write #2\n");
	sys_port_write(test_p1, 2, &testdata, sizeof(testdata));
	printf("porttest: write #3\n");
	sys_port_write(test_p1, 3, &testdata, sizeof(testdata));

	printf("porttest: waiting on spawned thread\n");
	sys_thread_wait_on_thread(t, NULL);

	printf("porttest: close p1\n");
	sys_port_close(test_p2);
	printf("porttest: attempt write p1 after close\n");
	res = sys_port_write(test_p2, 4, &testdata, sizeof(testdata));
	printf("porttest: port_write ret %s (should give Bad port id)\n", strerror(res));

	printf("porttest: testing delete p2\n");
	sys_port_delete(test_p2);

	printf("porttest: end test main thread\n");
}

static int port_test_thread_func(void* arg)
{
	int msg_code;
	int n;
	char buf[5];

	printf("porttest: port_test_thread_func()\n");

	n = sys_port_read(test_p1, &msg_code, &buf, 3);
	printf("port_read #1 code %d len %d buf %3s\n", msg_code, n, buf);
	n = sys_port_read(test_p1, &msg_code, &buf, 4);
	printf("port_read #1 code %d len %d buf %4s\n", msg_code, n, buf);
	buf[4] = 'X';
	n = sys_port_read(test_p1, &msg_code, &buf, 5);
	printf("port_read #1 code %d len %d buf %5s\n", msg_code, n, buf);

	printf("porttest: testing delete p1 from other thread\n");
	sys_port_delete(test_p1);
	printf("porttest: end port_test_thread_func()\n");

	return 0;
}


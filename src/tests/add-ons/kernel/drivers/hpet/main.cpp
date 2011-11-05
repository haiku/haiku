#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <OS.h>

#include <hpet_interface.h>

int main()
{
	int hpetFD = open("/dev/misc/hpet", O_RDWR);
	if (hpetFD < 0) {
		printf("Cannot open HPET driver: %s\n", strerror(errno));
		return -1;	
	}

	uint64 value, newValue;
	read(hpetFD, &value, sizeof(uint64));

	snooze(1000000);

	read(hpetFD, &newValue, sizeof(uint64));
	printf("HPET counter value difference (1 sec): %lld\n", newValue - value);

	status_t status;
	bigtime_t timeValue = 2000000;
	printf("Waiting 2 seconds...\n");
	status = ioctl(hpetFD, HPET_WAIT_TIMER, &timeValue, sizeof(timeValue));
	printf("%s.\n", strerror(status));

	timeValue = 5000000;
	printf("Waiting 5 seconds...\n");
	status = ioctl(hpetFD, HPET_WAIT_TIMER, &timeValue, sizeof(timeValue));
	printf("%s.\n", strerror(status));

	timeValue = 1000000;
	printf("Waiting 1 second...\n");
	status = ioctl(hpetFD, HPET_WAIT_TIMER, &timeValue, sizeof(timeValue));
	printf("%s.\n", strerror(status));

	close(hpetFD);

	return 0;
}


#include <stdio.h>
#include <string.h>
#include <SupportDefs.h>

struct uhci_td_control_status {
	uint32	actual_length : 11;
	uint32	reserved_1 : 6;
	uint32	bitstuff_error : 1;
	uint32	crc_timeout : 1;
	uint32	nak_received : 1;
	uint32	babble_detected : 1;
	uint32	data_buffer_error : 1;
	uint32	stalled : 1;
	uint32	active : 1;
	uint32	interrupt_on_complete : 1;
	uint32	isochronous_select : 1;
	uint32	low_speed_device : 1;
	uint32	error_counter : 2;
	uint32	short_packet_detect : 1;
	uint32	reserved_2 : 2;
};


int
main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;

	uint32 value;
	if (sscanf(argv[1], "%lx", &value) != 1)
		return 2;

	uhci_td_control_status status;
	memcpy(&status, &value, sizeof(status));

	printf("value: 0x%08lx\n", value);
	printf("actual_length: %ld\n", status.actual_length);
	printf("reserved_1: %ld\n", status.reserved_1);
	printf("bitstuff_error: %ld\n", status.bitstuff_error);
	printf("crc_timeout: %ld\n", status.crc_timeout);
	printf("nak_received: %ld\n", status.nak_received);
	printf("babble_detected: %ld\n", status.babble_detected);
	printf("data_buffer_error: %ld\n", status.data_buffer_error);
	printf("stalled: %ld\n", status.stalled);
	printf("active: %ld\n", status.active);
	printf("interrupt_on_complete: %ld\n", status.interrupt_on_complete);
	printf("isochronous_select: %ld\n", status.isochronous_select);
	printf("low_speed_device: %ld\n", status.low_speed_device);
	printf("error_counter: %ld\n", status.error_counter);
	printf("short_packet_detect: %ld\n", status.short_packet_detect);
	printf("reserved_2: %ld\n", status.reserved_2);

	return 0;
}

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MIDI_DRIVER_H
#define _MIDI_DRIVER_H

#include <Drivers.h>
#include <module.h>

/* -----
	ioctl codes
----- */

/* the old opcodes are deprecated, and may or may not work 
   in newer drivers */
enum {
	B_MIDI_GET_READ_TIMEOUT = B_MIDI_DRIVER_BASE,
	B_MIDI_SET_READ_TIMEOUT,
	B_MIDI_TIMED_READ,
	B_MIDI_TIMED_WRITE,
	B_MIDI_WRITE_SYNC,
	B_MIDI_WRITE_CLEAR,
	B_MIDI_GET_READ_TIMEOUT_OLD	= B_DEVICE_OP_CODES_END + 1,
	B_MIDI_SET_READ_TIMEOUT_OLD
};

/* the default timeout when you open a midi driver */
#define B_MIDI_DEFAULT_TIMEOUT 1000000000000000LL

/*	Usage:
	To read, set "data" to a pointer to your buffer, and "size" to the 
	maximum size of this buffer. On return, "when" will contain the time 
	at which the data was received (first byte) and "size" will contain 
	the actual amount of data read.
	Call ioctl(fd, B_MIDI_TIMED_READ, &midi_timed_data, sizeof(midi_timed_data));
	To write, set "when" to when you want the first byte to go out the 
	wire, set "data" to point to your data, and set "size" to the size 
	of your data.
	Call ioctl(fd, B_MIDI_TIMED_WRITE, &midi_timed_data, sizeof(midi_timed_data));
*/
typedef struct {
	bigtime_t when;
	size_t size;
	unsigned char * data;
} midi_timed_data;


/* The MIDI parser returns the number of bytes that a message contains, given the */
/* initial byte. For some messages, this is not known until the second byte is seen. */
/* For such messages, a state > 0 is returned as well as some count > 0. When state */
/* is > 0, you should call (*parse) for the next byte as well, which might modify */
/* the returned message size. Message size will always be returned with the current */
/* byte being counted as byte 1. A return of 0 means that the byte initiates a new */
/* message. SysX is handled by returning max_size until the end or next initial message */
/* is seen. So your loop looks something like: */
/* 
  uint32 state = 0;	// Only set this to 0 the first time you call the parser.
                  // preserve the 'state' between invocations of your read() hook.
  int todo = 0;
  unsigned char * end = in_buf+buf_size
  unsigned char * out_buf = in_buf;
  while (true) {
    uchar byte = read_midi();
    if (!todo || state) {
      todo = (*parser->parse)(&state, byte, end-out_buf);
    }
    if (todo < 1) {
      unput_midi(byte);
    } else {
      *(out_buf++) = byte;
      todo--;
    }
    if (todo < 1 || out_buf >= end) {
      received_midi_message(in_buf, out_buf-in_buf);
      todo = 0;
    }
  }
   */

#define B_MIDI_PARSER_MODULE_NAME "media/midiparser/v1"

typedef struct _midi_parser_module_info {
	module_info	minfo;
	int		(*parse)(uint32 * state, uchar byte, size_t max_size);
	int		_reserved_;
} midi_parser_module_info;

#define B_MPU_401_MODULE_NAME "generic/mpu401/v1"

enum {
	B_MPU_401_ENABLE_CARD_INT = 1,
	B_MPU_401_DISABLE_CARD_INT
};
typedef struct _generic_mpu401_module {
	module_info minfo;
	status_t (*create_device)(int port, void ** out_storage, uint32 workarounds, void (*interrupt_op)(int32 op, void * card), void * card);
	status_t (*delete_device)(void * storage);
	status_t (*open_hook)(void * storage, uint32 flags, void ** out_cookie);
	status_t (*close_hook)(void * cookie);
	status_t (*free_hook)(void * cookie);
	status_t (*control_hook)(void * cookie, uint32 op, void * data, size_t len);
	status_t (*read_hook)(void * cookie, off_t pos, void * data, size_t * len);
	status_t (*write_hook)(void * cookie, off_t pos, const void * data, size_t * len);
	bool (*interrupt_hook)(void * cookie);
	int	_reserved_;
} generic_mpu401_module;

#endif

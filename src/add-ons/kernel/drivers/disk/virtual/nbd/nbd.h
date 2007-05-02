/*
 * Network Block Device protocol
 * Copyright 2006-2007, François Revol. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * references:
 * include/linux/nbd.h
 */

enum {
	NBD_CMD_READ = 0,
	NBD_CMD_WRITE,
	NBD_CMD_DISC
};

#define NBD_REQUEST_MAGIC 0x25609513
#define NBD_REPLY_MAGIC 0x67446698

/* in network byte order */
struct nbd_request {
	uint32 magic; /* REQUEST_MAGIC */
	uint32 type;
	uint64 handle; //char handle[8];
	uint64 from;
	uint32 len;
} _PACKED;

/* in network byte order */
struct nbd_reply {
	uint32 magic; /* REPLY_MAGIC */
	uint32 error;
	uint64 handle; //char handle[8];
} _PACKED;

/* initialization protocol (ENBD ? or at least Linux specific ?) */

#define NBD_INIT_PASSWD "NBDMAGIC"
#define NBD_INIT_MAGIC 0x0000420281861253LL

/* in network byte order */
struct nbd_init_packet {
	uint8 passwd[8]; /* "NBDMAGIC" */
	uint64 magic; /* INIT_MAGIC */
	uint64 device_size; /* size in bytes */
	uint8 dummy[128]; /* reserved for future use */
} _PACKED;
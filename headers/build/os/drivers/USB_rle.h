/* 
** USB_rle.h
**
** Copyright 1999, Be Incorporated. All Rights Reserved.
**
*/

#ifndef _USB_RLE_H
#define _USB_RLE_H

#ifdef __cplusplus
extern "C" {
#endif

struct _usbd_param_hdr;

/*
Run Length encoding records for isochronous IN transfers.

Run Length encoding records are used to identify which samples in
the buffer are good which are bad and which are missing. 
Bad bytes are not extracted from the buffer, but are padded to next
nearest sample boundary.  The ultimate consumer of the buffer
should also receive the RLE array.

RLE records are constructed based on the following rules:

1.	an RLE record contains a sample count and a status 
	(good, bad, missing or unknown). A buffer has
	associated with it an array of rle records. The number of 
	rle records available is specified in the RLE header. The
	number used is also in the RLE header.

2.	Within the scope of a buffer, successive packets with the
	same completion status are represented with (1) rle record.
	For example, after three transactions which have completion
	status of success, the byte count in the rle record for this
	position in the data stream represents the bytes received in
	all three packets.

3.	New rle records are initialized each time the status for a
	given packet differs from that of the previous packet.

*/

#define RLE_GOOD       1
#define RLE_BAD        2
#define RLE_MISSING    3
#define RLE_UNKNOWN    4

/*
Name:    rle
Purpose: used to represent the state of a portion of a data buffer
Fields:
	rle_status		will contain only the values: RLE_GOOD, RLE_BAD, RLE_MISSING
	sample_count	the number of usb samples in the buffer associated with this rle
					record.
Notes:
	If the buffer length field in queue_buffer_single structure changes to an
	uint32 from uin16, then the sample_count data type must 
	track this change.
*/
typedef struct rle {
	uint16	rle_status;
	uint16	sample_count;
} rle;


/*
Name:    rlea
Purpose: used as the primary rle information data structure between the
	USB driver stack and a consuming client.

Fields:
	length		the number of rle records available in this structure.
	num_valid	filled in by the USB driver. indicates the number of valid 
				records filled.
	rles[]		unconstrained array of rle records.
*/
typedef struct rlea {
	uint16	length;
	uint16	num_valid;
	rle		rles[1];
} rlea;


#ifdef __cplusplus
}
#endif

#endif

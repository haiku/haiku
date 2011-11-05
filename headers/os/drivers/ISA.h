/*******************************************************************************
/
/	File:			ISA.h
/
/	Description:	Interface to ISA module
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _ISA_H
#define _ISA_H

//#include <SupportDefs.h>
#include <bus_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---
	ISA scatter/gather dma support.
--- */

typedef struct {
	ulong	address;				/* memory address (little endian!) 4 bytes */
	ushort	transfer_count;			/* # transfers minus one (little endian!) 2 bytes*/
	uchar	reserved;				/* filler, 1byte*/
    uchar   flag;					/* end of link flag, 1byte */
} isa_dma_entry;

#define 	B_LAST_ISA_DMA_ENTRY	0x80	/* sets end of link flag in isa_dma_entry */

enum {
	B_8_BIT_TRANSFER,
	B_16_BIT_TRANSFER
};

#define B_MAX_ISA_DMA_COUNT	0x10000

typedef struct isa_module_info {
	bus_manager_info	binfo;

	uint8			(*read_io_8) (int mapped_io_addr);
	void			(*write_io_8) (int mapped_io_addr, uint8 value);
	uint16			(*read_io_16) (int mapped_io_addr);
	void			(*write_io_16) (int mapped_io_addr, uint16 value);
	uint32			(*read_io_32) (int mapped_io_addr);
	void			(*write_io_32) (int mapped_io_addr, uint32 value);

	void *			(*ram_address) (const void *physical_address_in_system_memory);

	long			(*make_isa_dma_table) (
							const void		*buffer,		/* buffer to make a table for */
							long			buffer_size,	/* buffer size */
							ulong			num_bits,		/* dma transfer size that will be used */
							isa_dma_entry	*table,			/* -> caller-supplied scatter/gather table */
							long			num_entries		/* max # entries in table */
						);
	long			(*start_isa_dma) (
							long	channel,				/* dma channel to use */
							void	*buf,					/* buffer to transfer */
							long	transfer_count,			/* # transfers */
							uchar	mode,					/* mode flags */
							uchar	e_mode					/* extended mode flags */
						);
	long			(*start_scattered_isa_dma) (
							long				channel,	/* channel # to use */
							const isa_dma_entry	*table,		/* physical address of scatter/gather table */
							uchar				mode,		/* mode flags */
							uchar				emode		/* extended mode flags */
						);
	long			(*lock_isa_dma_channel) (long channel);
	long			(*unlock_isa_dma_channel) (long channel);
} isa_module_info;
	
#define	B_ISA_MODULE_NAME		"bus_managers/isa/v1"

#ifdef __cplusplus
}
#endif

#endif	/* _ISA_H */

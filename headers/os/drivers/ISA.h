/* 
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ISA_H
#define _ISA_H


#include <SupportDefs.h>
#include <bus_manager.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct isa_dma_entry {
	uint32	address;
	uint16	transfer_count;
	uchar	reserved;
	uchar	flag;
} isa_dma_entry;


#define 	B_LAST_ISA_DMA_ENTRY	0x80


enum {
	B_8_BIT_TRANSFER,
	B_16_BIT_TRANSFER
};


#define B_MAX_ISA_DMA_COUNT	0x10000


typedef struct isa_module_info isa_module_info;
struct isa_module_info {
	bus_manager_info	binfo;

	uint8				(*read_io_8) (int32 mapped_io_addr);
	void				(*write_io_8) (int32 mapped_io_addr, uint8 value);
	uint16				(*read_io_16) (int32 mapped_io_addr);
	void				(*write_io_16) (int32 mapped_io_addr, uint16 value);
	uint32				(*read_io_32) (int32 mapped_io_addr);
	void				(*write_io_32) (int32 mapped_io_addr, uint32 value);

	void*				(*ram_address) 
							(const void * physical_address_in_system_memory);

	int32				(*make_isa_dma_table) (
							const void		*buffer,
							int32			buffer_size,
							uint32			num_bits,
							isa_dma_entry	*table,
							int32			num_entries
						);
	int32				(*start_isa_dma) (
							int32	channel,
							void	*buf,
							int32	transfer_count,
							uchar	mode,
							uchar	e_mode
						);
	int32				(*start_scattered_isa_dma) (
							int32					channel,
							const isa_dma_entry*	table,
							uchar					mode,
							uchar					emode
						);
	int32				(*lock_isa_dma_channel) (int32 channel);
	int32				(*unlock_isa_dma_channel) (int32 channel);
};


#define	B_ISA_MODULE_NAME		"bus_managers/isa/v1"


#ifdef __cplusplus
}
#endif


#endif	/* _ISA_H */

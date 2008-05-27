/*
 * Copyright 2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	François Revol (revol@free.fr)
 */

/*
	ISA bus manager

	arch-dependant functions
*/

#ifdef __cplusplus
extern "C" {
#endif

/* implemented in arch/<cpu>/ */
extern status_t arch_isa_init(void);
extern uint8 arch_isa_read_io_8(int mapped_io_addr);
extern void arch_isa_write_io_8(int mapped_io_addr, uint8 value);
extern uint16 arch_isa_read_io_16(int mapped_io_addr);
extern void arch_isa_write_io_16(int mapped_io_addr, uint16 value);
extern uint32 arch_isa_read_io_32(int mapped_io_addr);
extern void arch_isa_write_io_32(int mapped_io_addr, uint32 value);
extern void *arch_isa_ram_address(const void *);
extern status_t arch_start_isa_dma(long channel, void *buf,
								   long transfer_count,
								   uchar mode, uchar e_mode);

#ifdef __cplusplus
}
#endif

/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SBI_SYSCALLS_H_
#define _SBI_SYSCALLS_H_


#include <stdint.h>


enum {
	SBI_SUCCESS               =  0,
	SBI_ERR_FAILED            = -1,
	SBI_ERR_NOT_SUPPORTED     = -2,
	SBI_ERR_INVALID_PARAM     = -3,
	SBI_ERR_DENIED            = -4,
	SBI_ERR_INVALID_ADDRESS   = -5,
	SBI_ERR_ALREADY_AVAILABLE = -6,
};

struct sbiret {
	long error; // a0
	long value; // a1
};

enum {
	SBI_HART_STATE_STARTED         = 0,
	SBI_HART_STATE_STOPPED         = 1,
	SBI_HART_STATE_START_PENDING   = 2,
	SBI_HART_STATE_STOP_PENDING    = 3,
	SBI_HART_STATE_SUSPENDED       = 4,
	SBI_HART_STATE_SUSPEND_PENDING = 5,
	SBI_HART_STATE_RESUME_PENDING  = 6,
};

enum {
	SBI_RESET_TYPE_SHUTDOWN    = 0,
	SBI_RESET_TYPE_COLD_REBOOT = 1,
	SBI_RESET_TYPE_WARM_REBOOT = 2,
};

enum {
	SBI_RESET_REASON_NONE           = 0,
	SBI_RESET_REASON_SYSTEM_FAILURE = 1,
};

// a7: EID, a6: FID

extern "C" {

// Base Extension (EID #0x10)
struct sbiret sbi_get_spec_version(void);             // FID #0
struct sbiret sbi_get_impl_id(void);                  // FID #1
struct sbiret sbi_get_impl_version(void);             // FID #2
struct sbiret sbi_probe_extension(long extension_id); // FID #3
struct sbiret sbi_get_mvendorid(void);                // FID #4
struct sbiret sbi_get_marchid(void);                  // FID #5
struct sbiret sbi_get_mimpid(void);                   // FID #6

// Legacy Extensions (EIDs #0x00 - #0x0F)
void sbi_set_timer_legacy(uint64_t stime_value);                // EID #0x00
void sbi_console_putchar_legacy(int ch);                        // EID #0x01
int sbi_console_getchar_legacy(void);                           // EID #0x02
void sbi_clear_ipi_legacy(void);                                // EID #0x03
void sbi_send_ipi_legacy(const unsigned long *hart_mask);       // EID #0x04
void sbi_remote_fence_i_legacy(const unsigned long *hart_mask); // EID #0x05
void sbi_remote_sfence_vma_legacy(                              // EID #0x06
	const unsigned long *hart_mask,
	unsigned long start,
	unsigned long size);
void sbi_remote_sfence_vma_asid_legacy(                         // EID #0x07
	const unsigned long *hart_mask,
	unsigned long start,
	unsigned long size,
	unsigned long asid);
void sbi_shutdown_legacy(void);                                 // EID #0x08

// Timer Extension (EID #0x54494D45 "TIME")
struct sbiret sbi_set_timer(uint64_t stime_value); // FID #0

// IPI Extension (EID #0x735049 "sPI: s-mode IPI")
struct sbiret sbi_send_ipi( // FID #0
	unsigned long hart_mask,
	unsigned long hart_mask_base);

// RFENCE Extension (EID #0x52464E43 "RFNC")
struct sbiret sbi_remote_fence_i( // FID #0
	unsigned long hart_mask,
	unsigned long hart_mask_base);
struct sbiret sbi_remote_sfence_vma( // FID #1
	unsigned long hart_mask,
	unsigned long hart_mask_base,
	unsigned long start_addr,
	unsigned long size);
struct sbiret sbi_remote_sfence_vma_asid( // FID #2
	unsigned long hart_mask,
	unsigned long hart_mask_base,
	unsigned long start_addr,
	unsigned long size,
	unsigned long asid);
struct sbiret sbi_remote_hfence_gvma_vmid( // FID #3
	unsigned long hart_mask,
	unsigned long hart_mask_base,
	unsigned long start_addr,
	unsigned long size,
	unsigned long vmid);
struct sbiret sbi_remote_hfence_gvma( // FID #4
	unsigned long hart_mask,
	unsigned long hart_mask_base,
	unsigned long start_addr,
	unsigned long size);
struct sbiret sbi_remote_hfence_vvma_asid( // FID #5
	unsigned long hart_mask,
	unsigned long hart_mask_base,
	unsigned long start_addr,
	unsigned long size,
	unsigned long asid);
struct sbiret sbi_remote_hfence_vvma( // FID #6
	unsigned long hart_mask,
	unsigned long hart_mask_base,
	unsigned long start_addr,
	unsigned long size);

// Hart State Management Extension (EID #0x48534D "HSM")
struct sbiret sbi_hart_start( // FID #0
	unsigned long hartid,
	unsigned long start_addr,
	unsigned long opaque);
struct sbiret sbi_hart_stop(void); // FID #1
struct sbiret sbi_hart_get_status(unsigned long hartid); // FID #2
struct sbiret sbi_hart_suspend( // FID #3
	uint32_t suspend_type,
	unsigned long resume_addr,
	unsigned long opaque);

// System Reset Extension (EID #0x53525354 "SRST")
struct sbiret sbi_system_reset(uint32_t reset_type, uint32_t reset_reason); // FID #0

}


#endif	// _SBI_SYSCALLS_H_

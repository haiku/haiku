/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#define SBI_SYSCALL(name, eid, fid) \
.globl name; \
.type name,%function; \
name: \
	li a7, eid; \
	li a6, fid; \
	ecall; \
	ret; \
.size	name, .-name

#define SBI_SYSCALL_LEGACY(name, eid) \
.globl name; \
.type name,%function; \
name: \
	li a7, eid; \
	ecall; \
	ret; \
.size	name, .-name


SBI_SYSCALL(sbi_get_spec_version, 0x10, 0)
SBI_SYSCALL(sbi_get_impl_id,      0x10, 1)
SBI_SYSCALL(sbi_get_impl_version, 0x10, 2)
SBI_SYSCALL(sbi_probe_extension,  0x10, 3)
SBI_SYSCALL(sbi_get_mvendorid,    0x10, 4)
SBI_SYSCALL(sbi_get_marchid,      0x10, 5)
SBI_SYSCALL(sbi_get_mimpid,       0x10, 6)

SBI_SYSCALL_LEGACY(sbi_set_timer_legacy,              0)
SBI_SYSCALL_LEGACY(sbi_console_putchar_legacy,        1)
SBI_SYSCALL_LEGACY(sbi_console_getchar_legacy,        2)
SBI_SYSCALL_LEGACY(sbi_clear_ipi_legacy,              3)
SBI_SYSCALL_LEGACY(sbi_send_ipi_legacy,               4)
SBI_SYSCALL_LEGACY(sbi_remote_fence_i_legacy,         5)
SBI_SYSCALL_LEGACY(sbi_remote_sfence_vma_legacy,      6)
SBI_SYSCALL_LEGACY(sbi_remote_sfence_vma_asid_legacy, 7)
SBI_SYSCALL_LEGACY(sbi_shutdown_legacy,               8)

SBI_SYSCALL(sbi_set_timer, 0x54494D45, 0)

SBI_SYSCALL(sbi_send_ipi, 0x735049, 0)

SBI_SYSCALL(sbi_remote_fence_i,          0x52464E43, 0)
SBI_SYSCALL(sbi_remote_sfence_vma,       0x52464E43, 1)
SBI_SYSCALL(sbi_remote_sfence_vma_asid,  0x52464E43, 2)
SBI_SYSCALL(sbi_remote_hfence_gvma_vmid, 0x52464E43, 3)
SBI_SYSCALL(sbi_remote_hfence_gvma,      0x52464E43, 4)
SBI_SYSCALL(sbi_remote_hfence_vvma_asid, 0x52464E43, 5)
SBI_SYSCALL(sbi_remote_hfence_vvma,      0x52464E43, 6)

SBI_SYSCALL(sbi_hart_start,      0x48534D, 0)
SBI_SYSCALL(sbi_hart_stop,       0x48534D, 1)
SBI_SYSCALL(sbi_hart_get_status, 0x48534D, 2)
SBI_SYSCALL(sbi_hart_suspend,    0x48534D, 3)

SBI_SYSCALL(sbi_system_reset,    0x53525354, 0)

#ifdef ARCH_x86
#	include "arch/x86/syscalls.inc"
#endif
#ifdef ARCH_alpha
#	include "arch/alpha/syscalls.inc"
#endif
#ifdef ARCH_sh4
#	include "arch/sh4/syscalls.inc"
#endif
#ifdef ARCH_sparc
#	include "arch/sparc/syscalls.inc"
#endif
#ifdef ARCH_sparc64
#	include "arch/sparc64/syscalls.inc"
#endif
#ifdef ARCH_mips
#	include "arch/mips/syscalls.inc"
#endif
#ifdef ARCH_ppc
#	include "arch/ppc/syscalls.inc"
#endif
#ifdef ARCH_m68k
#	include "arch/m68k/syscalls.inc"
#endif

/* ToDo: we should do something about this file; either machine generate it */
/* or make it more intelligent... it's really boring to maintain this one. */

SYSCALL0(_kern_null, 0)

/* VFS calls */
SYSCALL4(_kern_mount, 1)
SYSCALL1(_kern_unmount, 2)
SYSCALL0(_kern_sync, 3)
SYSCALL2(_kern_open, 4)
SYSCALL5(_kern_open_entry_ref, 81)
SYSCALL1(_kern_close, 5)
SYSCALL1(_kern_fsync, 6)
SYSCALL5(_kern_read, 7)
SYSCALL5(_kern_write, 8)
SYSCALL4(_kern_seek, 9)
SYSCALL1(_kern_open_dir, 77)
SYSCALL4(_kern_open_dir_entry_ref, 82)
SYSCALL3(_kern_open_dir_node_ref, 83)
SYSCALL2(_kern_create_dir, 78)
SYSCALL5(_kern_create_dir_entry_ref, 85)
SYSCALL1(_kern_remove_dir, 93)
SYSCALL3(_kern_create_symlink, 86)
SYSCALL2(_kern_create_link, 92)
SYSCALL3(_kern_read_link, 87)
SYSCALL4(_kern_read_dir, 75)
SYSCALL1(_kern_rewind_dir, 76)
SYSCALL4(_kern_ioctl, 10)
SYSCALL3(_kern_create, 11)
SYSCALL6(_kern_create_entry_ref, 84)
SYSCALL1(_kern_unlink, 12)
SYSCALL2(_kern_rename, 13)
SYSCALL2(_kern_access, 73);
SYSCALL3(_kern_read_stat, 74)
SYSCALL4(_kern_write_stat, 100)
SYSCALL4(_kern_read_path_stat, 14)
SYSCALL5(_kern_write_path_stat, 15)
SYSCALL2(_kern_getcwd, 41)
SYSCALL2(_kern_setcwd, 42)
SYSCALL1(_kern_dup, 61)
SYSCALL2(_kern_dup2, 62)
SYSCALL7(_kern_select, 112)
SYSCALL4(_kern_poll, 113)

/* VFS attribute calls */
SYSCALL2(_kern_open_attr_dir, 97)
SYSCALL4(_kern_create_attr, 98)
SYSCALL3(_kern_open_attr, 99)
SYSCALL2(_kern_remove_attr, 101)
SYSCALL4(_kern_rename_attr, 102)

/* VFS index calls */
SYSCALL1(_kern_open_index_dir, 106)
SYSCALL4(_kern_create_index, 107)
SYSCALL3(_kern_read_index_stat, 108)
SYSCALL2(_kern_remove_index, 109)

/* semaphore calls */
SYSCALL2(sys_create_sem, 18)
SYSCALL1(sys_delete_sem, 19)
SYSCALL1(sys_acquire_sem, 20)
SYSCALL4(sys_acquire_sem_etc, 21)
SYSCALL1(sys_release_sem, 22)
SYSCALL3(sys_release_sem_etc, 23)
SYSCALL2(sys_sem_get_count, 57)
SYSCALL3(sys_get_sem_info, 58)
SYSCALL4(sys_get_next_sem_info, 59)
SYSCALL2(sys_set_sem_owner, 60)

/* thread/team calls */
SYSCALL1(_kern_find_thread, 24)
SYSCALL5(_kern_spawn_thread, 35)
SYSCALL1(_kern_kill_thread, 36)
SYSCALL1(_kern_suspend_thread, 37)
SYSCALL1(_kern_resume_thread, 38)
SYSCALL2(_kern_rename_thread, 141)
SYSCALL4(_kern_snooze_etc, 17)
SYSCALL4(_kern_send_data, 94)
SYSCALL3(_kern_receive_data, 95)
SYSCALL1(_kern_has_data, 96)
SYSCALL2(_kern_wait_for_thread, 27)
SYSCALL2(_kern_wait_for_team, 28)
SYSCALL2(_kern_get_thread_info, 88)
SYSCALL3(_kern_get_next_thread_info, 89)
SYSCALL2(_kern_set_thread_priority, 121)
SYSCALL1(_kern_exit, 25)
SYSCALL5(_kern_create_team, 26)
SYSCALL2(_kern_get_team_info, 90)
SYSCALL2(_kern_get_next_team_info, 91)
SYSCALL1(_kern_kill_team, 39)
SYSCALL0(_kern_get_current_team, 40)

/* image calls */
SYSCALL2(_kern_register_image, 114)
SYSCALL1(_kern_unregister_image, 115)
SYSCALL3(_kern_get_image_info, 116)
SYSCALL4(_kern_get_next_image_info, 117)

/* node monitor calls */
SYSCALL6(_kern_start_watching, 118)
SYSCALL6(_kern_stop_watching, 119)
SYSCALL2(_kern_stop_notifying, 120)

/* time calls */
SYSCALL1(_kern_set_real_time_clock, 138)

/* VM calls */
SYSCALL6(_kern_create_area, 29)
SYSCALL5(_kern_clone_area, 30)
SYSCALL9(sys_vm_map_file, 31)
SYSCALL1(_kern_delete_area, 32)
SYSCALL2(_kern_get_area_info, 33)
SYSCALL1(_kern_find_area, 34)
SYSCALL3(_kern_get_next_area_info, 122)
SYSCALL2(_kern_set_area_protection, 123)
SYSCALL2(_kern_resize_area, 124)
SYSCALL1(_kern_area_for, 125)

/* port syscalls */
SYSCALL2(sys_port_create, 43)
SYSCALL1(sys_port_close, 44)
SYSCALL1(sys_port_delete, 45)
SYSCALL1(sys_port_find, 46)
SYSCALL2(sys_port_get_info, 47)
SYSCALL3(sys_port_get_next_port_info, 48)
SYSCALL1(sys_port_buffer_size, 49)
SYSCALL4(sys_port_buffer_size_etc, 50)
SYSCALL1(sys_port_count, 51)
SYSCALL4(sys_port_read, 52)
SYSCALL7(sys_port_read_etc, 53)
SYSCALL2(sys_port_set_owner, 54)
SYSCALL4(sys_port_write, 55)
SYSCALL7(sys_port_write_etc, 56)

/* misc calls */
SYSCALL0(_kern_system_time, 16)
//SYSCALL2(sys_team_get_table, 63)
SYSCALL2(_kern_getrlimit, 64)
SYSCALL2(_kern_setrlimit, 65)
SYSCALL6(sys_sysctl, 71)
//SYSCALL3(sys_socket, 72)
SYSCALL3(sys_setenv, 79)
SYSCALL2(sys_getenv, 80)
SYSCALL1(_kern_debug_output, 139)
SYSCALL2(_kern_get_system_info, 140);

/* atomic calls */
SYSCALL2(_kern_atomic_set, 126)
SYSCALL3(_kern_atomic_test_and_set, 127)
SYSCALL2(_kern_atomic_add, 128)
SYSCALL2(_kern_atomic_and, 129)
SYSCALL2(_kern_atomic_or, 130)
SYSCALL1(_kern_atomic_get, 131)
SYSCALL3(_kern_atomic_set64, 132)
SYSCALL5(_kern_atomic_test_and_set64, 133)
SYSCALL3(_kern_atomic_add64, 134)
SYSCALL3(_kern_atomic_and64, 135)
SYSCALL3(_kern_atomic_or64, 136)
SYSCALL1(_kern_atomic_get64, 137)

/* Signal handling calls */
SYSCALL0(sys_return_from_signal, 103)
SYSCALL3(sys_sigaction, 105)
SYSCALL2(sys_send_signal, 110)
SYSCALL3(sys_set_alarm, 111)

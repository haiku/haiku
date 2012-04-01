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
#ifdef ARCH_mipsel
#	include "arch/mipsel/syscalls.inc"
#endif
#ifdef ARCH_ppc
#	include "arch/ppc/syscalls.inc"
#endif
#ifdef ARCH_arm
#	include "arch/arm/syscalls.inc"
#endif
#ifdef ARCH_m68k
#	include "arch/m68k/syscalls.inc"
#endif

/* include the auto-generated file syscalls file */
#include "syscalls.S.inc"

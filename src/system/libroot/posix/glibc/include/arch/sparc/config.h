//#if !defined __ASSEMBLER__ && !defined _ISOMAC && !defined __OPTIMIZE__
//# error "glibc cannot be compiled without optimization"
//#endif

/* Another evil option when it comes to compiling the C library is
   --ffast-math since it alters the ABI.  */
#if defined __FAST_MATH__ && !defined TEST_FAST_MATH
# error "glibc must not be compiled with -ffast-math"
#endif

/* Define if using GNU ld, with support for weak symbols in a.out,
   and for symbol set and warning messages extensions in a.out and ELF.
   This implies HAVE_WEAK_SYMBOLS; set by --with-gnu-ld.  */
#define	HAVE_GNU_LD 1

/* Define if using ELF, which supports weak symbols.
   This implies HAVE_ASM_WEAK_DIRECTIVE set by --with-elf.  */
#define	HAVE_ELF 1

/* Define if using XCOFF. Set by --with-xcoff.  */
#undef	HAVE_XCOFF

/* Define if weak symbols are available via the `.weak' directive.  */
#define	HAVE_ASM_WEAK_DIRECTIVE 1

/* Define if weak symbols are available via the `.weakext' directive.  */
#undef	HAVE_ASM_WEAKEXT_DIRECTIVE

/* Define to the assembler line separator character for multiple
   assembler instructions per line.  Default is `;'  */
#undef ASM_LINE_SEP

/* Define if not using ELF, but `.init' and `.fini' sections are available.  */
#undef	HAVE_INITFINI

/* Define if __attribute__((section("foo"))) puts quotes around foo.  */
/*#define	HAVE_SECTION_QUOTES 1
 [zooey]: defining this causes assembler errors, and I don't think
          that any BeOS-gcc actually produces quotes in sections...
*/
#undef	HAVE_SECTION_QUOTES

/* Define if using the GNU assembler, gas.  */
#define	HAVE_GNU_AS 1

/* Define if the assembler supports the `.set' directive.  */
#define	HAVE_ASM_SET_DIRECTIVE 1

/* Define to the name of the assembler's directive for
   declaring a symbol global (default `.globl').  */
#define	ASM_GLOBAL_DIRECTIVE .globl

/* Define to the prefix before `object' or `function' in the
   assembler's `.type' directive, if it has one.  */
#undef	ASM_TYPE_DIRECTIVE_PREFIX

/* Define a symbol_name as a global .symbol_name for ld.  */
#undef	HAVE_ASM_GLOBAL_DOT_NAME

/* Define if the assembler generates debugging information directly.  */
#undef	HAVE_CPP_ASM_DEBUGINFO

/* Define if _Unwind_Find_FDE should be exported from glibc.  */
#undef  EXPORT_UNWIND_FIND_FDE

/* Define to use GNU libio instead of GNU stdio.
   This is defined by configure under --enable-libio.  */
#define	USE_IN_LIBIO 1

/* Define if using ELF and the assembler supports the `.previous'
   directive.  */
#define	HAVE_ASM_PREVIOUS_DIRECTIVE 1

/* Define if using ELF and the assembler supports the `.popsection'
   directive.  */
#undef	HAVE_ASM_POPSECTION_DIRECTIVE

/* Define if versioning of the library is wanted.  */
#undef	DO_VERSIONING

/* Defined to the oldest ABI we support, like 2.1.  */
#undef GLIBC_OLDEST_ABI

/* Define if static NSS modules are wanted.  */
#undef	DO_STATIC_NSS

/* Define if gcc uses DWARF2 unwind information for exception support.  */
#define	HAVE_DWARF2_UNWIND_INFO 1

/* Define if gcc uses DWARF2 unwind information for exception support
   with static variable. */
#define	HAVE_DWARF2_UNWIND_INFO_STATIC 1

/* Define if the compiler supports __builtin_expect.  */
#undef	HAVE_BUILTIN_EXPECT

/* Define if the compiler supports __builtin_memset.  */
#undef	HAVE_BUILTIN_MEMSET

/* Define if the __thread keyword is supported.  */
#undef HAVE___THREAD

/* Define if the compiler supports __attribute__((tls_model(""))).  */
#undef HAVE_TLS_MODEL_ATTRIBUTE

/* Define if the regparm attribute shall be used for local functions
   (gcc on ix86 only).  */
#define	USE_REGPARMS 0

/* Defined on PowerPC if the GCC being used has a problem with clobbering
   certain registers (CR0, MQ, CTR, LR) in asm statements.  */
#undef	BROKEN_PPC_ASM_CR0

/* Defined on SPARC if ld doesn't handle R_SPARC_WDISP22 against .hidden
   symbol.  sysdeps/sparc/sparc32/elf/configure.  */
#undef	BROKEN_SPARC_WDISP22

/* Define if the linker supports the -z combreloc option.  */
#undef	HAVE_Z_COMBRELOC

/* Define if the assembler supported .protected.  */
#undef	HAVE_PROTECTED

/* Define if the assembler supported .hidden.  */
#undef	HAVE_HIDDEN

/* Define if the compiler supports __attribute__ ((visibility (...))).  */
#undef	HAVE_VISIBILITY_ATTRIBUTE

/* Define if the compiler doesn't support __attribute__ ((visibility (...)))
   together with __asm__ redirection properly.  */
#undef	HAVE_BROKEN_VISIBILITY_ATTRIBUTE

/* Define if the compiler doesn't support __attribute__ ((alias (...)))
   together with __asm__ redirection properly.  */
#undef	HAVE_BROKEN_ALIAS_ATTRIBUTE

/* Define if _rtld_local structure should be forced into .sdata section.  */
#undef	HAVE_SDATA_SECTION

/* Define if binutils support TLS handling.  */
#undef	HAVE_TLS_SUPPORT

/* Define if the linker supports .preinit_array/.init_array/.fini_array
   sections.  */
#undef	HAVE_INITFINI_ARRAY

/* Define if the access to static and hidden variables is position independent
   and does not need relocations.  */
#undef	PI_STATIC_AND_HIDDEN

/* Define this to disable the `hidden_proto' et al macros in
   include/libc-symbols.h that avoid PLT slots in the shared objects.  */
#undef	NO_HIDDEN


/* Defined to some form of __attribute__ ((...)) if the compiler supports
   a different, more efficient calling convention.  */
#if USE_REGPARMS && !defined PROF && !defined __BOUNDED_POINTERS__
# define internal_function __attribute__ ((regparm (3), stdcall))
#endif

/* Linux specific: minimum supported kernel version.  */
#undef	__LINUX_KERNEL_VERSION

/* Override abi-tags ABI version if necessary.  */
#undef  __ABI_TAG_VERSION

/* An extension in gcc 2.96 and up allows the subtraction of two
   local labels.  */
#undef	HAVE_SUBTRACT_LOCAL_LABELS

/* bash 2.0 introduced the _XXX_GNU_nonoption_argv_flags_ variable to help
   getopt determine whether a parameter is a flag or not.  This features
   was disabled later since it caused trouble.  We are by default therefore
   disabling the support as well.  */
#undef USE_NONOPTION_FLAGS

/* Mach/Hurd specific: define if mig supports the `retcode' keyword.  */
#undef	HAVE_MIG_RETCODE

/* Mach specific: define if the `host_page_size' RPC is available.  */
#undef	HAVE_HOST_PAGE_SIZE

/* Mach/i386 specific: define if the `i386_io_perm_*' RPCs are available.  */
#undef	HAVE_I386_IO_PERM_MODIFY

/* Mach/i386 specific: define if the `i386_set_gdt' RPC is available.  */
#undef	HAVE_I386_SET_GDT

/*
 */

#ifndef	_LIBC

/* These symbols might be defined by some sysdeps configures.
   They are used only in miscellaneous generator programs, not
   in compiling libc itself.   */

/* sysdeps/generic/configure.in */
#undef	HAVE_PSIGNAL

/* sysdeps/unix/configure.in */
#define	HAVE_STRERROR

/* sysdeps/unix/common/configure.in */
#undef	HAVE_SYS_SIGLIST
#undef	HAVE__SYS_SIGLIST
#undef	HAVE__CTYPE_
#undef	HAVE___CTYPE_
#undef	HAVE___CTYPE
#undef	HAVE__CTYPE__
#undef	HAVE__CTYPE
#undef	HAVE__LOCP

#endif

/*
 */

#ifdef	_LIBC

/* The zic and zdump programs need these definitions.  */

#define	HAVE_STRERROR	1

/* The locale code needs these definitions.  */

#define HAVE_REGEX 1

//#define HAVE_MMAP 1
#undef HAVE_MMAP

#endif


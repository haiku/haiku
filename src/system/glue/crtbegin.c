/* ===-- crtbegin.c - Provide __dso_handle ---------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

__attribute__((visibility("hidden")))
#ifdef CRT_SHARED
  void *__dso_handle = &__dso_handle;
#else
  void *__dso_handle = (void *)0;
#endif

static const long __EH_FRAME_LIST__[]
  __attribute__((section(".eh_frame"), aligned(4), visibility("hidden"))) = {};

struct object {
  void *p[8];
};

extern void __register_frame_info(const void *, void *)
  __attribute__((weak));

extern void *__deregister_frame_info(const void *) __attribute__((weak));
extern void *__deregister_frame_info_bases(const void *) __attribute__((weak));

#ifndef CRT_HAS_INITFINI_ARRAY
typedef void (*fp)(void);
static const fp __CTOR_LIST__[]
  __attribute__((section(".ctors"), aligned(sizeof(fp)), visibility("hidden"), used)) = { (fp)-1 };
extern const fp __CTOR_END__[] __attribute__((visibility("hidden")));
#endif

#ifdef CRT_SHARED
extern void __cxa_finalize(void *) __attribute__((weak));
#endif

static void __attribute__((used)) __do_init() {
  static _Bool __initialized;
  if (__builtin_expect(__initialized, 0))
    return;
  __initialized = 1;

  static struct object ob;
  if (__register_frame_info)
    __register_frame_info(__EH_FRAME_LIST__, &ob);

#ifndef CRT_HAS_INITFINI_ARRAY
  unsigned long n = (unsigned long)__CTOR_LIST__[0];
  if (n == (unsigned long)-1)
    for (n = 0; __CTOR_LIST__[n + 1] != 0; n++);
  for (unsigned long i = n; i >= 1; i--)
    __CTOR_LIST__[i]();
#endif
}

#ifndef CRT_HAS_INITFINI_ARRAY
static const fp __DTOR_LIST__[]
  __attribute__((section(".dtors"), aligned(sizeof(fp)), visibility("hidden"), used)) = { (fp)-1 };
extern const fp __DTOR_END__[] __attribute__((visibility("hidden")));
#endif

__attribute__((section(".init_array"), used))
  static void (*__init)(void) = __do_init;

static void __attribute__((used)) __do_fini() {
  static _Bool __finalized;
  if (__builtin_expect(__finalized, 0))
    return;
  __finalized = 1;

#ifdef CRT_SHARED
  if (__cxa_finalize)
    __cxa_finalize(__dso_handle);
#endif

  if (__deregister_frame_info)
    __deregister_frame_info(__EH_FRAME_LIST__);

#ifndef CRT_HAS_INITFINI_ARRAY
  for (unsigned long i = 1; __DTOR_LIST__[i]; i++)
    __DTOR_LIST__[i]();
#endif
}

__attribute__((section(".fini_array"), used))
  static void (*__fini)(void) = __do_fini;

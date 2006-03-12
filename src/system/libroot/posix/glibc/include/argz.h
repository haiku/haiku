#ifndef _ARGZ_H

#include <string/argz.h>

libc_hidden_proto (argz_delete)

extern size_t __argz_count_internal (__const char *__argz, size_t __len)
     __attribute_pure__ attribute_hidden;
extern void __argz_stringify_internal (char *__argz, size_t __len, int __sep)
     attribute_hidden;

#endif

#ifndef __CBUF_ADAPTER_H
#define __CBUF_ADAPTER_H

#include <SupportDefs.h>

struct cbuffer_t;
typedef struct cbuffer_t cbuffer;

size_t cbuf_getn_no_lock(cbuffer *, char *, size_t);
size_t cbuf_putn_no_lock(cbuffer *, char *, size_t);
cbuffer *cbuf_init(size_t size);
void cbuf_delete(cbuffer *buffer);
char cbuf_get(cbuffer *);
bool cbuf_mt(cbuffer *);
bool cbuf_full(cbuffer *);
status_t cbuf_put(cbuffer *, char);
status_t cbuf_unput(cbuffer *);
void cbuf_flush(cbuffer *);
size_t cbuf_size(cbuffer *);
size_t cbuf_avail(cbuffer *);
size_t cbuf_free(cbuffer *);
size_t cbuf_putn(cbuffer *, char *, size_t num_bytes);
size_t cbuf_getn(cbuffer *, char *, size_t num_bytes);
cpu_status cbuf_lock(cbuffer *);
void cbuf_unlock(cbuffer *, cpu_status);

#define cbuf cbuffer
#define cbuf_get_chain(size) cbuf_init(size)
#define cbuf_free_chain(chain) cbuf_delete(chain)
#define cbuf_memcpy_from_chain(dest, chain, offset, size) \
	(cbuf_getn(chain, dest, size) - size)
#define cbuf_memcpy_to_chain(chain, offset, source, size) \
	(cbuf_putn(chain, source, size) - size)


#endif

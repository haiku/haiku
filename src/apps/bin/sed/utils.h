#include <stdio.h>

void panic P_((const char *str, ...));

FILE *ck_fopen P_((const char *name, const char *mode));
void ck_fwrite P_((const VOID *ptr, size_t size, size_t nmemb, FILE *stream));
size_t ck_fread P_((VOID *ptr, size_t size, size_t nmemb, FILE *stream));
void ck_fflush P_((FILE *stream));
void ck_fclose P_((FILE *stream));

VOID *ck_malloc P_((size_t size));
VOID *xmalloc P_((size_t size));
VOID *ck_realloc P_((VOID *ptr, size_t size));
char *ck_strdup P_((const char *str));
VOID *ck_memdup P_((const VOID *buf, size_t len));
void ck_free P_((VOID *ptr));

struct buffer *init_buffer P_((void));
char *get_buffer P_((struct buffer *b));
size_t size_buffer P_((struct buffer *b));
void add_buffer P_((struct buffer *b, const char *p, size_t n));
void add1_buffer P_((struct buffer *b, int ch));
void free_buffer P_((struct buffer *b));

extern const char *myname;

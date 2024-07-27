/*
 * Copyright (c) 2007-2011, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#ifndef LIBSOLV_REPOPAGE_H
#define LIBSOLV_REPOPAGE_H

#define REPOPAGE_BLOBBITS 15
#define REPOPAGE_BLOBSIZE (1 << REPOPAGE_BLOBBITS)

typedef struct _Attrblobpage
{
  /* page_size == 0 means the page is not backed by some file storage.
     Otherwise it is L*2+(compressed ? 1 : 0), with L being the data
     length.  */
  unsigned int page_offset;
  unsigned int page_size;
} Attrblobpage;

typedef struct _Repopagestore {
  int pagefd;		/* file descriptor we're paging from */
  long file_offset;	/* pages in file start here */

  unsigned char *blob_store;
  unsigned int num_pages;

  /* mapped_at[page] == -1  --> not loaded, otherwise offset into
     store->blob_store.  The size of the mapping is REPOPAGE_BLOBSIZE
     except for the last page.  */
  unsigned int *mapped_at;

  Attrblobpage *file_pages;

  /* mapped[i] is -1 if nothing is mapped at logical page I,
   otherwise it contains the page number (of the mapped page).  */
  unsigned int *mapped;
  unsigned int nmapped;
  unsigned int rr_counter;
} Repopagestore;

#ifdef __cplusplus
extern "C" {
#endif

void repopagestore_init(Repopagestore *store);
void repopagestore_free(Repopagestore *store);

/* load pages pstart..pend into consecutive memory, return address */
unsigned char *repopagestore_load_page_range(Repopagestore *store, unsigned int pstart, unsigned int pend);

/* compress a page, return compressed len */
unsigned int repopagestore_compress_page(unsigned char *page, unsigned int len, unsigned char *cpage, unsigned int max);

/* setup page data for repodata_load_page_range */
int repopagestore_read_or_setup_pages(Repopagestore *store, FILE *fp, unsigned int pagesz, unsigned int blobsz);

void repopagestore_disable_paging(Repopagestore *store);

#ifdef __cplusplus
}
#endif

#endif	/* LIBSOLV_REPOPAGE_H */

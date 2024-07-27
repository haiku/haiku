/*
 * Copyright (c) 2007-2012, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * solvable.h
 * 
 * A solvable represents an object with name-epoch:version-release.arch
 * and dependencies
 */

#ifndef LIBSOLV_SOLVABLE_H
#define LIBSOLV_SOLVABLE_H

#include "pooltypes.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _Repo;

typedef struct _Solvable {
  Id name;
  Id arch;
  Id evr;			/* epoch:version-release */
  Id vendor;

  struct _Repo *repo;		/* repo we belong to */

  /* dependencies are offsets into repo->idarraydata */
  Offset provides;		/* terminated with Id 0 */
  Offset obsoletes;
  Offset conflicts;

  Offset requires;
  Offset recommends;
  Offset suggests;

  Offset supplements;
  Offset enhances;

} Solvable;

/* lookup functions */
Id solvable_lookup_type(Solvable *s, Id keyname);
Id solvable_lookup_id(Solvable *s, Id keyname);
unsigned long long solvable_lookup_num(Solvable *s, Id keyname, unsigned long long notfound);
unsigned int solvable_lookup_sizek(Solvable *s, Id keyname, unsigned int notfound);
const char *solvable_lookup_str(Solvable *s, Id keyname);
const char *solvable_lookup_str_poollang(Solvable *s, Id keyname);
const char *solvable_lookup_str_lang(Solvable *s, Id keyname, const char *lang, int usebase);
int solvable_lookup_bool(Solvable *s, Id keyname);
int solvable_lookup_void(Solvable *s, Id keyname);
const char *solvable_get_location(Solvable *s, unsigned int *medianrp);	/* old name */
const char *solvable_lookup_location(Solvable *s, unsigned int *medianrp);
const char *solvable_lookup_sourcepkg(Solvable *s);
const unsigned char *solvable_lookup_bin_checksum(Solvable *s, Id keyname, Id *typep);
const char *solvable_lookup_checksum(Solvable *s, Id keyname, Id *typep);
int solvable_lookup_idarray(Solvable *s, Id keyname, Queue *q);
int solvable_lookup_deparray(Solvable *s, Id keyname, Queue *q, Id marker);

/* setter functions */
void solvable_set_id(Solvable *s, Id keyname, Id id);
void solvable_set_num(Solvable *s, Id keyname, unsigned long long num);
void solvable_set_str(Solvable *s, Id keyname, const char *str);
void solvable_set_poolstr(Solvable *s, Id keyname, const char *str);
void solvable_add_poolstr_array(Solvable *s, Id keyname, const char *str);
void solvable_add_idarray(Solvable *s, Id keyname, Id id);
void solvable_add_deparray(Solvable *s, Id keyname, Id dep, Id marker);
void solvable_set_idarray(Solvable *s, Id keyname, Queue *q);
void solvable_set_deparray(Solvable *s, Id keyname, Queue *q, Id marker);
void solvable_unset(Solvable *s, Id keyname);

int solvable_identical(Solvable *s1, Solvable *s2);
Id solvable_selfprovidedep(Solvable *s); 

#ifdef __cplusplus
}
#endif

#endif /* LIBSOLV_SOLVABLE_H */

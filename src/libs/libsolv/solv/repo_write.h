/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * repo_write.h
 *
 */

#ifndef REPO_WRITE_H
#define REPO_WRITE_H

#include <stdio.h>

#include "repo.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int repo_write(Repo *repo, FILE *fp);
extern int repo_write_filtered(Repo *repo, FILE *fp, int (*keyfilter)(Repo *repo, Repokey *key, void *kfdata), void *kfdata, Queue *keyq);

extern int repodata_write(Repodata *data , FILE *fp);
extern int repodata_write_filtered(Repodata *data , FILE *fp, int (*keyfilter)(Repo *repo, Repokey *key, void *kfdata), void *kfdata, Queue *keyq);

extern int repo_write_stdkeyfilter(Repo *repo, Repokey *key, void *kfdata);

#ifdef __cplusplus
}
#endif

#endif

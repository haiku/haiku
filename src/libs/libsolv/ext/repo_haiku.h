/*
 * Copyright (c) 2011-2013, Ingo Weinhold <ingo_weinhold@gmx.de>
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */
 
#ifndef REPO_HAIKU_H
#define REPO_HAIKU_H

#include "repo.h"

#ifdef __cplusplus
extern "C" {
#endif

int repo_add_haiku_installed_packages(Repo *repo, const char *rootdir,
  int flags);
Id repo_add_haiku_package(Repo *repo, const char *hpkgPath, int flags);
int repo_add_haiku_packages(Repo *repo, const char *repoName, int flags);

#ifdef __cplusplus

namespace BPackageKit {
  class BPackageInfo;
}

Id repo_add_haiku_package_info(Repo *repo,
  const BPackageKit::BPackageInfo &packageInfo, int flags);

} /* extern "C" */

#endif /*__cplusplus*/

#endif /* REPO_HAIKU_H */

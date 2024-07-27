/*
 * Copyright (c) 2011-2013, Ingo Weinhold <ingo_weinhold@gmx.de>
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#include <package/PackageInfo.h>
#include <package/PackageInfoSet.h>
#include <package/PackageRoster.h>
#include <package/PackageVersion.h>
#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>

#include "repo_haiku.h"

using namespace BPackageKit;
using namespace BPackageKit::BHPKG;

static void add_dependency(Repo *repo, Offset &dependencies, const char *name,
  const char *version, int flags, const char* compatVersion = NULL)
{
  Pool *pool = repo->pool;

  Id dependency = pool_str2id(pool, name, 1);

  if (version && version[0] != '\0')
  {
    Id versionId = pool_str2id(pool, version, 1);

    if (compatVersion && compatVersion[0] != '\0')
      {
        versionId = pool_rel2id(pool, versionId, pool_str2id(pool, compatVersion, 1),
          REL_COMPAT, 1);
      }

    dependency = pool_rel2id(pool, dependency, versionId, flags, 1);
  }

  dependencies = repo_addid_dep(repo, dependencies, dependency, 0);
}

static void add_dependency(Repo *repo, Offset &dependencies, const char *name,
  const BPackageVersion &version, int flags)
{
  add_dependency(repo, dependencies, name, version.ToString(),
    flags);
}

static void add_resolvables(Repo *repo, Offset &dependencies,
  const BObjectList<BPackageResolvable> &resolvables)
{
  for (int32 i = 0; BPackageResolvable *resolvable = resolvables.ItemAt(i); i++)
    {
      add_dependency(repo, dependencies, resolvable->Name(),
        resolvable->Version().ToString(), REL_EQ,
        resolvable->CompatibleVersion().ToString());
    }
}

static void add_resolvable_expressions(Repo *repo, Offset &dependencies,
  const BObjectList<BPackageResolvableExpression> &expressions)
{
  for (int32 i = 0;
    BPackageResolvableExpression *expression = expressions.ItemAt(i); i++)
    {
      // It is possible that no version is specified. In that case any version
      // is acceptable.
      if (expression->Version().InitCheck() != B_OK)
        {
          BPackageVersion version;
          add_dependency(repo, dependencies, expression->Name(), NULL, 0);
          continue;
        }

      int flags = 0;
      switch (expression->Operator())
        {
          case B_PACKAGE_RESOLVABLE_OP_LESS:
            flags |= REL_LT;
            break;
          case B_PACKAGE_RESOLVABLE_OP_LESS_EQUAL:
            flags |= REL_LT | REL_EQ;
            break;
          case B_PACKAGE_RESOLVABLE_OP_EQUAL:
            flags |= REL_EQ;
            break;
          case B_PACKAGE_RESOLVABLE_OP_NOT_EQUAL:
            break;
          case B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL:
            flags |= REL_GT | REL_EQ;
            break;
          case B_PACKAGE_RESOLVABLE_OP_GREATER:
              flags |= REL_GT;
            break;
        }

      add_dependency(repo, dependencies, expression->Name(),
        expression->Version(), flags);
    }
}

static void add_replaces_list(Repo *repo, Offset &dependencies,
  const BStringList &packageNames)
{
  int32 count = packageNames.CountStrings();
  for (int32 i = 0; i < count; i++)
    {
      const BString &packageName = packageNames.StringAt(i);
      add_dependency(repo, dependencies, packageName, BPackageVersion(), 0);
    }
}

static Id add_package_info_to_repo(Repo *repo, Repodata *repoData,
  const BPackageInfo &packageInfo)
{
  Pool *pool = repo->pool;

  Id solvableId = repo_add_solvable(repo);
  Solvable *solvable = pool_id2solvable(pool, solvableId);
  // Prepend "pkg:" to package name, so "provides" don't match unless explicitly
  // specified this way.
  BString name("pkg:");
  name << packageInfo.Name();
  solvable->name = pool_str2id(pool, name, 1);
  if (packageInfo.Architecture() == B_PACKAGE_ARCHITECTURE_ANY)
    solvable->arch = ARCH_ANY;
  else if (packageInfo.Architecture() == B_PACKAGE_ARCHITECTURE_SOURCE)
    solvable->arch = ARCH_SRC;
  else
    solvable->arch = pool_str2id(pool,
      BPackageInfo::kArchitectureNames[packageInfo.Architecture()], 1);
  solvable->evr = pool_str2id(pool, packageInfo.Version().ToString(), 1);
  solvable->vendor = pool_str2id(pool, packageInfo.Vendor(), 1);
  repodata_set_str(repoData, solvable - pool->solvables, SOLVABLE_SUMMARY,
    packageInfo.Summary());
  repodata_set_str(repoData, solvable - pool->solvables, SOLVABLE_DESCRIPTION,
    packageInfo.Description());
  repodata_set_str(repoData, solvable - pool->solvables, SOLVABLE_PACKAGER,
    packageInfo.Packager());

  if (!packageInfo.Checksum().IsEmpty())
    repodata_set_checksum(repoData, solvable - pool->solvables,
      SOLVABLE_CHECKSUM, REPOKEY_TYPE_SHA256, packageInfo.Checksum());

  solvable->provides = repo_addid_dep(repo, solvable->provides,
    pool_rel2id(pool, solvable->name, solvable->evr, REL_EQ, 1), 0);

  add_resolvables(repo, solvable->provides, packageInfo.ProvidesList());
  add_resolvable_expressions(repo, solvable->requires,
    packageInfo.RequiresList());
  add_resolvable_expressions(repo, solvable->supplements,
    packageInfo.SupplementsList());
  add_resolvable_expressions(repo, solvable->conflicts,
    packageInfo.ConflictsList());
  add_resolvable_expressions(repo, solvable->enhances,
    packageInfo.FreshensList());
  add_replaces_list(repo, solvable->obsoletes, packageInfo.ReplacesList());
  // TODO: Check whether freshens and replaces does indeed work as intended
  // here.

  // TODO: copyrights, licenses, URLs, source URLs

  return solvableId;
}

static void add_installed_packages(Repo *repo, Repodata *repoData,
  BPackageInstallationLocation location)
{
  BPackageRoster roster;
  BPackageInfoSet packageInfos;
  if (roster.GetActivePackages(location, packageInfos) == B_OK)
    {
      BRepositoryCache::Iterator it = packageInfos.GetIterator();
      while (const BPackageInfo *packageInfo = it.Next())
        add_package_info_to_repo(repo, repoData, *packageInfo);
    }
}

int repo_add_haiku_installed_packages(Repo *repo, const char *rootdir,
  int flags)
{
  Repodata *repoData = repo_add_repodata(repo, flags);

  add_installed_packages(repo, repoData,
    B_PACKAGE_INSTALLATION_LOCATION_SYSTEM);
  add_installed_packages(repo, repoData, B_PACKAGE_INSTALLATION_LOCATION_HOME);

  if (!(flags & REPO_NO_INTERNALIZE))
    repodata_internalize(repoData);

  return 0;
}

Id repo_add_haiku_package(Repo *repo, const char *hpkgPath, int flags)
{
  BPackageInfo packageInfo;
  if (packageInfo.ReadFromPackageFile(hpkgPath) != B_OK)
    return 0;

  return repo_add_haiku_package_info(repo, packageInfo, flags);
}

int repo_add_haiku_packages(Repo *repo, const char *repoName, int flags)
{
  BPackageRoster roster;
  BRepositoryCache cache;
  if (roster.GetRepositoryCache(repoName, &cache) != B_OK)
    return 0;

  Repodata *repoData = repo_add_repodata(repo, flags);

  BRepositoryCache::Iterator it = cache.GetIterator();
  while (const BPackageInfo *packageInfo = it.Next())
    add_package_info_to_repo(repo, repoData, *packageInfo);

  if (!(flags & REPO_NO_INTERNALIZE))
    repodata_internalize(repoData);

  return 0;
}

Id repo_add_haiku_package_info(Repo *repo,
  const BPackageKit::BPackageInfo &packageInfo, int flags)
{
  if (packageInfo.InitCheck() != B_OK)
    return 0;
  
  Repodata *repoData = repo_add_repodata(repo, flags);

  Id id = add_package_info_to_repo(repo, repoData, packageInfo);

  if (!(flags & REPO_NO_INTERNALIZE))
    repodata_internalize(repoData);

  return id;
}

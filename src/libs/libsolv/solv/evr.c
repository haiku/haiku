/*
 * Copyright (c) 2007-2009, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * evr.c
 *
 * version compare
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "evr.h"
#include "pool.h"



#if defined(DEBIAN) || defined(MULTI_SEMANTICS)

/* debian type version compare */
int
solv_vercmp_deb(const char *s1, const char *q1, const char *s2, const char *q2)
{
  int r, c1, c2;
  while (1)
    {
      c1 = s1 < q1 ? *(const unsigned char *)s1++ : 0;
      c2 = s2 < q2 ? *(const unsigned char *)s2++ : 0;
      if ((c1 >= '0' && c1 <= '9') && (c2 >= '0' && c2 <= '9'))
	{
	  while (c1 == '0')
            c1 = s1 < q1 ? *(const unsigned char *)s1++ : 0;
	  while (c2 == '0')
            c2 = s2 < q2 ? *(const unsigned char *)s2++ : 0;
	  r = 0;
	  while ((c1 >= '0' && c1 <= '9') && (c2 >= '0' && c2 <= '9'))
	    {
	      if (!r)
		r = c1 - c2;
              c1 = s1 < q1 ? *(const unsigned char *)s1++ : 0;
              c2 = s2 < q2 ? *(const unsigned char *)s2++ : 0;
	    }
	  if (c1 >= '0' && c1 <= '9')
	    return 1;
	  if (c2 >= '0' && c2 <= '9')
	    return -1;
	  if (r)
	    return r < 0 ? -1 : 1;
	}
      c1 = c1 == '~' ? -1 : !c1 || (c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z')  ? c1 : c1 + 256;
      c2 = c2 == '~' ? -1 : !c2 || (c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'Z') || (c2 >= 'a' && c2 <= 'z')  ? c2 : c2 + 256;
      r = c1 - c2;
      if (r)
	return r < 0 ? -1 : 1;
      if (!c1)
	return 0;
    }
}

#endif

#if !defined(DEBIAN) || defined(MULTI_SEMANTICS)

/* rpm type version compare */
/* note: the code assumes that *q1 and *q2 are not alphanumeric! */

int
solv_vercmp_rpm(const char *s1, const char *q1, const char *s2, const char *q2)
{
  int r = 0;
  const char *e1, *e2;

  for (;;)
    {
      while (s1 < q1 && !(*s1 >= '0' && *s1 <= '9') &&
          !(*s1 >= 'a' && *s1 <= 'z') && !(*s1 >= 'A' && *s1 <= 'Z') && *s1 != '~')
	s1++;
      while (s2 < q2 && !(*s2 >= '0' && *s2 <= '9') &&
          !(*s2 >= 'a' && *s2 <= 'z') && !(*s2 >= 'A' && *s2 <= 'Z') && *s2 != '~')
	s2++;
      if (s1 < q1 && *s1 == '~')
        {
	  if (s2 < q2 && *s2 == '~')
	    {
	      s1++;
	      s2++;
	      continue;
	    }
	  return -1;
        }
      if (s2 < q2 && *s2 == '~')
	return 1;
      if (s1 >= q1 || s2 >= q2)
	break;
      if ((*s1 >= '0' && *s1 <= '9') || (*s2 >= '0' && *s2 <= '9'))
	{
	  while (*s1 == '0' && s1[1] >= '0' && s1[1] <= '9')
	    s1++;
	  while (*s2 == '0' && s2[1] >= '0' && s2[1] <= '9')
	    s2++;
	  for (e1 = s1; *e1 >= '0' && *e1 <= '9'; )
	    e1++;
	  for (e2 = s2; *e2 >= '0' && *e2 <= '9'; )
	    e2++;
	  r = (e1 - s1) - (e2 - s2);
          if (!r)
	    r = strncmp(s1, s2, e1 - s1);
          if (r)
	    return r > 0 ? 1 : -1;
	}
      else
	{
	  for (e1 = s1; (*e1 >= 'a' && *e1 <= 'z') || (*e1 >= 'A' && *e1 <= 'Z'); )
	    e1++;
	  for (e2 = s2; (*e2 >= 'a' && *e2 <= 'z') || (*e2 >= 'A' && *e2 <= 'Z'); )
	    e2++;
	  r = (e1 - s1) - (e2 - s2);
          if (r > 0)
	    {
	      r = strncmp(s1, s2, e2 - s2);
	      return r >= 0 ? 1 : -1;
	    }
          if (r < 0)
	    {
	      r = strncmp(s1, s2, e1 - s1);
	      return r <= 0 ? -1 : 1;
	    }
	  r = strncmp(s1, s2, e1 - s1);
	  if (r)
	    return r > 0 ? 1 : -1;
	}
      s1 = e1;
      s2 = e2;
    }
  return s1 < q1 ? 1 : s2 < q2 ? -1 : 0;
}

int
solv_vercmp_rpm_notilde(const char *s1, const char *q1, const char *s2, const char *q2)
{
  int r = 0;
  const char *e1, *e2;

  while (s1 < q1 && s2 < q2)
    {
      while (s1 < q1 && !(*s1 >= '0' && *s1 <= '9') &&
          !(*s1 >= 'a' && *s1 <= 'z') && !(*s1 >= 'A' && *s1 <= 'Z'))
	s1++;
      while (s2 < q2 && !(*s2 >= '0' && *s2 <= '9') &&
          !(*s2 >= 'a' && *s2 <= 'z') && !(*s2 >= 'A' && *s2 <= 'Z'))
	s2++;
      if ((*s1 >= '0' && *s1 <= '9') || (*s2 >= '0' && *s2 <= '9'))
	{
	  while (*s1 == '0' && s1[1] >= '0' && s1[1] <= '9')
	    s1++;
	  while (*s2 == '0' && s2[1] >= '0' && s2[1] <= '9')
	    s2++;
	  for (e1 = s1; *e1 >= '0' && *e1 <= '9'; )
	    e1++;
	  for (e2 = s2; *e2 >= '0' && *e2 <= '9'; )
	    e2++;
	  r = (e1 - s1) - (e2 - s2);
          if (!r)
	    r = strncmp(s1, s2, e1 - s1);
          if (r)
	    return r > 0 ? 1 : -1;
	}
      else
	{
	  for (e1 = s1; (*e1 >= 'a' && *e1 <= 'z') || (*e1 >= 'A' && *e1 <= 'Z'); )
	    e1++;
	  for (e2 = s2; (*e2 >= 'a' && *e2 <= 'z') || (*e2 >= 'A' && *e2 <= 'Z'); )
	    e2++;
	  r = (e1 - s1) - (e2 - s2);
          if (r > 0)
	    {
	      r = strncmp(s1, s2, e2 - s2);
	      return r >= 0 ? 1 : -1;
	    }
          if (r < 0)
	    {
	      r = strncmp(s1, s2, e1 - s1);
	      return r <= 0 ? -1 : 1;
	    }
	  r = strncmp(s1, s2, e1 - s1);
	  if (r)
	    return r > 0 ? 1 : -1;
	}
      s1 = e1;
      s2 = e2;
    }
  return s1 < q1 ? 1 : s2 < q2 ? -1 : 0;
}

#endif

#if defined(HAIKU) || defined(MULTI_SEMANTICS)

static int
solv_cmp_version_part_haiku(const char *s1, const char *q1, const char *s2,
  const char *q2)
{
  while (s1 < q1 && s2 < q2)
    {
      int cmp, len1, len2;
      const char *part1 = s1, *part2 = s2;

      /* compare non-number part */
      while (s1 < q1 && !isdigit(*s1))
        s1++;
      while (s2 < q2 && !isdigit(*s2))
        s2++;

      if (part1 != s1)
        {
          if (part2 == s2)
            return 1;

          len1 = s1 - part1;
          len2 = s2 - part2;
          cmp = strncmp(part1, part2, len1 < len2 ? len1 : len2);
          if (cmp != 0)
            return cmp;
          if (len1 != len2)
            return len1 - len2;
       }
      else if (part2 != s2)
        return -1;

      /* compare number part */
      part1 = s1;
      part2 = s2;

      while (s1 < q1 && isdigit(*s1))
        s1++;
      while (s2 < q2 && isdigit(*s2))
        s2++;

      while (part1 + 1 < s1 && *part1 == '0')
        part1++;
      while (part2 + 1 < s2 && *part2 == '0')
        part2++;

      len1 = s1 - part1;
      len2 = s2 - part2;
      if (len1 != len2)
        return len1 - len2;
      if (len1 == 0)
        return 0;

      cmp = strncmp(part1, part2, len1);
      if (cmp != 0)
       return cmp;
    }

  return s1 < q1 ? 1 : s2 < q2 ? -1 : 0;
}

int
solv_vercmp_haiku(const char *s1, const char *q1, const char *s2, const char *q2)
{
  const char *pre1 = s1;
  const char *pre2 = s2;
  int cmp;

  /* find pre-release separator */
  while (pre1 != q1 && *pre1 != '~')
    pre1++;
  while (pre2 != q2 && *pre2 != '~')
    pre2++;

  /* compare main versions */
  cmp = solv_cmp_version_part_haiku(s1, pre1, s2, pre2);
  if (cmp != 0)
    return cmp < 0 ? -1 : 1; /* must return -1, 0, or 1 */

  /* main versions are equal -- compare pre-release (none is greatest) */
  if (pre1 == q1)
    return pre2 == q2 ? 0 : 1;
  if (pre2 == q2)
    return -1;

  cmp = solv_cmp_version_part_haiku(pre1 + 1, q1, pre2 + 1, q2);
  return cmp == 0 ? 0 : cmp < 0 ? -1 : 1; /* must return -1, 0, or 1 */
}

#endif /* HAIKU */
 
 
/* 
 * the solv_vercmp variant your system uses.
 */
int
solv_vercmp(const char *s1, const char *q1, const char *s2, const char *q2)
{
#if defined(DEBIAN)
  return solv_vercmp_deb(s1, q1, s2, q2);
#elif defined(ARCHLINUX)
  return solv_vercmp_rpm_notilde(s1, q1, s2, q2);
#elif defined(HAIKU)
  return solv_vercmp_haiku(s1, q1, s2, q2);
#else
  return solv_vercmp_rpm(s1, q1, s2, q2);
#endif
}

#if defined(MULTI_SEMANTICS)
# define solv_vercmp (*(pool->disttype == DISTTYPE_DEB ? &solv_vercmp_deb : \
                        pool->disttype == DISTTYPE_HAIKU ? solv_vercmp_haiku : \
                        &solv_ver##cmp_rpm))
#elif defined(DEBIAN)
# define solv_vercmp solv_vercmp_deb
#elif defined(ARCHLINUX)
# define solv_vercmp solv_vercmp_rpm_notilde
#elif defined(HAIKU)
# define solv_vercmp solv_vercmp_haiku
#else
# define solv_vercmp solv_vercmp_rpm
#endif

/* edition (e:v-r) compare */
int
pool_evrcmp_str(const Pool *pool, const char *evr1, const char *evr2, int mode)
{
  int r;
  const char *s1, *s2;
  const char *r1, *r2;

  if (evr1 == evr2)
    return 0;

#if 0
  POOL_DEBUG(DEBUG_EVRCMP, "evrcmp %s %s mode=%d\n", evr1, evr2, mode);
#endif
  for (s1 = evr1; *s1 >= '0' && *s1 <= '9'; s1++)
    ;
  for (s2 = evr2; *s2 >= '0' && *s2 <= '9'; s2++)
    ;
  if (mode == EVRCMP_MATCH && (*evr1 == ':' || *evr2 == ':'))
    {
      /* empty epoch, skip epoch check */
      if (*s1 == ':')
	evr1 = s1 + 1;
      if (*s2 == ':')
	evr2 = s2 + 1;
      s1 = evr1;
      s2 = evr2;
    }

  /* compare the epoch */
  if (s1 == evr1 || *s1 != ':')
    s1 = 0;
  if (s2 == evr2 || *s2 != ':')
    s2 = 0;
  if (s1 && s2)
    {
      r = solv_vercmp(evr1, s1, evr2, s2);
      if (r)
	return r;
      evr1 = s1 + 1;
      evr2 = s2 + 1;
    }
  else if (s1)
    {
      if (!pool->promoteepoch)
	{
	  while (*evr1 == '0')
	    evr1++;
	  if (*evr1 != ':')
	    return 1;
	}
      evr1 = s1 + 1;
    }
  else if (s2)
    {
      while (*evr2 == '0')
	evr2++;
      if (*evr2 != ':')
	return -1;
      evr2 = s2 + 1;
    }

  /* same epoch, now split into version/release */
  for (s1 = evr1, r1 = 0; *s1; s1++)
    if (*s1 == '-')
      r1 = s1;
  for (s2 = evr2, r2 = 0; *s2; s2++)
    if (*s2 == '-')
      r2 = s2;
  r = 0;
  if (mode != EVRCMP_MATCH || (evr1 != (r1 ? r1 : s1) && evr2 != (r2 ? r2 : s2)))
    r = solv_vercmp(evr1, r1 ? r1 : s1, evr2, r2 ? r2 : s2);
  if (r)
    return r;

  if (mode == EVRCMP_COMPARE)
    {
      if (!r1 && r2)
	return -1;
      if (r1 && !r2)
	return 1;
    }
  if (mode == EVRCMP_COMPARE_EVONLY)
    return 0;
  if (mode == EVRCMP_MATCH_RELEASE)
    {
      /* rpm treats empty releases as missing, i.e "foo = 4-" is the same as "foo = 4" */
      if (r1 && r1 + 1 == s1)
	r1 = 0;
      if (r2 && r2 + 1 == s2)
	r2 = 0;
    }
  if (r1 && r2)
    {
      r1++;
      r2++;
      if (mode != EVRCMP_MATCH || (s1 != r1 && s2 != r2))
	{
	  if (pool->havedistepoch)
	    {
	      const char *d1, *d2;
	      for (d1 = r1; d1 < s1; d1++)
		if (*d1 == ':')
		  break;
	      for (d2 = r2; d2 < s2; d2++)
		if (*d2 == ':')
		  break;
	      /* XXX: promote just in one direction? */
	      r = solv_vercmp(r1, d1 ? d1 : s1, r2, d2 ? d2 : s2);
	      if (r == 0 && d1 < s1 && d2 < s2)
		r = solv_vercmp(d1 + 1, s1, d2 + 1, s2);
	    }
	  else
            r = solv_vercmp(r1, s1, r2, s2);
	}
    }
  else if (mode == EVRCMP_MATCH_RELEASE)
    {
      if (!r1 && r2)
	return -2;
      if (r1 && !r2)
	return 2;
    }
  return r;
}

int
pool_evrcmp(const Pool *pool, Id evr1id, Id evr2id, int mode)
{
  const char *evr1, *evr2;
  if (evr1id == evr2id)
    return 0;
  evr1 = pool_id2str(pool, evr1id);
  evr2 = pool_id2str(pool, evr2id);
  return pool_evrcmp_str(pool, evr1, evr2, mode);
}

int
pool_evrmatch(const Pool *pool, Id evrid, const char *epoch, const char *version, const char *release)
{
  const char *evr1;
  const char *s1;
  const char *r1;
  int r;

  evr1 = pool_id2str(pool, evrid);
  for (s1 = evr1; *s1 >= '0' && *s1 <= '9'; s1++)
    ;
  if (s1 != evr1 && *s1 == ':')
    {
      if (epoch)
	{
	  r = solv_vercmp(evr1, s1, epoch, epoch + strlen(epoch));
	  if (r)
	    return r;
	}
      evr1 = s1 + 1;
    }
  else if (epoch)
    {
      while (*epoch == '0')
	epoch++;
      if (*epoch)
	return -1;
    }
  for (s1 = evr1, r1 = 0; *s1; s1++)
    if (*s1 == '-')
      r1 = s1;
  if (version)
    {
      r = solv_vercmp(evr1, r1 ? r1 : s1, version, version + strlen(version));
      if (r)
	return r;
    }
  if (release)
    {
      if (!r1)
	return -1;
      r = solv_vercmp(r1 + 1, s1, release, release + strlen(release));
      if (r)
	return r;
    }
  return 0;
}


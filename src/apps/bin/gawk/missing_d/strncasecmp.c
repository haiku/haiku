/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strcasecmp.c	5.6 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */

#ifdef atarist
#include <sys/types.h>
#else
#define u_char unsigned char
#endif

/* This rather ugly macro is for VMS C */
#ifdef C
#undef C
#endif
#define C(c) ((u_char)c)  
/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static u_char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	C('\200'), C('\201'), C('\202'), C('\203'), C('\204'), C('\205'), C('\206'), C('\207'),
	C('\210'), C('\211'), C('\212'), C('\213'), C('\214'), C('\215'), C('\216'), C('\217'),
	C('\220'), C('\221'), C('\222'), C('\223'), C('\224'), C('\225'), C('\226'), C('\227'),
	C('\230'), C('\231'), C('\232'), C('\233'), C('\234'), C('\235'), C('\236'), C('\237'),
	C('\240'), C('\241'), C('\242'), C('\243'), C('\244'), C('\245'), C('\246'), C('\247'),
	C('\250'), C('\251'), C('\252'), C('\253'), C('\254'), C('\255'), C('\256'), C('\257'),
	C('\260'), C('\261'), C('\262'), C('\263'), C('\264'), C('\265'), C('\266'), C('\267'),
	C('\270'), C('\271'), C('\272'), C('\273'), C('\274'), C('\275'), C('\276'), C('\277'),
	C('\340'), C('\341'), C('\342'), C('\343'), C('\344'), C('\345'), C('\346'), C('\347'),
	C('\350'), C('\351'), C('\352'), C('\353'), C('\354'), C('\355'), C('\356'), C('\357'),
	C('\360'), C('\361'), C('\362'), C('\363'), C('\364'), C('\365'), C('\366'), C('\327'),
	C('\370'), C('\371'), C('\372'), C('\373'), C('\374'), C('\375'), C('\376'), C('\337'),
	C('\340'), C('\341'), C('\342'), C('\343'), C('\344'), C('\345'), C('\346'), C('\347'),
	C('\350'), C('\351'), C('\352'), C('\353'), C('\354'), C('\355'), C('\356'), C('\357'),
	C('\360'), C('\361'), C('\362'), C('\363'), C('\364'), C('\365'), C('\366'), C('\367'),
	C('\370'), C('\371'), C('\372'), C('\373'), C('\374'), C('\375'), C('\376'), C('\377'),
};

#undef C

int
strcasecmp(s1, s2)
	const char *s1, *s2;
{
	register u_char	*cm = charmap,
			*us1 = (u_char *)s1,
			*us2 = (u_char *)s2;

	while (cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return(0);
	return(cm[*us1] - cm[*--us2]);
}

int
strncasecmp(s1, s2, n)
	const char *s1, *s2;
	register size_t n;
{
	register u_char	*cm = charmap,
			*us1 = (u_char *)s1,
			*us2 = (u_char *)s2;

	while ((long)(--n) >= 0 && cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return(0);
	return((long)n < 0 ? 0 : cm[*us1] - cm[*--us2]);
}

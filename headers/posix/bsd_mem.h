/*
 * Copyright 2005-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*
   Make life easier for those porting old Berkeley style
   code...
*/
#ifndef _BSD_MEM_H_
#define _BSD_MEM_H_

#ifndef USG   /* when this is defined, these aren't needed */

#ifndef bcopy
#define bcopy(s, d, l)  memmove(d, s, l)
#endif
#ifndef bzero
#define bzero(s, l)  memset(s, 0, l)
#endif
#ifndef bcmp
#define bcmp(a, b, len) memcmp(a, b, len)
#endif

#ifndef index
#define index(str, chr) strchr(str, chr)
#endif
#ifndef rindex
#define rindex(str, chr) strrchr(str, chr)
#endif

#endif /* USG */

#endif /* _BSD_MEM_H_ */

#ifndef _CORE_PRIVATE__H
#define _CORE_PRIVATE__H


extern struct pool_ctl *mbpool;
extern struct pool_ctl *clpool;

extern int max_hdr;		/* largest link+protocol header */
extern int max_linkhdr;		/* largest link level header */
extern int max_protohdr;		/* largest protocol header */

#endif

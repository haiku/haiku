#ifndef _CORE_PRIVATE__H
#define _CORE_PRIVATE__H


struct ifnet;
struct uio;

int uiomove(char *cp, int n, struct uio *uio);

void in_if_detach(struct ifnet *ifp);
	// this removes all IP related references for this interface (route, address)

extern struct pool_ctl *mbpool;
extern struct pool_ctl *clpool;

extern int max_hdr;		/* largest link+protocol header */
extern int max_linkhdr;		/* largest link level header */
extern int max_protohdr;		/* largest protocol header */

#endif

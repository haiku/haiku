/* domain.h */

/* A domain is just a container for like minded protocols! */

#ifndef _SYS_DOMAIN_H
#define _SYS_DOMAIN_H

#ifdef __cplusplus
extern "C" {
#endif

struct domain {
	int dom_family;	                /* AF_INET and so on */
	char *dom_name;
	
	void (*dom_init)(void);	        /* initialise */
	
	struct protosw	*dom_protosw;	/* the protocols we have */
	struct domain *dom_next;
	
	int (*dom_rtattach)(void **, int);
	int dom_rtoffset;
	int dom_maxrtkey;
};

extern struct domain *domains;

void add_domain    (struct domain *dom, int fam);
void remove_domain (int fam);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_DOMAIN_H */

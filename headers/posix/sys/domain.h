/* domain.h */

/* A domain is just a container for like minded protocols! */

#ifndef _SYS_DOMAIN_H
#define _SYS_DOMAIN_H

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

#ifdef _KERNEL_MODE
  struct domain *domains;

  void add_domain    (struct domain *dom, int fam);
  void remove_domain (int fam);
#endif /* _KERNEL_MODE */

#endif /* _SYS_DOMAIN_H */

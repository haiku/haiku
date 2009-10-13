#ifndef _NL_TYPES_H_
#define _NL_TYPES_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/


#define	NL_SETD			0
#define	NL_CAT_LOCALE	1

typedef	int nl_item;
typedef	void *nl_catd;

#ifdef __cplusplus
extern "C" {
#endif

extern nl_catd catopen(const char *name, int oflag);
extern char *catgets(nl_catd cat, int setID, int msgID, const char *defaultMessage);
extern int catclose(nl_catd cat);

#ifdef __cplusplus
}
#endif

#endif	/* _NL_TYPES_H_ */

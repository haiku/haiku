/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_UNISTD_H_
#define _BSD_UNISTD_H_


#include_next <unistd.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#define L_SET	SEEK_SET
#define	L_INCR	SEEK_CUR
#define	L_XTND	SEEK_END


#ifdef __cplusplus
extern "C" {
#endif

void	endusershell(void);
char	*getpass(const char *prompt);
char	*getusershell(void);
int		issetugid(void);
void	setusershell(void);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_UNISTD_H_ */

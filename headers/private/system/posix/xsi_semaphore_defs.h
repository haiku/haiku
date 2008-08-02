/*
 * Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_XSI_SEM_H
#define SYSTEM_XSI_SEM_H

/*
 * Shared union used by the semctl option argument.
 * The user should declare it explicitly
 */
union semun {
	int				val;
	struct semid_ds	*buf;
	unsigned short	*array;
};

#endif

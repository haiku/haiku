/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Benaphore definition
*/


#ifndef _BENAPHORE_H
#define _BENAPHORE_H


typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

 
#define INIT_BEN(x, prefix)	( (x).ben = 0, (x).sem = create_sem(0, #prefix " benaphore"), (x).sem )
#define ACQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem);

#endif

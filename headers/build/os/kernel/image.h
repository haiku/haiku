/******************************************************************************
/
/	File:			image.h
/
/	Description:	Kernel interface for managing executable images.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _IMAGE_H
#define	_IMAGE_H

#include <BeBuild.h>
#include <OS.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C" {
#endif


/*---------------------------------------------------------*/
/*----- Image types, info, and functions ------------------*/

#define	B_INIT_BEFORE_FUNCTION_NAME		"initialize_before"
#define B_INIT_AFTER_FUNCTION_NAME		"initialize_after"
#define	B_TERM_BEFORE_FUNCTION_NAME		"terminate_before"
#define B_TERM_AFTER_FUNCTION_NAME		"terminate_after"

typedef	int32 image_id;

typedef enum {
	B_APP_IMAGE = 1,
	B_LIBRARY_IMAGE,
	B_ADD_ON_IMAGE,
	B_SYSTEM_IMAGE
} image_type;

typedef struct {
	image_id	id;
	image_type	type;
	int32		sequence;
	int32		init_order;
	void		(*init_routine)();
	void		(*term_routine)();
	dev_t		device;
	ino_t		node;
	char		name[MAXPATHLEN];
	void		*text;
	void		*data;
	int32		text_size;
	int32		data_size;
} image_info;

// flags for _kern_load_image()
enum {
	B_WAIT_TILL_LOADED	= 0x01,	// Wait till the loader has loaded and relocated
								// (but not yet initialized) the application
								// image and all dependencies. If not supplied,
								// the function returns before the loader
								// started to do anything at all, i.e. it
								// returns success, even if the executable
								// doesn't exist.
};

extern _IMPEXP_ROOT thread_id	load_image(int32 argc, const char **argv,
									const char **envp);
extern _IMPEXP_ROOT image_id	load_add_on(const char *path);
extern _IMPEXP_ROOT status_t	unload_add_on(image_id imageID);

/* private; use the macros, below */
extern _IMPEXP_ROOT status_t	_get_image_info (image_id imageID,
									image_info *info, size_t size);
extern _IMPEXP_ROOT status_t	_get_next_image_info (team_id team, int32 *cookie,
									image_info *info, size_t size);
/* use these */
#define get_image_info(image, info)                        \
              _get_image_info((image), (info), sizeof(*(info)))
#define get_next_image_info(team, cookie, info)   \
	          _get_next_image_info((team), (cookie), (info), sizeof(*(info)))


/*---------------------------------------------------------*/
/*----- symbol types and functions ------------------------*/

#define	B_SYMBOL_TYPE_DATA		0x1
#define	B_SYMBOL_TYPE_TEXT		0x2
#define B_SYMBOL_TYPE_ANY		0x5

extern _IMPEXP_ROOT status_t	get_image_symbol(image_id imid,
									const char *name, int32 sclass,	void **ptr);
extern _IMPEXP_ROOT status_t	get_nth_image_symbol(image_id imid, int32 index,
									char *buf, int32 *bufsize, int32 *sclass,
									void **ptr);


/*---------------------------------------------------------*/
/*----- cache manipulation --------------------------------*/

#define B_FLUSH_DCACHE         0x0001	/* dcache = data cache */
#define B_FLUSH_ICACHE         0x0004	/* icache = instruction cache */
#define B_INVALIDATE_DCACHE    0x0002	 
#define B_INVALIDATE_ICACHE    0x0008   

extern _IMPEXP_ROOT void	clear_caches(void *addr, size_t len, uint32 flags);

/*---------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif	/* _IMAGE_H */

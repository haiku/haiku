#ifndef _SIZE_T_H_
#define _SIZE_T_H_

/*
 *    XXX serious hack that doesn't really solve the problem.
 *       As of right now, some versions of the toolchain expect size_t to
 *       be unsigned long (newer ones than 2.95.2 and beos), and the older
 *       ones need it to be unsigned int. It's an actual failure when 
 *       operator new is declared. This will have to be resolved in the future. 
 */

#ifdef __BEOS__
typedef unsigned long       size_t;
typedef signed long         ssize_t;
#else
typedef unsigned int        size_t;
typedef signed int          ssize_t;
#endif

#endif /* _SIZE_T_H_ */

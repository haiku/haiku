/* 
** Distributed under the terms of the Haiku License.
*/
#ifndef _SETJMP_H_
#define _SETJMP_H_


#include <signal.h>


typedef struct __jmp_buf {
	int			regs[6];	/* saved registers, stack & program pointer */
	int			mask_was_saved;
	sigset_t	saved_mask;
} jmp_buf[1];

typedef jmp_buf sigjmp_buf;


#ifdef __cplusplus
extern "C" {
#endif

extern int	_setjmp(jmp_buf jumpBuffer);
extern int	setjmp(jmp_buf jumpBuffer);
extern int	sigsetjmp(jmp_buf jumpBuffer, int saveMask);

extern void	_longjmp(jmp_buf jumpBuffer, int value);
extern void	longjmp(jmp_buf jumpBuffer, int value);
extern void	siglongjmp(sigjmp_buf jumpBuffer, int value);

// ToDo: this is a temporary workaround for a bug in BeOS
//	(it exports setjmp/sigsetjmp functions that do not work)
#ifdef COMPILE_FOR_R5
#	define setjmp(buffer) __sigsetjmp((buffer), 0)
#	define sigsetjmp(buffer, mask) __sigsetjmp((buffer), (mask))
extern int __sigsetjmp(jmp_buf buffer, int saveMask);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SETJMP_H_ */

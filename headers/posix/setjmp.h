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

#ifdef __cplusplus
}
#endif

#endif /* _SETJMP_H_ */

#ifndef TABLE_HEADER
#define TABLE_HEADER

#define NELEMS              500     /* maximum number of symbols */

#define IGNORE_KIND           0     /* a variable or forward declaration */
#define FUNC_KIND             1     /* a method declaration */
#define CLASS_KIND            2     /* a class declaration */
#define INLINED_KIND          3     /* a method w/body in a class decl. */
#define SKEL_KIND             4     /* a method found in a code file */
#define DONE_FUNC_KIND       11     /* a fn that we've finished expanded */
#define DONE_CLASS_KIND      12     /* a class that we've finished expanded */

#endif

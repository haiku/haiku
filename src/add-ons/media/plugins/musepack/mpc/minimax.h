#ifndef MPP_MINIMAX_H
#define MPP_MINIMAX_H

#if   defined __GNUC__  &&  defined __cplusplus

# define maxi(A,B)  ( (A) >? (B) )
# define mini(A,B)  ( (A) <? (B) )
# define maxd(A,B)  ( (A) >? (B) )
# define mind(A,B)  ( (A) <? (B) )
# define maxf(A,B)  ( (A) >? (B) )
# define minf(A,B)  ( (A) <? (B) )

# define absi(A)    abs   (A)
# define absf(A)    fabsf (A)
# define absd(A)    fabs  (A)

#elif defined __GNUC__

# define maxi(A,B)  ( (A) > (B)  ?  (A)  :  (B) )
# define mini(A,B)  ( (A) < (B)  ?  (A)  :  (B) )
# define maxd(A,B)  ( (A) > (B)  ?  (A)  :  (B) )
# define mind(A,B)  ( (A) < (B)  ?  (A)  :  (B) )
# define maxf(A,B)  ( (A) > (B)  ?  (A)  :  (B) )
# define minf(A,B)  ( (A) < (B)  ?  (A)  :  (B) )

# define absi(A)    abs   (A)
# define absf(A)    fabsf (A)
# define absd(A)    fabs  (A)

#else

# define maxi(A,B)  ( (A) >  (B)  ?  (A)  :  (B) )
# define mini(A,B)  ( (A) <  (B)  ?  (A)  :  (B) )
# define maxd(A,B)  ( (A) >  (B)  ?  (A)  :  (B) )
# define mind(A,B)  ( (A) <  (B)  ?  (A)  :  (B) )
# define maxf(A,B)  ( (A) >  (B)  ?  (A)  :  (B) )
# define minf(A,B)  ( (A) <  (B)  ?  (A)  :  (B) )

# define absi(A)    ( (A) >= 0    ?  (A)  : -(A) )
# define absf(A)    ( (A) >= 0.f  ?  (A)  : -(A) )
# define absd(A)    ( (A) >= 0.   ?  (A)  : -(A) )

#endif /* GNUC && C++ */

#endif /* MPP_MINIMAX_H */

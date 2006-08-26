//begin o[d]
#include <math.h>
#include <assert.h>

#if NUMT_IS_DOUBLE
typedef double numT;

 inline void
 make_neg (double &x)
 {
   x = -x;
 }

 inline void
 reciprocate (double &x)
 {
   x = 1/x;
 }

 inline double
 dbl_val (double x)
 {
   return x;
 }

 inline bool
 is_zero (double x)
 {
   return x == 0.0;
 }

 inline double
 recip( double x)
 {
   return 1.0 / x;
 }

#elif NUMT_IS_PJM_MPQ
/* pjm's own hacked-up toy gmp wrapper */
# include <gmp.h>

class mpq;
typedef mpq numT;

class mpq
{
public:
  mpq()
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
  }

  mpq( mpq const &other)
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
    mpq_set( rep, other.rep);
  }

  mpq( long int n)
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
    if (n != 0)
      mpq_set_si (rep, n, 1ul);
  }

  mpq (int n)
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
    if (n != 0)
      mpq_set_si (rep, n, 1ul);
  }

  mpq (unsigned int n)
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
    if (n != 0)
      mpq_set_ui (rep, n, 1ul);
  }

  mpq (unsigned long n, unsigned long d = 1)
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
    if ((n != 0) || (d == 0))
      mpq_set_ui( rep, n, d);
    if (d != 1)
      mpq_canonicalize( rep);
  }

  mpq (double x)
    : fMagic( MPQ_MAGIC)
  {
    mpq_init( rep);
    if (x != 0.0) // common special case.
      {
	mpq_set_d( rep, x);
	mpq_canonicalize( rep);
      }
  }

  ~mpq()
  {
    assertIsMpq();
    mpq_clear (rep);
    fMagic = 0;
  }

  void assertIsMpq() const
  {
    assert( fMagic == MPQ_MAGIC);
  }

  mpq &operator=(mpq const &other)
  {
    assertIsMpq();
    mpq_set( rep, other.rep);
    return *this;
  }

  mpq &operator+= (mpq const &other)
  {
    assertIsMpq();
    mpq_add (rep, rep, other.rep);
    return *this;
  }

  mpq &operator-= (mpq const &other)
  {
    assertIsMpq();
    mpq_sub (rep, rep, other.rep);
    return *this;
  }

  mpq &operator*= (mpq const &other)
  {
    assertIsMpq();
    mpq_mul (rep, rep, other.rep);
    return *this;
  }

  mpq &operator/= (mpq const &other)
  {
    assertIsMpq();
    mpq_div (rep, rep, other.rep);
    return *this;
  }

  mpq operator* (mpq const &other) const
  {
    assertIsMpq();
    mpq prod;
    mpq_mul( prod.rep, rep, other.rep);
    return prod;
  }

  mpq operator* (unsigned other) const
  {
    assertIsMpq();
    mpq prod( other);
    mpq_mul( prod.rep, rep, prod.rep);
    return prod;
  }

  mpq operator/ (mpq const &other) const
  {
    assertIsMpq();
    mpq quotient;
    mpq_div( quotient.rep, rep, other.rep);
    return quotient;
  }

  mpq operator+ (mpq const &other) const
  {
    assertIsMpq();
    mpq sum;
    mpq_add( sum.rep, rep, other.rep);
    return sum;
  }

  mpq operator- (mpq const &other) const
  {
    assertIsMpq();
    mpq diff;
    mpq_sub( diff.rep, rep, other.rep);
    return diff;
  }

  bool operator< (mpq const &other) const
  {
    assertIsMpq();
    return mpq_cmp( rep, other.rep) < 0;
  }

  bool operator> (mpq const &other) const
    { return mpq_cmp( rep, other.rep) > 0; }

  bool operator== (mpq const &other) const
    { return mpq_equal( rep, other.rep); }

  bool operator!= (mpq const &other) const
    { return !mpq_equal( rep, other.rep); }

  bool operator< (int x) const
  {
    assertIsMpq();
    if (x == 0)
      return mpq_sgn( rep) < 0;
    else
      abort();
  }

  bool operator> (int x) const
  {
    assertIsMpq();
    if (x == 0)
      return mpq_sgn( rep) > 0;
    else
      abort();
  }

  bool operator>= (int x) const
  {
    assertIsMpq();
    if (x == 0)
      return mpq_sgn( rep) >= 0;
    else
      abort();
  }

  bool operator== (int x) const
  {
    assertIsMpq();
    if (x == 0)
      return mpq_sgn( rep) == 0;
    else if (x > 0)
      return mpq_cmp_ui( rep, x, 1u) == 0;
    else
      abort();
  }

  bool operator== (double x) const
  {
    assertIsMpq();
    if (x == 0.0)
      return mpq_sgn( rep) == 0;
    else
      abort();
  }

  bool isFinite() const
  {
    assertIsMpq();
    return mpz_sgn( &rep[0]._mp_den);
  }

  long get_lround() const
  {
    assertIsMpq();
    return lround( mpq_get_d( rep));
  }

  mpz_srcptr get_num_mpz_t() const
    { return mpq_numref( rep); }

  mpz_srcptr get_den_mpz_t() const
    { return mpq_denref( rep); }

public:
  mpq_t rep;
private:
  unsigned fMagic;
  static unsigned const MPQ_MAGIC = 0xc04546fb; // from /dev/random
};

 inline void
 make_neg (mpq &x)
 {
   mpq_neg (x.rep, x.rep);
 }

 inline void
 reciprocate (mpq &x)
 {
   mpq_inv (x.rep, x.rep);
 }

 inline mpq
 recip( mpq const &x)
 {
   mpq r;
   mpq_inv( r.rep, x.rep);
   return r;
 }

 inline double
 dbl_val (mpq const &x)
 {
   return mpq_get_d (x.rep);
 }

 inline bool
 is_zero (mpq const &x)
 {
   return mpq_sgn (x.rep) == 0;
 }

 // it should be better to define mp_free in the library
 extern "C" {
   extern void (*__gmp_free_func)(void *, size_t);
 }
 inline void mp_free(void *ptr, size_t size)
 {
   (*__gmp_free_func)(ptr, size);
 }

# ifndef qcNoStream
#  include <iostream>
 inline ostream & operator<<(ostream &o, const mpq &q)
 {
#  if 0
   char *num = mpz_get_str(0, 10, q.get_num_mpz_t());
   o << num;
   mp_free(num, strlen(num)+1);
   if (mpz_cmp_ui(q.get_den_mpz_t(), 1))
     {
       char *den = mpz_get_str(0, 10, q.get_den_mpz_t());
       o << '/' << den;
       mp_free(den, strlen(den)+1);
     }
   return o;
#  else
   o << dbl_val(q);
   return o;
#  endif
 }
# endif

 inline numT operator-(const numT &x)
 {
   numT ret = x;
   make_neg( ret);
   return ret;
 }

 inline numT fabs(numT const &x)
 {
   numT ret = x;
   if (mpq_sgn( ret.rep) < 0)
     ret.rep[0]._mp_num._mp_size = - ret.rep[0]._mp_num._mp_size;
   return ret;
 }

#elif NUMT_IS_GB_MPQ
/* Gerardo Ballabio's sophisticated gmp wrapper. */
# include "g++.h"
typedef mpq_class numT;

#else
# error "Don't know numT type."
#endif
//end o[d]

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

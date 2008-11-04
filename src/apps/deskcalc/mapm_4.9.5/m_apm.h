
/* 
 *  M_APM  -  m_apm.h
 *
 *  Copyright (C) 1999 - 2007   Michael C. Ring
 *
 *  Permission to use, copy, and distribute this software and its
 *  documentation for any purpose with or without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and
 *  that both that copyright notice and this permission notice appear
 *  in supporting documentation.
 *
 *  Permission to modify the software is granted. Permission to distribute
 *  the modified code is granted. Modifications are to be distributed by
 *  using the file 'license.txt' as a template to modify the file header.
 *  'license.txt' is available in the official MAPM distribution.
 *
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      This is the header file that the user will include.
 *
 *      $Log: m_apm.h,v $
 *      Revision 1.42  2007/12/03 02:28:25  mike
 *      update copyright
 *
 *      Revision 1.41  2007/12/03 01:21:35  mike
 *      Update license
 *      Update version to 4.9.5
 *
 *      Revision 1.40  2004/05/31 22:06:02  mike
 *      add % operator to C++ wrapper
 *
 *      Revision 1.39  2004/05/24 04:11:41  mike
 *      updated version to 4.9.2
 *
 *      Revision 1.38  2004/04/01 03:17:19  mike
 *      update version to 4.9.1
 *
 *      Revision 1.37  2004/01/02 20:40:49  mike
 *      fix date on copyright
 *
 *      Revision 1.36  2004/01/02 00:52:38  mike
 *      update version to 4.9
 *
 *      Revision 1.35  2003/11/23 05:12:46  mike
 *      update version
 *
 *      Revision 1.34  2003/07/21 20:59:54  mike
 *      update version to 4.8
 *
 *      Revision 1.33  2003/05/14 21:19:23  mike
 *      change version string
 *
 *      Revision 1.32  2003/05/06 21:29:11  mike
 *      add defines for lib versions (and prototypes)
 *
 *      Revision 1.31  2002/11/04 20:46:33  mike
 *      change definition of the M_APM structure
 *
 *      Revision 1.30  2002/11/03 23:36:24  mike
 *      added new function, m_apm_integer_pow_nr
 *
 *      Revision 1.29  2002/02/14 21:43:00  mike
 *      add set_random_seed prototype
 *
 *      Revision 1.28  2001/08/28 18:29:32  mike
 *      fix fixptstringexp
 *
 *      Revision 1.27  2001/08/27 22:45:03  mike
 *      fix typo
 *
 *      Revision 1.26  2001/08/27 22:43:06  mike
 *      add new fix pt functions to C++ wrapper
 *
 *      Revision 1.25  2001/08/26 22:09:13  mike
 *      add new prototype
 *
 *      Revision 1.24  2001/08/25 16:48:21  mike
 *      add new prototypes
 *
 *      Revision 1.23  2001/07/16 18:40:27  mike
 *      add free_all_mem, trim_mem_usage prototypes
 *
 *      Revision 1.22  2001/07/15 20:49:21  mike
 *      added is_odd, is_even, gcd, lcm functions
 *
 *      Revision 1.21  2001/03/25 21:24:55  mike
 *      add floor and ceil functions
 *
 *      Revision 1.20  2000/09/23 19:05:29  mike
 *      add _reciprocal prototype
 *
 *      Revision 1.19  2000/08/21 23:30:13  mike
 *      add _is_integer function
 *
 *      Revision 1.18  2000/07/06 00:10:15  mike
 *      redo declare for MM_cpp_min_precision
 *
 *      Revision 1.17  2000/07/04 20:59:43  mike
 *      move MM_cpp_min_precision into cplusplus block below
 *
 *      Revision 1.16  2000/07/04 20:49:04  mike
 *      move 'MM_cpp_min_precision' inside the extern "C"
 *      brackets
 *
 *      Revision 1.15  2000/04/06 21:19:38  mike
 *      minor final tweaks from Orion
 *
 *      Revision 1.14  2000/04/05 20:15:25  mike
 *      add cpp_min_precision
 *
 *      Revision 1.13  2000/04/04 22:20:09  mike
 *      updated some comments from Orion
 *
 *      Revision 1.12  2000/04/04 19:46:36  mike
 *      fix preincrement, postincrement operators
 *      added some comments
 *      added 'ipow' operators
 *
 *      Revision 1.11  2000/04/03 22:08:35  mike
 *      added MAPM C++ wrapper class
 *      supplied by Orion Sky Lawlor (olawlor@acm.org)
 *
 *      Revision 1.10  2000/04/03 18:40:28  mike
 *      add #define atan2 for alias
 *
 *      Revision 1.9  2000/04/03 18:05:23  mike
 *      added hyperbolic functions
 *
 *      Revision 1.8  2000/04/03 17:26:57  mike
 *      add cbrt prototype
 *
 *      Revision 1.7  1999/09/18 03:11:23  mike
 *      add new prototype
 *
 *      Revision 1.6  1999/09/18 03:08:25  mike
 *      add new prototypes
 *
 *      Revision 1.5  1999/09/18 01:37:55  mike
 *      added new prototype
 *
 *      Revision 1.4  1999/07/12 02:04:30  mike
 *      added new function prototpye (m_apm_integer_string)
 *
 *      Revision 1.3  1999/05/15 21:04:08  mike
 *      added factorial prototype
 *
 *      Revision 1.2  1999/05/12 20:50:12  mike
 *      added more constants
 *
 *      Revision 1.1  1999/05/12 20:48:25  mike
 *      Initial revision
 *
 *      $Id: m_apm.h,v 1.42 2007/12/03 02:28:25 mike Exp $
 */

#ifndef M__APM__INCLUDED
#define M__APM__INCLUDED

#ifdef __cplusplus
/* Comment this line out if you've compiled the library as C++. */
#define APM_CONVERT_FROM_C
#endif

#ifdef APM_CONVERT_FROM_C
extern "C" {
#endif

typedef unsigned char UCHAR;

typedef struct  {
	UCHAR	*m_apm_data;
	long	m_apm_id;
	int     m_apm_refcount;       /* <- used only by C++ MAPM class */
	int	m_apm_malloclength;
	int	m_apm_datalength;
	int	m_apm_exponent;
	int	m_apm_sign;
} M_APM_struct;

typedef M_APM_struct *M_APM;


#define MAPM_LIB_VERSION \
    "MAPM Library Version 4.9.5  Copyright (C) 1999-2007, Michael C. Ring"
#define MAPM_LIB_SHORT_VERSION "4.9.5"


/*
 *	convienient predefined constants
 */

extern	M_APM	MM_Zero;
extern	M_APM	MM_One;
extern	M_APM	MM_Two;
extern	M_APM	MM_Three;
extern	M_APM	MM_Four;
extern	M_APM	MM_Five;
extern	M_APM	MM_Ten;

extern	M_APM	MM_PI;
extern	M_APM	MM_HALF_PI;
extern	M_APM	MM_2_PI;
extern	M_APM	MM_E;

extern	M_APM	MM_LOG_E_BASE_10;
extern	M_APM	MM_LOG_10_BASE_E;
extern	M_APM	MM_LOG_2_BASE_E;
extern	M_APM	MM_LOG_3_BASE_E;


/*
 *	function prototypes
 */

extern	M_APM	m_apm_init(void);
extern	void	m_apm_free(M_APM);
extern	void	m_apm_free_all_mem(void);
extern	void	m_apm_trim_mem_usage(void);
extern	char	*m_apm_lib_version(char *);
extern	char	*m_apm_lib_short_version(char *);

extern	void	m_apm_set_string(M_APM, char *);
extern	void	m_apm_set_double(M_APM, double);
extern	void	m_apm_set_long(M_APM, long);

extern	void	m_apm_to_string(char *, int, M_APM);
extern  void	m_apm_to_fixpt_string(char *, int, M_APM);
extern  void	m_apm_to_fixpt_stringex(char *, int, M_APM, char, char, int);
extern  char	*m_apm_to_fixpt_stringexp(int, M_APM, char, char, int);
extern  void    m_apm_to_integer_string(char *, M_APM);

extern	void	m_apm_absolute_value(M_APM, M_APM);
extern	void	m_apm_negate(M_APM, M_APM);
extern	void	m_apm_copy(M_APM, M_APM);
extern	void	m_apm_round(M_APM, int, M_APM);
extern	int	m_apm_compare(M_APM, M_APM);
extern	int	m_apm_sign(M_APM);
extern	int	m_apm_exponent(M_APM);
extern	int	m_apm_significant_digits(M_APM);
extern	int	m_apm_is_integer(M_APM);
extern	int	m_apm_is_even(M_APM);
extern	int	m_apm_is_odd(M_APM);

extern	void	m_apm_gcd(M_APM, M_APM, M_APM);
extern	void	m_apm_lcm(M_APM, M_APM, M_APM);

extern	void	m_apm_add(M_APM, M_APM, M_APM);
extern	void	m_apm_subtract(M_APM, M_APM, M_APM);
extern	void	m_apm_multiply(M_APM, M_APM, M_APM);
extern	void	m_apm_divide(M_APM, int, M_APM, M_APM);
extern	void	m_apm_integer_divide(M_APM, M_APM, M_APM);
extern	void	m_apm_integer_div_rem(M_APM, M_APM, M_APM, M_APM);
extern	void	m_apm_reciprocal(M_APM, int, M_APM);
extern	void	m_apm_factorial(M_APM, M_APM);
extern	void	m_apm_floor(M_APM, M_APM);
extern	void	m_apm_ceil(M_APM, M_APM);
extern	void	m_apm_get_random(M_APM);
extern	void	m_apm_set_random_seed(char *);

extern	void	m_apm_sqrt(M_APM, int, M_APM);
extern	void	m_apm_cbrt(M_APM, int, M_APM);
extern	void	m_apm_log(M_APM, int, M_APM);
extern	void	m_apm_log10(M_APM, int, M_APM);
extern	void	m_apm_exp(M_APM, int, M_APM);
extern	void	m_apm_pow(M_APM, int, M_APM, M_APM);
extern  void	m_apm_integer_pow(M_APM, int, M_APM, int);
extern  void	m_apm_integer_pow_nr(M_APM, M_APM, int);

extern	void	m_apm_sin_cos(M_APM, M_APM, int, M_APM);
extern	void	m_apm_sin(M_APM, int, M_APM);
extern	void	m_apm_cos(M_APM, int, M_APM);
extern	void	m_apm_tan(M_APM, int, M_APM);
extern	void	m_apm_arcsin(M_APM, int, M_APM);
extern	void	m_apm_arccos(M_APM, int, M_APM);
extern	void	m_apm_arctan(M_APM, int, M_APM);
extern	void	m_apm_arctan2(M_APM, int, M_APM, M_APM);

extern  void    m_apm_sinh(M_APM, int, M_APM);
extern  void    m_apm_cosh(M_APM, int, M_APM);
extern  void    m_apm_tanh(M_APM, int, M_APM);
extern  void    m_apm_arcsinh(M_APM, int, M_APM);
extern  void    m_apm_arccosh(M_APM, int, M_APM);
extern  void    m_apm_arctanh(M_APM, int, M_APM);

extern  void    m_apm_cpp_precision(int);   /* only for C++ wrapper */

/* more intuitive alternate names for the ARC functions ... */

#define m_apm_asin m_apm_arcsin
#define m_apm_acos m_apm_arccos
#define m_apm_atan m_apm_arctan
#define m_apm_atan2 m_apm_arctan2

#define m_apm_asinh m_apm_arcsinh
#define m_apm_acosh m_apm_arccosh
#define m_apm_atanh m_apm_arctanh

#ifdef APM_CONVERT_FROM_C
}      /* End extern "C" bracket */
#endif

#ifdef __cplusplus   /*<- Hides the class below from C compilers */

/*
    This class lets you use M_APM's a bit more intuitively with
    C++'s operator and function overloading, constructors, etc.

    Added 3/24/2000 by Orion Sky Lawlor, olawlor@acm.org
*/

extern 
#ifdef APM_CONVERT_FROM_C
"C" 
#endif
int MM_cpp_min_precision;


class MAPM {
protected:

/*
The M_APM structure here is implemented as a reference-
counted, copy-on-write data structure-- this makes copies
very fast, but that's why it's so ugly.  A MAPM object is 
basically just a wrapper around a (possibly shared) 
M_APM_struct myVal.
*/


	M_APM myVal;  /* My M_APM structure */
	void create(void) {myVal=makeNew();}
	void destroy(void) {unref(myVal);myVal=NULL;}
	void copyFrom(M_APM Nval) 
	{
		 M_APM oldVal=myVal;
		 myVal=Nval;
		 ref(myVal);
		 unref(oldVal);
	}
	static M_APM makeNew(void) 
	{
		M_APM val=m_apm_init();
		/* refcount initialized to 1 by 'm_apm_init' */
		return val;
	}
	static void ref(M_APM val) 
	{
		val->m_apm_refcount++;
	}
	static void unref(M_APM val) 
	{
		val->m_apm_refcount--;
		if (val->m_apm_refcount==0)
			m_apm_free(val);
	}
	
	/* This routine is called to get a private (mutable)
	   copy of our current value. */
	M_APM val(void) 
	{
		if (myVal->m_apm_refcount==1) 
		/* Return my private myVal */
			return myVal;

		/* Otherwise, our copy of myVal is shared--
		   we need to make a new private copy.
                */
		M_APM oldVal=myVal;
		myVal=makeNew();
		m_apm_copy(myVal,oldVal);
		unref(oldVal);
		return myVal;
	}
	
	/*BAD: C M_APM routines doesn't use "const" where they should--
	  hence we have to cast to a non-const type here (FIX THIS!).

	  (in due time.... MCR)
	*/
	M_APM cval(void) const 
	{
		return (M_APM)myVal;
	}
	/* This is the default number of digits to use for 
	   1-ary functions like sin, cos, tan, etc.
	   It's the larger of my digits and cpp_min_precision.
        */
	int myDigits(void) const 
	{
		int maxd=m_apm_significant_digits(cval());
		if (maxd<MM_cpp_min_precision) maxd=MM_cpp_min_precision;
		return maxd;
	}
	/* This is the default number of digits to use for 
	   2-ary functions like divide, atan2, etc.
	   It's the larger of my digits, his digits, and cpp_min_precision.
        */
	int digits(const MAPM &otherVal) const 
	{
		int maxd=myDigits();
		int his=m_apm_significant_digits(otherVal.cval());
		if (maxd<his) maxd=his;
		return maxd;
	}
public:
	/* Constructors: */
	MAPM(void) /* Default constructor (takes no value) */
		{create();}
	MAPM(const MAPM &m) /* Copy constructor */
		{myVal=(M_APM)m.cval();ref(myVal);}
	MAPM(M_APM m) /* M_APM constructor (refcount starts at one) */
		{myVal=(M_APM)m;ref(myVal);}
	MAPM(const char *s) /* Constructor from string */
		{create();m_apm_set_string(val(),(char *)s);}
	MAPM(double d) /* Constructor from double-precision float */
		{create();m_apm_set_double(val(),d);}
	MAPM(int l) /* Constructor from int */
		{create();m_apm_set_long(val(),l);}
	MAPM(long l) /* Constructor from long int */
		{create();m_apm_set_long(val(),l);}
	/* Destructor */
	~MAPM() {destroy();}
	
	/* Extracting string descriptions: */
	void toString(char *dest,int decimalPlaces) const
		{m_apm_to_string(dest,decimalPlaces,cval());}
	void toFixPtString(char *dest,int decimalPlaces) const
		{m_apm_to_fixpt_string(dest,decimalPlaces,cval());}
	void toFixPtStringEx(char *dest,int dp,char a,char b,int c) const
		{m_apm_to_fixpt_stringex(dest,dp,cval(),a,b,c);}
	char *toFixPtStringExp(int dp,char a,char b,int c) const
		{return(m_apm_to_fixpt_stringexp(dp,cval(),a,b,c));}
	void toIntegerString(char *dest) const
		{m_apm_to_integer_string(dest,cval());}
	
	/* Basic operators: */
	MAPM &operator=(const MAPM &m) /* Assigment operator */
		{copyFrom((M_APM)m.cval());return *this;}
	MAPM &operator=(const char *s) /* Assigment operator */
		{m_apm_set_string(val(),(char *)s);return *this;}
	MAPM &operator=(double d) /* Assigment operator */
		{m_apm_set_double(val(),d);return *this;}
	MAPM &operator=(int l) /* Assigment operator */
		{m_apm_set_long(val(),l);return *this;}
	MAPM &operator=(long l) /* Assigment operator */
		{m_apm_set_long(val(),l);return *this;}
	MAPM operator++() /* Prefix increment operator */
		{return *this = *this+MM_One;}
	MAPM operator--() /* Prefix decrement operator */
		{return *this = *this-MM_One;}
	const MAPM operator++(int)  /* Postfix increment operator */
	{
		MAPM old = *this;
		++(*this);          /* Call prefix increment */
		return old;
	}
	const MAPM operator--(int)  /* Postfix decrement operator */
	{
		MAPM old = *this;
		--(*this);          /* Call prefix decrement */
		return old;
	}
	
	/* Comparison operators */
	int operator==(const MAPM &m) const /* Equality operator */
	 {return m_apm_compare(cval(),m.cval())==0;}
	int operator!=(const MAPM &m) const /* Inequality operator */
	 {return m_apm_compare(cval(),m.cval())!=0;}
	int operator<(const MAPM &m) const
	 {return m_apm_compare(cval(),m.cval())<0;}
	int operator<=(const MAPM &m) const
	 {return m_apm_compare(cval(),m.cval())<=0;}
	int operator>(const MAPM &m) const
	 {return m_apm_compare(cval(),m.cval())>0;}
	int operator>=(const MAPM &m) const
	 {return m_apm_compare(cval(),m.cval())>=0;}
	
	/* Basic arithmetic operators */
	friend MAPM operator+(const MAPM &a,const MAPM &b)
	 {MAPM ret;m_apm_add(ret.val(),a.cval(),b.cval());return ret;}
	friend MAPM operator-(const MAPM &a,const MAPM &b)
	 {MAPM ret;m_apm_subtract(ret.val(),a.cval(),b.cval());return ret;}
	friend MAPM operator*(const MAPM &a,const MAPM &b)
	 {MAPM ret;m_apm_multiply(ret.val(),a.cval(),b.cval());return ret;}
	friend MAPM operator%(const MAPM &a,const MAPM &b)
	 {MAPM quot,ret;m_apm_integer_div_rem(quot.val(),ret.val(),
		a.cval(),b.cval());return ret;}

	/* Default division keeps larger of cpp_min_precision, numerator 
	   digits of precision, or denominator digits of precision. */
	friend MAPM operator/(const MAPM &a,const MAPM &b) 
		{return a.divide(b,a.digits(b));}
	
	MAPM divide(const MAPM &m,int toDigits) const
        	{MAPM ret;m_apm_divide(ret.val(),toDigits,cval(),
					        m.cval());return ret;}
	MAPM divide(const MAPM &m) const {return divide(m,digits(m));}
	
	/* Assignment arithmetic operators */
	MAPM &operator+=(const MAPM &m) {*this = *this+m;return *this;}
	MAPM &operator-=(const MAPM &m) {*this = *this-m;return *this;}
	MAPM &operator*=(const MAPM &m) {*this = *this*m;return *this;}
	MAPM &operator/=(const MAPM &m) {*this = *this/m;return *this;}
	MAPM &operator%=(const MAPM &m) {*this = *this%m;return *this;}
	
	/* Extracting/setting simple information: */
	int sign(void) const
		{return m_apm_sign(cval());}
	int exponent(void) const 
		{return m_apm_exponent(cval());}
	int significant_digits(void) const 
		{return m_apm_significant_digits(cval());}
	int is_integer(void) const 
		{return m_apm_is_integer(cval());}
	int is_even(void) const 
		{return m_apm_is_even(cval());}
	int is_odd(void) const 
		{return m_apm_is_odd(cval());}

	/* Functions: */
	MAPM abs(void) const
		{MAPM ret;m_apm_absolute_value(ret.val(),cval());return ret;}
	MAPM neg(void) const
		{MAPM ret;m_apm_negate(ret.val(),cval());return ret;}
	MAPM round(int toDigits) const
		{MAPM ret;m_apm_round(ret.val(),toDigits,cval());return ret;}
	MAPM operator-(void) const {return neg();}

/* I got tired of typing the various declarations for a simple 
   1-ary real-to-real function on MAPM's; hence this define:
   The digits-free versions return my digits of precision or 
   cpp_min_precision, whichever is bigger.
*/

#define MAPM_1aryFunc(func) \
	MAPM func(int toDigits) const\
		{MAPM ret;m_apm_##func(ret.val(),toDigits,cval());return ret;}\
	MAPM func(void) const {return func(myDigits());}

	MAPM_1aryFunc(sqrt)
	MAPM_1aryFunc(cbrt)
	MAPM_1aryFunc(log)
	MAPM_1aryFunc(exp)
	MAPM_1aryFunc(log10)
	MAPM_1aryFunc(sin)
	MAPM_1aryFunc(asin)
	MAPM_1aryFunc(cos)
	MAPM_1aryFunc(acos)
	MAPM_1aryFunc(tan)
	MAPM_1aryFunc(atan)
	MAPM_1aryFunc(sinh)
	MAPM_1aryFunc(asinh)
	MAPM_1aryFunc(cosh)
	MAPM_1aryFunc(acosh)
	MAPM_1aryFunc(tanh)
	MAPM_1aryFunc(atanh)
#undef MAPM_1aryFunc
	
	void sincos(MAPM &sinR,MAPM &cosR,int toDigits)
		{m_apm_sin_cos(sinR.val(),cosR.val(),toDigits,cval());}
	void sincos(MAPM &sinR,MAPM &cosR)
		{sincos(sinR,cosR,myDigits());}
	MAPM pow(const MAPM &m,int toDigits) const
		{MAPM ret;m_apm_pow(ret.val(),toDigits,cval(),
					  m.cval());return ret;}
	MAPM pow(const MAPM &m) const {return pow(m,digits(m));}
	MAPM atan2(const MAPM &x,int toDigits) const
		{MAPM ret;m_apm_arctan2(ret.val(),toDigits,cval(),
					    x.cval());return ret;}
	MAPM atan2(const MAPM &x) const
		{return atan2(x,digits(x));}

	MAPM gcd(const MAPM &m) const
		{MAPM ret;m_apm_gcd(ret.val(),cval(),m.cval());return ret;}

	MAPM lcm(const MAPM &m) const
		{MAPM ret;m_apm_lcm(ret.val(),cval(),m.cval());return ret;}

	static MAPM random(void) 
		{MAPM ret;m_apm_get_random(ret.val());return ret;}

	MAPM floor(void) const
		{MAPM ret;m_apm_floor(ret.val(),cval());return ret;}
	MAPM ceil(void) const
		{MAPM ret;m_apm_ceil(ret.val(),cval());return ret;}

	/* Functions defined only on integers: */
	MAPM factorial(void) const
		{MAPM ret;m_apm_factorial(ret.val(),cval());return ret;}
	MAPM ipow_nr(int p) const
		{MAPM ret;m_apm_integer_pow_nr(ret.val(),
				cval(),p);return ret;}
	MAPM ipow(int p,int toDigits) const
		{MAPM ret;m_apm_integer_pow(ret.val(),
				toDigits,cval(),p);return ret;}
	MAPM ipow(int p) const
		{return ipow(p,myDigits());}
	MAPM integer_divide(const MAPM &denom) const
		{MAPM ret;m_apm_integer_divide(ret.val(),cval(),
		                       denom.cval());return ret;}
	void integer_div_rem(const MAPM &denom,MAPM &quot,MAPM &rem) const
		{m_apm_integer_div_rem(quot.val(),rem.val(),cval(),
					             denom.cval());}
	MAPM div(const MAPM &denom) const {return integer_divide(denom);}
	MAPM rem(const MAPM &denom) const {MAPM ret,ignored;
		integer_div_rem(denom,ignored,ret);return ret;}
};

/* math.h-style functions: */

inline MAPM fabs(const MAPM &m) {return m.abs();}
inline MAPM factorial(const MAPM &m) {return m.factorial();}
inline MAPM floor(const MAPM &m) {return m.floor();}
inline MAPM ceil(const MAPM &m) {return m.ceil();}
inline MAPM get_random(void) {return MAPM::random();}

/* I got tired of typing the various declarations for a simple 
   1-ary real-to-real function on MAPM's; hence this define:
*/
#define MAPM_1aryFunc(func) \
	inline MAPM func(const MAPM &m) {return m.func();} \
	inline MAPM func(const MAPM &m,int toDigits) {return m.func(toDigits);}

/* Define a big block of simple functions: */
	MAPM_1aryFunc(sqrt)
	MAPM_1aryFunc(cbrt)
	MAPM_1aryFunc(log)
	MAPM_1aryFunc(exp)
	MAPM_1aryFunc(log10)
	MAPM_1aryFunc(sin)
	MAPM_1aryFunc(asin)
	MAPM_1aryFunc(cos)
	MAPM_1aryFunc(acos)
	MAPM_1aryFunc(tan)
	MAPM_1aryFunc(atan)
	MAPM_1aryFunc(sinh)
	MAPM_1aryFunc(asinh)
	MAPM_1aryFunc(cosh)
	MAPM_1aryFunc(acosh)
	MAPM_1aryFunc(tanh)
	MAPM_1aryFunc(atanh)
#undef MAPM_1aryFunc

/* Computes x to the power y */
inline MAPM pow(const MAPM &x,const MAPM &y,int toDigits)
		{return x.pow(y,toDigits);}
inline MAPM pow(const MAPM &x,const MAPM &y)
		{return x.pow(y);}
inline MAPM atan2(const MAPM &y,const MAPM &x,int toDigits)
		{return y.atan2(x,toDigits);}
inline MAPM atan2(const MAPM &y,const MAPM &x)
		{return y.atan2(x);}
inline MAPM gcd(const MAPM &u,const MAPM &v)
		{return u.gcd(v);}
inline MAPM lcm(const MAPM &u,const MAPM &v)
		{return u.lcm(v);}
#endif
#endif


/* Generated automatically by ./hpgsmkrop at Mon Mar 19 17:26:14 2007.
   Do not edit!
 */
#include <hpgs.h>

/* 0 source/pattern opaque. */
static void rop3_0_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 0;
  *D = stk1;
}

/* 0 source opaque/pattern transparent. */
static void rop3_0_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 0;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* 0 source transparent/pattern opaque. */
static void rop3_0_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 0;
  *D = (stk1 & (~S)) | (*D & S);
}

/* 0 source/pattern transparent. */
static void rop3_0_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 0;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* 0 source/pattern opaque. */
static unsigned xrop3_0_0_0 (unsigned char s, unsigned char t)
{
  unsigned stk1;
  stk1 = 0x0000;
  return stk1;
}

/* 0 source opaque/pattern transparent. */
static unsigned xrop3_0_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = 0x0000;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* 0 source transparent/pattern opaque. */
static unsigned xrop3_0_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = 0x0000;
  return (stk1 & (~S)) | (D & S);
}

/* 0 source/pattern transparent. */
static unsigned xrop3_0_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = 0x0000;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSoon source/pattern opaque. */
static void rop3_1_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSoon source opaque/pattern transparent. */
static void rop3_1_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSoon source transparent/pattern opaque. */
static void rop3_1_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSoon source/pattern transparent. */
static void rop3_1_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSoon source/pattern opaque. */
static unsigned xrop3_1_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSoon source opaque/pattern transparent. */
static unsigned xrop3_1_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSoon source transparent/pattern opaque. */
static unsigned xrop3_1_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSoon source/pattern transparent. */
static unsigned xrop3_1_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSona source/pattern opaque. */
static void rop3_2_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSona source opaque/pattern transparent. */
static void rop3_2_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSona source transparent/pattern opaque. */
static void rop3_2_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSona source/pattern transparent. */
static void rop3_2_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSona source/pattern opaque. */
static unsigned xrop3_2_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return stk1;
}

/* DTSona source opaque/pattern transparent. */
static unsigned xrop3_2_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSona source transparent/pattern opaque. */
static unsigned xrop3_2_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSona source/pattern transparent. */
static unsigned xrop3_2_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSon source/pattern opaque. */
static void rop3_3_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSon source opaque/pattern transparent. */
static void rop3_3_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSon source transparent/pattern opaque. */
static void rop3_3_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSon source/pattern transparent. */
static void rop3_3_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSon source/pattern opaque. */
static unsigned xrop3_3_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  return stk1;
}

/* TSon source opaque/pattern transparent. */
static unsigned xrop3_3_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSon source transparent/pattern opaque. */
static unsigned xrop3_3_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSon source/pattern transparent. */
static unsigned xrop3_3_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTona source/pattern opaque. */
static void rop3_4_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDTona source opaque/pattern transparent. */
static void rop3_4_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTona source transparent/pattern opaque. */
static void rop3_4_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTona source/pattern transparent. */
static void rop3_4_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTona source/pattern opaque. */
static unsigned xrop3_4_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return stk1;
}

/* SDTona source opaque/pattern transparent. */
static unsigned xrop3_4_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTona source transparent/pattern opaque. */
static unsigned xrop3_4_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTona source/pattern transparent. */
static unsigned xrop3_4_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTon source/pattern opaque. */
static void rop3_5_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTon source opaque/pattern transparent. */
static void rop3_5_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTon source transparent/pattern opaque. */
static void rop3_5_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTon source/pattern transparent. */
static void rop3_5_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTon source/pattern opaque. */
static unsigned xrop3_5_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  stk1 = ~stk1;
  return stk1;
}

/* DTon source opaque/pattern transparent. */
static unsigned xrop3_5_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTon source transparent/pattern opaque. */
static unsigned xrop3_5_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTon source/pattern transparent. */
static unsigned xrop3_5_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxnon source/pattern opaque. */
static void rop3_6_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSxnon source opaque/pattern transparent. */
static void rop3_6_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxnon source transparent/pattern opaque. */
static void rop3_6_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxnon source/pattern transparent. */
static void rop3_6_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxnon source/pattern opaque. */
static unsigned xrop3_6_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSxnon source opaque/pattern transparent. */
static unsigned xrop3_6_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxnon source transparent/pattern opaque. */
static unsigned xrop3_6_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxnon source/pattern transparent. */
static unsigned xrop3_6_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSaon source/pattern opaque. */
static void rop3_7_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSaon source opaque/pattern transparent. */
static void rop3_7_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSaon source transparent/pattern opaque. */
static void rop3_7_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSaon source/pattern transparent. */
static void rop3_7_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSaon source/pattern opaque. */
static unsigned xrop3_7_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSaon source opaque/pattern transparent. */
static unsigned xrop3_7_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSaon source transparent/pattern opaque. */
static unsigned xrop3_7_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSaon source/pattern transparent. */
static unsigned xrop3_7_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnaa source/pattern opaque. */
static void rop3_8_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDTnaa source opaque/pattern transparent. */
static void rop3_8_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnaa source transparent/pattern opaque. */
static void rop3_8_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnaa source/pattern transparent. */
static void rop3_8_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnaa source/pattern opaque. */
static unsigned xrop3_8_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S & stk2;
  return stk1;
}

/* SDTnaa source opaque/pattern transparent. */
static unsigned xrop3_8_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnaa source transparent/pattern opaque. */
static unsigned xrop3_8_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnaa source/pattern transparent. */
static unsigned xrop3_8_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxon source/pattern opaque. */
static void rop3_9_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSxon source opaque/pattern transparent. */
static void rop3_9_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxon source transparent/pattern opaque. */
static void rop3_9_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxon source/pattern transparent. */
static void rop3_9_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxon source/pattern opaque. */
static unsigned xrop3_9_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSxon source opaque/pattern transparent. */
static unsigned xrop3_9_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxon source transparent/pattern opaque. */
static unsigned xrop3_9_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxon source/pattern transparent. */
static unsigned xrop3_9_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTna source/pattern opaque. */
static void rop3_10_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTna source opaque/pattern transparent. */
static void rop3_10_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTna source transparent/pattern opaque. */
static void rop3_10_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTna source/pattern transparent. */
static void rop3_10_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTna source/pattern opaque. */
static unsigned xrop3_10_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D & stk2;
  return stk1;
}

/* DTna source opaque/pattern transparent. */
static unsigned xrop3_10_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTna source transparent/pattern opaque. */
static unsigned xrop3_10_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTna source/pattern transparent. */
static unsigned xrop3_10_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnaon source/pattern opaque. */
static void rop3_11_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDnaon source opaque/pattern transparent. */
static void rop3_11_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnaon source transparent/pattern opaque. */
static void rop3_11_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnaon source/pattern transparent. */
static void rop3_11_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnaon source/pattern opaque. */
static unsigned xrop3_11_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDnaon source opaque/pattern transparent. */
static unsigned xrop3_11_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnaon source transparent/pattern opaque. */
static unsigned xrop3_11_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnaon source/pattern transparent. */
static unsigned xrop3_11_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STna source/pattern opaque. */
static void rop3_12_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  *D = stk1;
}

/* STna source opaque/pattern transparent. */
static void rop3_12_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STna source transparent/pattern opaque. */
static void rop3_12_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STna source/pattern transparent. */
static void rop3_12_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STna source/pattern opaque. */
static unsigned xrop3_12_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  return stk1;
}

/* STna source opaque/pattern transparent. */
static unsigned xrop3_12_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STna source transparent/pattern opaque. */
static unsigned xrop3_12_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STna source/pattern transparent. */
static unsigned xrop3_12_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSnaon source/pattern opaque. */
static void rop3_13_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSnaon source opaque/pattern transparent. */
static void rop3_13_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSnaon source transparent/pattern opaque. */
static void rop3_13_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSnaon source/pattern transparent. */
static void rop3_13_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSnaon source/pattern opaque. */
static unsigned xrop3_13_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSnaon source opaque/pattern transparent. */
static unsigned xrop3_13_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSnaon source transparent/pattern opaque. */
static unsigned xrop3_13_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSnaon source/pattern transparent. */
static unsigned xrop3_13_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSonon source/pattern opaque. */
static void rop3_14_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSonon source opaque/pattern transparent. */
static void rop3_14_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSonon source transparent/pattern opaque. */
static void rop3_14_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSonon source/pattern transparent. */
static void rop3_14_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSonon source/pattern opaque. */
static unsigned xrop3_14_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSonon source opaque/pattern transparent. */
static unsigned xrop3_14_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSonon source transparent/pattern opaque. */
static unsigned xrop3_14_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSonon source/pattern transparent. */
static unsigned xrop3_14_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* Tn source/pattern opaque. */
static void rop3_15_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~T;
  *D = stk1;
}

/* Tn source opaque/pattern transparent. */
static void rop3_15_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~T;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* Tn source transparent/pattern opaque. */
static void rop3_15_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~T;
  *D = (stk1 & (~S)) | (*D & S);
}

/* Tn source/pattern transparent. */
static void rop3_15_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~T;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* Tn source/pattern opaque. */
static unsigned xrop3_15_0_0 (unsigned char s, unsigned char t)
{
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~T;
  return stk1;
}

/* Tn source opaque/pattern transparent. */
static unsigned xrop3_15_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~T;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* Tn source transparent/pattern opaque. */
static unsigned xrop3_15_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~T;
  return (stk1 & (~S)) | (D & S);
}

/* Tn source/pattern transparent. */
static unsigned xrop3_15_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~T;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSona source/pattern opaque. */
static void rop3_16_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDSona source opaque/pattern transparent. */
static void rop3_16_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSona source transparent/pattern opaque. */
static void rop3_16_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSona source/pattern transparent. */
static void rop3_16_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSona source/pattern opaque. */
static unsigned xrop3_16_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return stk1;
}

/* TDSona source opaque/pattern transparent. */
static unsigned xrop3_16_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSona source transparent/pattern opaque. */
static unsigned xrop3_16_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSona source/pattern transparent. */
static unsigned xrop3_16_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSon source/pattern opaque. */
static void rop3_17_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSon source opaque/pattern transparent. */
static void rop3_17_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSon source transparent/pattern opaque. */
static void rop3_17_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSon source/pattern transparent. */
static void rop3_17_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSon source/pattern opaque. */
static unsigned xrop3_17_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D | S;
  stk1 = ~stk1;
  return stk1;
}

/* DSon source opaque/pattern transparent. */
static unsigned xrop3_17_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | S;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSon source transparent/pattern opaque. */
static unsigned xrop3_17_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D | S;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSon source/pattern transparent. */
static unsigned xrop3_17_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | S;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxnon source/pattern opaque. */
static void rop3_18_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTxnon source opaque/pattern transparent. */
static void rop3_18_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxnon source transparent/pattern opaque. */
static void rop3_18_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxnon source/pattern transparent. */
static void rop3_18_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxnon source/pattern opaque. */
static unsigned xrop3_18_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTxnon source opaque/pattern transparent. */
static unsigned xrop3_18_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxnon source transparent/pattern opaque. */
static unsigned xrop3_18_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxnon source/pattern transparent. */
static unsigned xrop3_18_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTaon source/pattern opaque. */
static void rop3_19_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTaon source opaque/pattern transparent. */
static void rop3_19_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTaon source transparent/pattern opaque. */
static void rop3_19_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTaon source/pattern transparent. */
static void rop3_19_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTaon source/pattern opaque. */
static unsigned xrop3_19_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTaon source opaque/pattern transparent. */
static unsigned xrop3_19_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTaon source transparent/pattern opaque. */
static unsigned xrop3_19_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTaon source/pattern transparent. */
static unsigned xrop3_19_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxnon source/pattern opaque. */
static void rop3_20_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSxnon source opaque/pattern transparent. */
static void rop3_20_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxnon source transparent/pattern opaque. */
static void rop3_20_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxnon source/pattern transparent. */
static void rop3_20_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxnon source/pattern opaque. */
static unsigned xrop3_20_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSxnon source opaque/pattern transparent. */
static unsigned xrop3_20_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxnon source transparent/pattern opaque. */
static unsigned xrop3_20_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxnon source/pattern transparent. */
static unsigned xrop3_20_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSaon source/pattern opaque. */
static void rop3_21_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSaon source opaque/pattern transparent. */
static void rop3_21_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSaon source transparent/pattern opaque. */
static void rop3_21_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSaon source/pattern transparent. */
static void rop3_21_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSaon source/pattern opaque. */
static unsigned xrop3_21_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSaon source opaque/pattern transparent. */
static unsigned xrop3_21_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSaon source transparent/pattern opaque. */
static unsigned xrop3_21_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSaon source/pattern transparent. */
static unsigned xrop3_21_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTSanaxx source/pattern opaque. */
static void rop3_22_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTSanaxx source opaque/pattern transparent. */
static void rop3_22_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTSanaxx source transparent/pattern opaque. */
static void rop3_22_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTSanaxx source/pattern transparent. */
static void rop3_22_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTSanaxx source/pattern opaque. */
static unsigned xrop3_22_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTSanaxx source opaque/pattern transparent. */
static unsigned xrop3_22_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTSanaxx source transparent/pattern opaque. */
static unsigned xrop3_22_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTSanaxx source/pattern transparent. */
static unsigned xrop3_22_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk4 = ~stk4;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSTxDSxaxn source/pattern opaque. */
static void rop3_23_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SSTxDSxaxn source opaque/pattern transparent. */
static void rop3_23_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSTxDSxaxn source transparent/pattern opaque. */
static void rop3_23_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSTxDSxaxn source/pattern transparent. */
static void rop3_23_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSTxDSxaxn source/pattern opaque. */
static unsigned xrop3_23_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SSTxDSxaxn source opaque/pattern transparent. */
static unsigned xrop3_23_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSTxDSxaxn source transparent/pattern opaque. */
static unsigned xrop3_23_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SSTxDSxaxn source/pattern transparent. */
static unsigned xrop3_23_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STxTDxa source/pattern opaque. */
static void rop3_24_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = stk1;
}

/* STxTDxa source opaque/pattern transparent. */
static void rop3_24_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STxTDxa source transparent/pattern opaque. */
static void rop3_24_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STxTDxa source/pattern transparent. */
static void rop3_24_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STxTDxa source/pattern opaque. */
static unsigned xrop3_24_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return stk1;
}

/* STxTDxa source opaque/pattern transparent. */
static unsigned xrop3_24_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STxTDxa source transparent/pattern opaque. */
static unsigned xrop3_24_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STxTDxa source/pattern transparent. */
static unsigned xrop3_24_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSanaxn source/pattern opaque. */
static void rop3_25_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSanaxn source opaque/pattern transparent. */
static void rop3_25_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSanaxn source transparent/pattern opaque. */
static void rop3_25_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSanaxn source/pattern transparent. */
static void rop3_25_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSanaxn source/pattern opaque. */
static unsigned xrop3_25_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSanaxn source opaque/pattern transparent. */
static unsigned xrop3_25_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSanaxn source transparent/pattern opaque. */
static unsigned xrop3_25_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSanaxn source/pattern transparent. */
static unsigned xrop3_25_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTaox source/pattern opaque. */
static void rop3_26_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSTaox source opaque/pattern transparent. */
static void rop3_26_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTaox source transparent/pattern opaque. */
static void rop3_26_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTaox source/pattern transparent. */
static void rop3_26_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTaox source/pattern opaque. */
static unsigned xrop3_26_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSTaox source opaque/pattern transparent. */
static unsigned xrop3_26_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTaox source transparent/pattern opaque. */
static unsigned xrop3_26_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTaox source/pattern transparent. */
static unsigned xrop3_26_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSxaxn source/pattern opaque. */
static void rop3_27_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSxaxn source opaque/pattern transparent. */
static void rop3_27_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSxaxn source transparent/pattern opaque. */
static void rop3_27_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSxaxn source/pattern transparent. */
static void rop3_27_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSxaxn source/pattern opaque. */
static unsigned xrop3_27_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSxaxn source opaque/pattern transparent. */
static unsigned xrop3_27_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSxaxn source transparent/pattern opaque. */
static unsigned xrop3_27_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSxaxn source/pattern transparent. */
static unsigned xrop3_27_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTaox source/pattern opaque. */
static void rop3_28_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTaox source opaque/pattern transparent. */
static void rop3_28_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTaox source transparent/pattern opaque. */
static void rop3_28_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTaox source/pattern transparent. */
static void rop3_28_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTaox source/pattern opaque. */
static unsigned xrop3_28_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTaox source opaque/pattern transparent. */
static unsigned xrop3_28_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTaox source transparent/pattern opaque. */
static unsigned xrop3_28_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTaox source/pattern transparent. */
static unsigned xrop3_28_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDxaxn source/pattern opaque. */
static void rop3_29_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDxaxn source opaque/pattern transparent. */
static void rop3_29_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDxaxn source transparent/pattern opaque. */
static void rop3_29_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDxaxn source/pattern transparent. */
static void rop3_29_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDxaxn source/pattern opaque. */
static unsigned xrop3_29_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDxaxn source opaque/pattern transparent. */
static unsigned xrop3_29_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDxaxn source transparent/pattern opaque. */
static unsigned xrop3_29_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDxaxn source/pattern transparent. */
static unsigned xrop3_29_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSox source/pattern opaque. */
static void rop3_30_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSox source opaque/pattern transparent. */
static void rop3_30_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSox source transparent/pattern opaque. */
static void rop3_30_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSox source/pattern transparent. */
static void rop3_30_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSox source/pattern opaque. */
static unsigned xrop3_30_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSox source opaque/pattern transparent. */
static unsigned xrop3_30_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSox source transparent/pattern opaque. */
static unsigned xrop3_30_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSox source/pattern transparent. */
static unsigned xrop3_30_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSoan source/pattern opaque. */
static void rop3_31_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSoan source opaque/pattern transparent. */
static void rop3_31_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSoan source transparent/pattern opaque. */
static void rop3_31_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSoan source/pattern transparent. */
static void rop3_31_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSoan source/pattern opaque. */
static unsigned xrop3_31_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSoan source opaque/pattern transparent. */
static unsigned xrop3_31_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSoan source transparent/pattern opaque. */
static unsigned xrop3_31_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSoan source/pattern transparent. */
static unsigned xrop3_31_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnaa source/pattern opaque. */
static void rop3_32_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSnaa source opaque/pattern transparent. */
static void rop3_32_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnaa source transparent/pattern opaque. */
static void rop3_32_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnaa source/pattern transparent. */
static void rop3_32_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnaa source/pattern opaque. */
static unsigned xrop3_32_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D & stk2;
  return stk1;
}

/* DTSnaa source opaque/pattern transparent. */
static unsigned xrop3_32_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnaa source transparent/pattern opaque. */
static unsigned xrop3_32_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnaa source/pattern transparent. */
static unsigned xrop3_32_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxon source/pattern opaque. */
static void rop3_33_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTxon source opaque/pattern transparent. */
static void rop3_33_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxon source transparent/pattern opaque. */
static void rop3_33_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxon source/pattern transparent. */
static void rop3_33_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxon source/pattern opaque. */
static unsigned xrop3_33_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTxon source opaque/pattern transparent. */
static unsigned xrop3_33_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxon source transparent/pattern opaque. */
static unsigned xrop3_33_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxon source/pattern transparent. */
static unsigned xrop3_33_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSna source/pattern opaque. */
static void rop3_34_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DSna source opaque/pattern transparent. */
static void rop3_34_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSna source transparent/pattern opaque. */
static void rop3_34_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSna source/pattern transparent. */
static void rop3_34_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSna source/pattern opaque. */
static unsigned xrop3_34_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D & stk2;
  return stk1;
}

/* DSna source opaque/pattern transparent. */
static unsigned xrop3_34_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSna source transparent/pattern opaque. */
static unsigned xrop3_34_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSna source/pattern transparent. */
static unsigned xrop3_34_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDnaon source/pattern opaque. */
static void rop3_35_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDnaon source opaque/pattern transparent. */
static void rop3_35_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDnaon source transparent/pattern opaque. */
static void rop3_35_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDnaon source/pattern transparent. */
static void rop3_35_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDnaon source/pattern opaque. */
static unsigned xrop3_35_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDnaon source opaque/pattern transparent. */
static unsigned xrop3_35_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDnaon source transparent/pattern opaque. */
static unsigned xrop3_35_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDnaon source/pattern transparent. */
static unsigned xrop3_35_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STxDSxa source/pattern opaque. */
static void rop3_36_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  *D = stk1;
}

/* STxDSxa source opaque/pattern transparent. */
static void rop3_36_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STxDSxa source transparent/pattern opaque. */
static void rop3_36_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STxDSxa source/pattern transparent. */
static void rop3_36_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STxDSxa source/pattern opaque. */
static unsigned xrop3_36_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  return stk1;
}

/* STxDSxa source opaque/pattern transparent. */
static unsigned xrop3_36_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STxDSxa source transparent/pattern opaque. */
static unsigned xrop3_36_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STxDSxa source/pattern transparent. */
static unsigned xrop3_36_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTanaxn source/pattern opaque. */
static void rop3_37_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTanaxn source opaque/pattern transparent. */
static void rop3_37_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTanaxn source transparent/pattern opaque. */
static void rop3_37_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTanaxn source/pattern transparent. */
static void rop3_37_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTanaxn source/pattern opaque. */
static unsigned xrop3_37_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTanaxn source opaque/pattern transparent. */
static unsigned xrop3_37_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTanaxn source transparent/pattern opaque. */
static unsigned xrop3_37_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTanaxn source/pattern transparent. */
static unsigned xrop3_37_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSaox source/pattern opaque. */
static void rop3_38_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSaox source opaque/pattern transparent. */
static void rop3_38_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSaox source transparent/pattern opaque. */
static void rop3_38_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSaox source/pattern transparent. */
static void rop3_38_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSaox source/pattern opaque. */
static unsigned xrop3_38_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSaox source opaque/pattern transparent. */
static unsigned xrop3_38_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSaox source transparent/pattern opaque. */
static unsigned xrop3_38_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSaox source/pattern transparent. */
static unsigned xrop3_38_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSxnox source/pattern opaque. */
static void rop3_39_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSxnox source opaque/pattern transparent. */
static void rop3_39_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSxnox source transparent/pattern opaque. */
static void rop3_39_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSxnox source/pattern transparent. */
static void rop3_39_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSxnox source/pattern opaque. */
static unsigned xrop3_39_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSxnox source opaque/pattern transparent. */
static unsigned xrop3_39_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSxnox source transparent/pattern opaque. */
static unsigned xrop3_39_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSxnox source/pattern transparent. */
static unsigned xrop3_39_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxa source/pattern opaque. */
static void rop3_40_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSxa source opaque/pattern transparent. */
static void rop3_40_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxa source transparent/pattern opaque. */
static void rop3_40_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxa source/pattern transparent. */
static void rop3_40_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxa source/pattern opaque. */
static unsigned xrop3_40_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  return stk1;
}

/* DTSxa source opaque/pattern transparent. */
static unsigned xrop3_40_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxa source transparent/pattern opaque. */
static unsigned xrop3_40_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxa source/pattern transparent. */
static unsigned xrop3_40_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTSaoxxn source/pattern opaque. */
static void rop3_41_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDTSaoxxn source opaque/pattern transparent. */
static void rop3_41_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTSaoxxn source transparent/pattern opaque. */
static void rop3_41_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTSaoxxn source/pattern transparent. */
static void rop3_41_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTSaoxxn source/pattern opaque. */
static unsigned xrop3_41_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDTSaoxxn source opaque/pattern transparent. */
static unsigned xrop3_41_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTSaoxxn source transparent/pattern opaque. */
static unsigned xrop3_41_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTSaoxxn source/pattern transparent. */
static unsigned xrop3_41_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSana source/pattern opaque. */
static void rop3_42_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSana source opaque/pattern transparent. */
static void rop3_42_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSana source transparent/pattern opaque. */
static void rop3_42_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSana source/pattern transparent. */
static void rop3_42_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSana source/pattern opaque. */
static unsigned xrop3_42_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return stk1;
}

/* DTSana source opaque/pattern transparent. */
static unsigned xrop3_42_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSana source transparent/pattern opaque. */
static unsigned xrop3_42_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSana source/pattern transparent. */
static unsigned xrop3_42_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSTxTDxaxn source/pattern opaque. */
static void rop3_43_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SSTxTDxaxn source opaque/pattern transparent. */
static void rop3_43_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSTxTDxaxn source transparent/pattern opaque. */
static void rop3_43_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSTxTDxaxn source/pattern transparent. */
static void rop3_43_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSTxTDxaxn source/pattern opaque. */
static unsigned xrop3_43_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SSTxTDxaxn source opaque/pattern transparent. */
static unsigned xrop3_43_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSTxTDxaxn source transparent/pattern opaque. */
static unsigned xrop3_43_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SSTxTDxaxn source/pattern transparent. */
static unsigned xrop3_43_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSoax source/pattern opaque. */
static void rop3_44_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSoax source opaque/pattern transparent. */
static void rop3_44_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSoax source transparent/pattern opaque. */
static void rop3_44_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSoax source/pattern transparent. */
static void rop3_44_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSoax source/pattern opaque. */
static unsigned xrop3_44_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSoax source opaque/pattern transparent. */
static unsigned xrop3_44_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSoax source transparent/pattern opaque. */
static unsigned xrop3_44_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSoax source/pattern transparent. */
static unsigned xrop3_44_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnox source/pattern opaque. */
static void rop3_45_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDnox source opaque/pattern transparent. */
static void rop3_45_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnox source transparent/pattern opaque. */
static void rop3_45_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnox source/pattern transparent. */
static void rop3_45_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnox source/pattern opaque. */
static unsigned xrop3_45_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDnox source opaque/pattern transparent. */
static unsigned xrop3_45_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnox source transparent/pattern opaque. */
static unsigned xrop3_45_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnox source/pattern transparent. */
static unsigned xrop3_45_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTxox source/pattern opaque. */
static void rop3_46_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTxox source opaque/pattern transparent. */
static void rop3_46_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTxox source transparent/pattern opaque. */
static void rop3_46_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTxox source/pattern transparent. */
static void rop3_46_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTxox source/pattern opaque. */
static unsigned xrop3_46_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTxox source opaque/pattern transparent. */
static unsigned xrop3_46_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTxox source transparent/pattern opaque. */
static unsigned xrop3_46_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTxox source/pattern transparent. */
static unsigned xrop3_46_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnoan source/pattern opaque. */
static void rop3_47_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDnoan source opaque/pattern transparent. */
static void rop3_47_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnoan source transparent/pattern opaque. */
static void rop3_47_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnoan source/pattern transparent. */
static void rop3_47_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnoan source/pattern opaque. */
static unsigned xrop3_47_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDnoan source opaque/pattern transparent. */
static unsigned xrop3_47_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnoan source transparent/pattern opaque. */
static unsigned xrop3_47_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnoan source/pattern transparent. */
static unsigned xrop3_47_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSna source/pattern opaque. */
static void rop3_48_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  *D = stk1;
}

/* TSna source opaque/pattern transparent. */
static void rop3_48_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSna source transparent/pattern opaque. */
static void rop3_48_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSna source/pattern transparent. */
static void rop3_48_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSna source/pattern opaque. */
static unsigned xrop3_48_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  return stk1;
}

/* TSna source opaque/pattern transparent. */
static unsigned xrop3_48_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSna source transparent/pattern opaque. */
static unsigned xrop3_48_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSna source/pattern transparent. */
static unsigned xrop3_48_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnaon source/pattern opaque. */
static void rop3_49_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTnaon source opaque/pattern transparent. */
static void rop3_49_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnaon source transparent/pattern opaque. */
static void rop3_49_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnaon source/pattern transparent. */
static void rop3_49_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnaon source/pattern opaque. */
static unsigned xrop3_49_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTnaon source opaque/pattern transparent. */
static unsigned xrop3_49_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnaon source transparent/pattern opaque. */
static unsigned xrop3_49_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnaon source/pattern transparent. */
static unsigned xrop3_49_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSoox source/pattern opaque. */
static void rop3_50_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSoox source opaque/pattern transparent. */
static void rop3_50_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSoox source transparent/pattern opaque. */
static void rop3_50_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSoox source/pattern transparent. */
static void rop3_50_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSoox source/pattern opaque. */
static unsigned xrop3_50_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSoox source opaque/pattern transparent. */
static unsigned xrop3_50_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSoox source transparent/pattern opaque. */
static unsigned xrop3_50_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSoox source/pattern transparent. */
static unsigned xrop3_50_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* Sn source/pattern opaque. */
static void rop3_51_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~S;
  *D = stk1;
}

/* Sn source opaque/pattern transparent. */
static void rop3_51_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* Sn source transparent/pattern opaque. */
static void rop3_51_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* Sn source/pattern transparent. */
static void rop3_51_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* Sn source/pattern opaque. */
static unsigned xrop3_51_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = ~S;
  return stk1;
}

/* Sn source opaque/pattern transparent. */
static unsigned xrop3_51_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* Sn source transparent/pattern opaque. */
static unsigned xrop3_51_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = ~S;
  return (stk1 & (~S)) | (D & S);
}

/* Sn source/pattern transparent. */
static unsigned xrop3_51_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSaox source/pattern opaque. */
static void rop3_52_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSaox source opaque/pattern transparent. */
static void rop3_52_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSaox source transparent/pattern opaque. */
static void rop3_52_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSaox source/pattern transparent. */
static void rop3_52_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSaox source/pattern opaque. */
static unsigned xrop3_52_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSaox source opaque/pattern transparent. */
static unsigned xrop3_52_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSaox source transparent/pattern opaque. */
static unsigned xrop3_52_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSaox source/pattern transparent. */
static unsigned xrop3_52_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSxnox source/pattern opaque. */
static void rop3_53_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSxnox source opaque/pattern transparent. */
static void rop3_53_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSxnox source transparent/pattern opaque. */
static void rop3_53_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSxnox source/pattern transparent. */
static void rop3_53_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSxnox source/pattern opaque. */
static unsigned xrop3_53_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSxnox source opaque/pattern transparent. */
static unsigned xrop3_53_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSxnox source transparent/pattern opaque. */
static unsigned xrop3_53_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSxnox source/pattern transparent. */
static unsigned xrop3_53_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTox source/pattern opaque. */
static void rop3_54_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTox source opaque/pattern transparent. */
static void rop3_54_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTox source transparent/pattern opaque. */
static void rop3_54_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTox source/pattern transparent. */
static void rop3_54_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTox source/pattern opaque. */
static unsigned xrop3_54_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTox source opaque/pattern transparent. */
static unsigned xrop3_54_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTox source transparent/pattern opaque. */
static unsigned xrop3_54_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTox source/pattern transparent. */
static unsigned xrop3_54_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDToan source/pattern opaque. */
static void rop3_55_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDToan source opaque/pattern transparent. */
static void rop3_55_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDToan source transparent/pattern opaque. */
static void rop3_55_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDToan source/pattern transparent. */
static void rop3_55_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDToan source/pattern opaque. */
static unsigned xrop3_55_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDToan source opaque/pattern transparent. */
static unsigned xrop3_55_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDToan source transparent/pattern opaque. */
static unsigned xrop3_55_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDToan source/pattern transparent. */
static unsigned xrop3_55_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDToax source/pattern opaque. */
static void rop3_56_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDToax source opaque/pattern transparent. */
static void rop3_56_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDToax source transparent/pattern opaque. */
static void rop3_56_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDToax source/pattern transparent. */
static void rop3_56_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDToax source/pattern opaque. */
static unsigned xrop3_56_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDToax source opaque/pattern transparent. */
static unsigned xrop3_56_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDToax source transparent/pattern opaque. */
static unsigned xrop3_56_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDToax source/pattern transparent. */
static unsigned xrop3_56_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDnox source/pattern opaque. */
static void rop3_57_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDnox source opaque/pattern transparent. */
static void rop3_57_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDnox source transparent/pattern opaque. */
static void rop3_57_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDnox source/pattern transparent. */
static void rop3_57_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDnox source/pattern opaque. */
static unsigned xrop3_57_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDnox source opaque/pattern transparent. */
static unsigned xrop3_57_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDnox source transparent/pattern opaque. */
static unsigned xrop3_57_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDnox source/pattern transparent. */
static unsigned xrop3_57_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSxox source/pattern opaque. */
static void rop3_58_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSxox source opaque/pattern transparent. */
static void rop3_58_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSxox source transparent/pattern opaque. */
static void rop3_58_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSxox source/pattern transparent. */
static void rop3_58_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSxox source/pattern opaque. */
static unsigned xrop3_58_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSxox source opaque/pattern transparent. */
static unsigned xrop3_58_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSxox source transparent/pattern opaque. */
static unsigned xrop3_58_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSxox source/pattern transparent. */
static unsigned xrop3_58_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDnoan source/pattern opaque. */
static void rop3_59_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDnoan source opaque/pattern transparent. */
static void rop3_59_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDnoan source transparent/pattern opaque. */
static void rop3_59_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDnoan source/pattern transparent. */
static void rop3_59_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDnoan source/pattern opaque. */
static unsigned xrop3_59_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDnoan source opaque/pattern transparent. */
static unsigned xrop3_59_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDnoan source transparent/pattern opaque. */
static unsigned xrop3_59_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDnoan source/pattern transparent. */
static unsigned xrop3_59_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSx source/pattern opaque. */
static void rop3_60_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  *D = stk1;
}

/* TSx source opaque/pattern transparent. */
static void rop3_60_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSx source transparent/pattern opaque. */
static void rop3_60_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSx source/pattern transparent. */
static void rop3_60_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSx source/pattern opaque. */
static unsigned xrop3_60_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  return stk1;
}

/* TSx source opaque/pattern transparent. */
static unsigned xrop3_60_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSx source transparent/pattern opaque. */
static unsigned xrop3_60_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  return (stk1 & (~S)) | (D & S);
}

/* TSx source/pattern transparent. */
static unsigned xrop3_60_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSonox source/pattern opaque. */
static void rop3_61_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSonox source opaque/pattern transparent. */
static void rop3_61_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSonox source transparent/pattern opaque. */
static void rop3_61_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSonox source/pattern transparent. */
static void rop3_61_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSonox source/pattern opaque. */
static unsigned xrop3_61_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSonox source opaque/pattern transparent. */
static unsigned xrop3_61_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSonox source transparent/pattern opaque. */
static unsigned xrop3_61_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSonox source/pattern transparent. */
static unsigned xrop3_61_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSnaox source/pattern opaque. */
static void rop3_62_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSnaox source opaque/pattern transparent. */
static void rop3_62_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSnaox source transparent/pattern opaque. */
static void rop3_62_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSnaox source/pattern transparent. */
static void rop3_62_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSnaox source/pattern opaque. */
static unsigned xrop3_62_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSnaox source opaque/pattern transparent. */
static unsigned xrop3_62_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSnaox source transparent/pattern opaque. */
static unsigned xrop3_62_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSnaox source/pattern transparent. */
static unsigned xrop3_62_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSan source/pattern opaque. */
static void rop3_63_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSan source opaque/pattern transparent. */
static void rop3_63_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSan source transparent/pattern opaque. */
static void rop3_63_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSan source/pattern transparent. */
static void rop3_63_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSan source/pattern opaque. */
static unsigned xrop3_63_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  return stk1;
}

/* TSan source opaque/pattern transparent. */
static unsigned xrop3_63_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSan source transparent/pattern opaque. */
static unsigned xrop3_63_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSan source/pattern transparent. */
static unsigned xrop3_63_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnaa source/pattern opaque. */
static void rop3_64_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  *D = stk1;
}

/* TSDnaa source opaque/pattern transparent. */
static void rop3_64_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnaa source transparent/pattern opaque. */
static void rop3_64_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnaa source/pattern transparent. */
static void rop3_64_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnaa source/pattern opaque. */
static unsigned xrop3_64_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  return stk1;
}

/* TSDnaa source opaque/pattern transparent. */
static unsigned xrop3_64_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnaa source transparent/pattern opaque. */
static unsigned xrop3_64_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnaa source/pattern transparent. */
static unsigned xrop3_64_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxon source/pattern opaque. */
static void rop3_65_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSxon source opaque/pattern transparent. */
static void rop3_65_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxon source transparent/pattern opaque. */
static void rop3_65_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxon source/pattern transparent. */
static void rop3_65_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxon source/pattern opaque. */
static unsigned xrop3_65_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSxon source opaque/pattern transparent. */
static unsigned xrop3_65_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxon source transparent/pattern opaque. */
static unsigned xrop3_65_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxon source/pattern transparent. */
static unsigned xrop3_65_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDxTDxa source/pattern opaque. */
static void rop3_66_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = stk1;
}

/* SDxTDxa source opaque/pattern transparent. */
static void rop3_66_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDxTDxa source transparent/pattern opaque. */
static void rop3_66_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDxTDxa source/pattern transparent. */
static void rop3_66_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDxTDxa source/pattern opaque. */
static unsigned xrop3_66_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return stk1;
}

/* SDxTDxa source opaque/pattern transparent. */
static unsigned xrop3_66_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDxTDxa source transparent/pattern opaque. */
static unsigned xrop3_66_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDxTDxa source/pattern transparent. */
static unsigned xrop3_66_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSanaxn source/pattern opaque. */
static void rop3_67_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSanaxn source opaque/pattern transparent. */
static void rop3_67_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSanaxn source transparent/pattern opaque. */
static void rop3_67_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSanaxn source/pattern transparent. */
static void rop3_67_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSanaxn source/pattern opaque. */
static unsigned xrop3_67_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSanaxn source opaque/pattern transparent. */
static unsigned xrop3_67_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSanaxn source transparent/pattern opaque. */
static unsigned xrop3_67_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSanaxn source/pattern transparent. */
static unsigned xrop3_67_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDna source/pattern opaque. */
static void rop3_68_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDna source opaque/pattern transparent. */
static void rop3_68_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDna source transparent/pattern opaque. */
static void rop3_68_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDna source/pattern transparent. */
static void rop3_68_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDna source/pattern opaque. */
static unsigned xrop3_68_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S & stk2;
  return stk1;
}

/* SDna source opaque/pattern transparent. */
static unsigned xrop3_68_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDna source transparent/pattern opaque. */
static unsigned xrop3_68_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDna source/pattern transparent. */
static unsigned xrop3_68_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnaon source/pattern opaque. */
static void rop3_69_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSnaon source opaque/pattern transparent. */
static void rop3_69_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnaon source transparent/pattern opaque. */
static void rop3_69_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnaon source/pattern transparent. */
static void rop3_69_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnaon source/pattern opaque. */
static unsigned xrop3_69_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSnaon source opaque/pattern transparent. */
static unsigned xrop3_69_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnaon source transparent/pattern opaque. */
static unsigned xrop3_69_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnaon source/pattern transparent. */
static unsigned xrop3_69_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDaox source/pattern opaque. */
static void rop3_70_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTDaox source opaque/pattern transparent. */
static void rop3_70_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDaox source transparent/pattern opaque. */
static void rop3_70_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDaox source/pattern transparent. */
static void rop3_70_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDaox source/pattern opaque. */
static unsigned xrop3_70_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTDaox source opaque/pattern transparent. */
static unsigned xrop3_70_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDaox source transparent/pattern opaque. */
static unsigned xrop3_70_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDaox source/pattern transparent. */
static unsigned xrop3_70_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTxaxn source/pattern opaque. */
static void rop3_71_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDTxaxn source opaque/pattern transparent. */
static void rop3_71_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTxaxn source transparent/pattern opaque. */
static void rop3_71_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTxaxn source/pattern transparent. */
static void rop3_71_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTxaxn source/pattern opaque. */
static unsigned xrop3_71_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDTxaxn source opaque/pattern transparent. */
static unsigned xrop3_71_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTxaxn source transparent/pattern opaque. */
static unsigned xrop3_71_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTxaxn source/pattern transparent. */
static unsigned xrop3_71_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxa source/pattern opaque. */
static void rop3_72_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDTxa source opaque/pattern transparent. */
static void rop3_72_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxa source transparent/pattern opaque. */
static void rop3_72_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxa source/pattern transparent. */
static void rop3_72_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxa source/pattern opaque. */
static unsigned xrop3_72_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  return stk1;
}

/* SDTxa source opaque/pattern transparent. */
static unsigned xrop3_72_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxa source transparent/pattern opaque. */
static unsigned xrop3_72_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxa source/pattern transparent. */
static unsigned xrop3_72_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTDaoxxn source/pattern opaque. */
static void rop3_73_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & *D;
  stk3 = S | stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTDaoxxn source opaque/pattern transparent. */
static void rop3_73_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & *D;
  stk3 = S | stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTDaoxxn source transparent/pattern opaque. */
static void rop3_73_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & *D;
  stk3 = S | stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTDaoxxn source/pattern transparent. */
static void rop3_73_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & *D;
  stk3 = S | stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTDaoxxn source/pattern opaque. */
static unsigned xrop3_73_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & D;
  stk3 = S | stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTDaoxxn source opaque/pattern transparent. */
static unsigned xrop3_73_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & D;
  stk3 = S | stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTDaoxxn source transparent/pattern opaque. */
static unsigned xrop3_73_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & D;
  stk3 = S | stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTDaoxxn source/pattern transparent. */
static unsigned xrop3_73_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & D;
  stk3 = S | stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDoax source/pattern opaque. */
static void rop3_74_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDoax source opaque/pattern transparent. */
static void rop3_74_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDoax source transparent/pattern opaque. */
static void rop3_74_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDoax source/pattern transparent. */
static void rop3_74_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDoax source/pattern opaque. */
static unsigned xrop3_74_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDoax source opaque/pattern transparent. */
static unsigned xrop3_74_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDoax source transparent/pattern opaque. */
static unsigned xrop3_74_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDoax source/pattern transparent. */
static unsigned xrop3_74_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSnox source/pattern opaque. */
static void rop3_75_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSnox source opaque/pattern transparent. */
static void rop3_75_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSnox source transparent/pattern opaque. */
static void rop3_75_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSnox source/pattern transparent. */
static void rop3_75_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSnox source/pattern opaque. */
static unsigned xrop3_75_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSnox source opaque/pattern transparent. */
static unsigned xrop3_75_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSnox source transparent/pattern opaque. */
static unsigned xrop3_75_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSnox source/pattern transparent. */
static unsigned xrop3_75_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTana source/pattern opaque. */
static void rop3_76_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDTana source opaque/pattern transparent. */
static void rop3_76_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTana source transparent/pattern opaque. */
static void rop3_76_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTana source/pattern transparent. */
static void rop3_76_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTana source/pattern opaque. */
static unsigned xrop3_76_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return stk1;
}

/* SDTana source opaque/pattern transparent. */
static unsigned xrop3_76_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTana source transparent/pattern opaque. */
static unsigned xrop3_76_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTana source/pattern transparent. */
static unsigned xrop3_76_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSTxDSxoxn source/pattern opaque. */
static void rop3_77_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SSTxDSxoxn source opaque/pattern transparent. */
static void rop3_77_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSTxDSxoxn source transparent/pattern opaque. */
static void rop3_77_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSTxDSxoxn source/pattern transparent. */
static void rop3_77_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSTxDSxoxn source/pattern opaque. */
static unsigned xrop3_77_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SSTxDSxoxn source opaque/pattern transparent. */
static unsigned xrop3_77_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSTxDSxoxn source transparent/pattern opaque. */
static unsigned xrop3_77_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SSTxDSxoxn source/pattern transparent. */
static unsigned xrop3_77_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTxox source/pattern opaque. */
static void rop3_78_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSTxox source opaque/pattern transparent. */
static void rop3_78_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTxox source transparent/pattern opaque. */
static void rop3_78_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTxox source/pattern transparent. */
static void rop3_78_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTxox source/pattern opaque. */
static unsigned xrop3_78_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSTxox source opaque/pattern transparent. */
static unsigned xrop3_78_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTxox source transparent/pattern opaque. */
static unsigned xrop3_78_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTxox source/pattern transparent. */
static unsigned xrop3_78_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSnoan source/pattern opaque. */
static void rop3_79_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSnoan source opaque/pattern transparent. */
static void rop3_79_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSnoan source transparent/pattern opaque. */
static void rop3_79_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSnoan source/pattern transparent. */
static void rop3_79_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSnoan source/pattern opaque. */
static unsigned xrop3_79_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSnoan source opaque/pattern transparent. */
static unsigned xrop3_79_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSnoan source transparent/pattern opaque. */
static unsigned xrop3_79_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSnoan source/pattern transparent. */
static unsigned xrop3_79_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDna source/pattern opaque. */
static void rop3_80_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDna source opaque/pattern transparent. */
static void rop3_80_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDna source transparent/pattern opaque. */
static void rop3_80_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDna source/pattern transparent. */
static void rop3_80_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDna source/pattern opaque. */
static unsigned xrop3_80_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T & stk2;
  return stk1;
}

/* TDna source opaque/pattern transparent. */
static unsigned xrop3_80_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDna source transparent/pattern opaque. */
static unsigned xrop3_80_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDna source/pattern transparent. */
static unsigned xrop3_80_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTnaon source/pattern opaque. */
static void rop3_81_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTnaon source opaque/pattern transparent. */
static void rop3_81_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTnaon source transparent/pattern opaque. */
static void rop3_81_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTnaon source/pattern transparent. */
static void rop3_81_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTnaon source/pattern opaque. */
static unsigned xrop3_81_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTnaon source opaque/pattern transparent. */
static unsigned xrop3_81_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTnaon source transparent/pattern opaque. */
static unsigned xrop3_81_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTnaon source/pattern transparent. */
static unsigned xrop3_81_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDaox source/pattern opaque. */
static void rop3_82_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDaox source opaque/pattern transparent. */
static void rop3_82_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDaox source transparent/pattern opaque. */
static void rop3_82_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDaox source/pattern transparent. */
static void rop3_82_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDaox source/pattern opaque. */
static unsigned xrop3_82_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDaox source opaque/pattern transparent. */
static unsigned xrop3_82_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDaox source transparent/pattern opaque. */
static unsigned xrop3_82_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDaox source/pattern transparent. */
static unsigned xrop3_82_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSxaxn source/pattern opaque. */
static void rop3_83_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSxaxn source opaque/pattern transparent. */
static void rop3_83_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSxaxn source transparent/pattern opaque. */
static void rop3_83_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSxaxn source/pattern transparent. */
static void rop3_83_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSxaxn source/pattern opaque. */
static unsigned xrop3_83_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSxaxn source opaque/pattern transparent. */
static unsigned xrop3_83_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSxaxn source transparent/pattern opaque. */
static unsigned xrop3_83_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSxaxn source/pattern transparent. */
static unsigned xrop3_83_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSonon source/pattern opaque. */
static void rop3_84_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSonon source opaque/pattern transparent. */
static void rop3_84_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSonon source transparent/pattern opaque. */
static void rop3_84_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSonon source/pattern transparent. */
static void rop3_84_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSonon source/pattern opaque. */
static unsigned xrop3_84_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSonon source opaque/pattern transparent. */
static unsigned xrop3_84_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSonon source transparent/pattern opaque. */
static unsigned xrop3_84_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSonon source/pattern transparent. */
static unsigned xrop3_84_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* Dn source/pattern opaque. */
static void rop3_85_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~*D;
  *D = stk1;
}

/* Dn source opaque/pattern transparent. */
static void rop3_85_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~*D;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* Dn source transparent/pattern opaque. */
static void rop3_85_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~*D;
  *D = (stk1 & (~S)) | (*D & S);
}

/* Dn source/pattern transparent. */
static void rop3_85_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = ~*D;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* Dn source/pattern opaque. */
static unsigned xrop3_85_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned stk1;
  stk1 = ~D;
  return stk1;
}

/* Dn source opaque/pattern transparent. */
static unsigned xrop3_85_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~D;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* Dn source transparent/pattern opaque. */
static unsigned xrop3_85_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = ~D;
  return (stk1 & (~S)) | (D & S);
}

/* Dn source/pattern transparent. */
static unsigned xrop3_85_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = ~D;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSox source/pattern opaque. */
static void rop3_86_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSox source opaque/pattern transparent. */
static void rop3_86_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSox source transparent/pattern opaque. */
static void rop3_86_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSox source/pattern transparent. */
static void rop3_86_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSox source/pattern opaque. */
static unsigned xrop3_86_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSox source opaque/pattern transparent. */
static unsigned xrop3_86_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSox source transparent/pattern opaque. */
static unsigned xrop3_86_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSox source/pattern transparent. */
static unsigned xrop3_86_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSoan source/pattern opaque. */
static void rop3_87_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSoan source opaque/pattern transparent. */
static void rop3_87_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSoan source transparent/pattern opaque. */
static void rop3_87_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSoan source/pattern transparent. */
static void rop3_87_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSoan source/pattern opaque. */
static unsigned xrop3_87_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSoan source opaque/pattern transparent. */
static unsigned xrop3_87_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSoan source transparent/pattern opaque. */
static unsigned xrop3_87_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSoan source/pattern transparent. */
static unsigned xrop3_87_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSToax source/pattern opaque. */
static void rop3_88_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSToax source opaque/pattern transparent. */
static void rop3_88_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSToax source transparent/pattern opaque. */
static void rop3_88_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSToax source/pattern transparent. */
static void rop3_88_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSToax source/pattern opaque. */
static unsigned xrop3_88_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSToax source opaque/pattern transparent. */
static unsigned xrop3_88_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSToax source transparent/pattern opaque. */
static unsigned xrop3_88_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSToax source/pattern transparent. */
static unsigned xrop3_88_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnox source/pattern opaque. */
static void rop3_89_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSnox source opaque/pattern transparent. */
static void rop3_89_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnox source transparent/pattern opaque. */
static void rop3_89_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnox source/pattern transparent. */
static void rop3_89_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnox source/pattern opaque. */
static unsigned xrop3_89_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSnox source opaque/pattern transparent. */
static unsigned xrop3_89_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnox source transparent/pattern opaque. */
static unsigned xrop3_89_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnox source/pattern transparent. */
static unsigned xrop3_89_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTx source/pattern opaque. */
static void rop3_90_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ T;
  *D = stk1;
}

/* DTx source opaque/pattern transparent. */
static void rop3_90_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ T;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTx source transparent/pattern opaque. */
static void rop3_90_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ T;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTx source/pattern transparent. */
static void rop3_90_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ T;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTx source/pattern opaque. */
static unsigned xrop3_90_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ T;
  return stk1;
}

/* DTx source opaque/pattern transparent. */
static unsigned xrop3_90_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ T;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTx source transparent/pattern opaque. */
static unsigned xrop3_90_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ T;
  return (stk1 & (~S)) | (D & S);
}

/* DTx source/pattern transparent. */
static unsigned xrop3_90_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ T;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDonox source/pattern opaque. */
static void rop3_91_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDonox source opaque/pattern transparent. */
static void rop3_91_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDonox source transparent/pattern opaque. */
static void rop3_91_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDonox source/pattern transparent. */
static void rop3_91_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDonox source/pattern opaque. */
static unsigned xrop3_91_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDonox source opaque/pattern transparent. */
static unsigned xrop3_91_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDonox source transparent/pattern opaque. */
static unsigned xrop3_91_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDonox source/pattern transparent. */
static unsigned xrop3_91_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDxox source/pattern opaque. */
static void rop3_92_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDxox source opaque/pattern transparent. */
static void rop3_92_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDxox source transparent/pattern opaque. */
static void rop3_92_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDxox source/pattern transparent. */
static void rop3_92_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDxox source/pattern opaque. */
static unsigned xrop3_92_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDxox source opaque/pattern transparent. */
static unsigned xrop3_92_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDxox source transparent/pattern opaque. */
static unsigned xrop3_92_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDxox source/pattern transparent. */
static unsigned xrop3_92_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnoan source/pattern opaque. */
static void rop3_93_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSnoan source opaque/pattern transparent. */
static void rop3_93_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnoan source transparent/pattern opaque. */
static void rop3_93_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnoan source/pattern transparent. */
static void rop3_93_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnoan source/pattern opaque. */
static unsigned xrop3_93_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSnoan source opaque/pattern transparent. */
static unsigned xrop3_93_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnoan source transparent/pattern opaque. */
static unsigned xrop3_93_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnoan source/pattern transparent. */
static unsigned xrop3_93_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDnaox source/pattern opaque. */
static void rop3_94_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDnaox source opaque/pattern transparent. */
static void rop3_94_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDnaox source transparent/pattern opaque. */
static void rop3_94_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDnaox source/pattern transparent. */
static void rop3_94_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDnaox source/pattern opaque. */
static unsigned xrop3_94_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDnaox source opaque/pattern transparent. */
static unsigned xrop3_94_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDnaox source transparent/pattern opaque. */
static unsigned xrop3_94_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDnaox source/pattern transparent. */
static unsigned xrop3_94_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S & stk4;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTan source/pattern opaque. */
static void rop3_95_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTan source opaque/pattern transparent. */
static void rop3_95_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTan source transparent/pattern opaque. */
static void rop3_95_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTan source/pattern transparent. */
static void rop3_95_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTan source/pattern opaque. */
static unsigned xrop3_95_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  stk1 = ~stk1;
  return stk1;
}

/* DTan source opaque/pattern transparent. */
static unsigned xrop3_95_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTan source transparent/pattern opaque. */
static unsigned xrop3_95_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTan source/pattern transparent. */
static unsigned xrop3_95_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxa source/pattern opaque. */
static void rop3_96_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDSxa source opaque/pattern transparent. */
static void rop3_96_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxa source transparent/pattern opaque. */
static void rop3_96_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxa source/pattern transparent. */
static void rop3_96_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxa source/pattern opaque. */
static unsigned xrop3_96_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  return stk1;
}

/* TDSxa source opaque/pattern transparent. */
static unsigned xrop3_96_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxa source transparent/pattern opaque. */
static unsigned xrop3_96_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxa source/pattern transparent. */
static unsigned xrop3_96_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDSaoxxn source/pattern opaque. */
static void rop3_97_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDSaoxxn source opaque/pattern transparent. */
static void rop3_97_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDSaoxxn source transparent/pattern opaque. */
static void rop3_97_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDSaoxxn source/pattern transparent. */
static void rop3_97_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDSaoxxn source/pattern opaque. */
static unsigned xrop3_97_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDSaoxxn source opaque/pattern transparent. */
static unsigned xrop3_97_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDSaoxxn source transparent/pattern opaque. */
static unsigned xrop3_97_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDSaoxxn source/pattern transparent. */
static unsigned xrop3_97_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDoax source/pattern opaque. */
static void rop3_98_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTDoax source opaque/pattern transparent. */
static void rop3_98_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDoax source transparent/pattern opaque. */
static void rop3_98_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDoax source/pattern transparent. */
static void rop3_98_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDoax source/pattern opaque. */
static unsigned xrop3_98_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTDoax source opaque/pattern transparent. */
static unsigned xrop3_98_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDoax source transparent/pattern opaque. */
static unsigned xrop3_98_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDoax source/pattern transparent. */
static unsigned xrop3_98_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnox source/pattern opaque. */
static void rop3_99_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTnox source opaque/pattern transparent. */
static void rop3_99_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnox source transparent/pattern opaque. */
static void rop3_99_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnox source/pattern transparent. */
static void rop3_99_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnox source/pattern opaque. */
static unsigned xrop3_99_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTnox source opaque/pattern transparent. */
static unsigned xrop3_99_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnox source transparent/pattern opaque. */
static unsigned xrop3_99_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnox source/pattern transparent. */
static unsigned xrop3_99_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSoax source/pattern opaque. */
static void rop3_100_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSoax source opaque/pattern transparent. */
static void rop3_100_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSoax source transparent/pattern opaque. */
static void rop3_100_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSoax source/pattern transparent. */
static void rop3_100_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSoax source/pattern opaque. */
static unsigned xrop3_100_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSoax source opaque/pattern transparent. */
static unsigned xrop3_100_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSoax source transparent/pattern opaque. */
static unsigned xrop3_100_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSoax source/pattern transparent. */
static unsigned xrop3_100_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTnox source/pattern opaque. */
static void rop3_101_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTnox source opaque/pattern transparent. */
static void rop3_101_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTnox source transparent/pattern opaque. */
static void rop3_101_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTnox source/pattern transparent. */
static void rop3_101_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTnox source/pattern opaque. */
static unsigned xrop3_101_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTnox source opaque/pattern transparent. */
static unsigned xrop3_101_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTnox source transparent/pattern opaque. */
static unsigned xrop3_101_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTnox source/pattern transparent. */
static unsigned xrop3_101_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSx source/pattern opaque. */
static void rop3_102_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  *D = stk1;
}

/* DSx source opaque/pattern transparent. */
static void rop3_102_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSx source transparent/pattern opaque. */
static void rop3_102_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSx source/pattern transparent. */
static void rop3_102_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSx source/pattern opaque. */
static unsigned xrop3_102_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D ^ S;
  return stk1;
}

/* DSx source opaque/pattern transparent. */
static unsigned xrop3_102_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSx source transparent/pattern opaque. */
static unsigned xrop3_102_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D ^ S;
  return (stk1 & (~S)) | (D & S);
}

/* DSx source/pattern transparent. */
static unsigned xrop3_102_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSonox source/pattern opaque. */
static void rop3_103_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSonox source opaque/pattern transparent. */
static void rop3_103_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSonox source transparent/pattern opaque. */
static void rop3_103_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSonox source/pattern transparent. */
static void rop3_103_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSonox source/pattern opaque. */
static unsigned xrop3_103_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSonox source opaque/pattern transparent. */
static unsigned xrop3_103_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSonox source transparent/pattern opaque. */
static unsigned xrop3_103_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSonox source/pattern transparent. */
static unsigned xrop3_103_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDSonoxxn source/pattern opaque. */
static void rop3_104_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDSonoxxn source opaque/pattern transparent. */
static void rop3_104_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDSonoxxn source transparent/pattern opaque. */
static void rop3_104_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDSonoxxn source/pattern transparent. */
static void rop3_104_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDSonoxxn source/pattern opaque. */
static unsigned xrop3_104_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDSonoxxn source opaque/pattern transparent. */
static unsigned xrop3_104_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDSonoxxn source transparent/pattern opaque. */
static unsigned xrop3_104_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDSonoxxn source/pattern transparent. */
static unsigned xrop3_104_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk4 = ~stk4;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxxn source/pattern opaque. */
static void rop3_105_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSxxn source opaque/pattern transparent. */
static void rop3_105_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxxn source transparent/pattern opaque. */
static void rop3_105_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxxn source/pattern transparent. */
static void rop3_105_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxxn source/pattern opaque. */
static unsigned xrop3_105_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSxxn source opaque/pattern transparent. */
static unsigned xrop3_105_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxxn source transparent/pattern opaque. */
static unsigned xrop3_105_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxxn source/pattern transparent. */
static unsigned xrop3_105_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSax source/pattern opaque. */
static void rop3_106_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSax source opaque/pattern transparent. */
static void rop3_106_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSax source transparent/pattern opaque. */
static void rop3_106_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSax source/pattern transparent. */
static void rop3_106_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSax source/pattern opaque. */
static unsigned xrop3_106_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSax source opaque/pattern transparent. */
static unsigned xrop3_106_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSax source transparent/pattern opaque. */
static unsigned xrop3_106_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSax source/pattern transparent. */
static unsigned xrop3_106_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTSoaxxn source/pattern opaque. */
static void rop3_107_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDTSoaxxn source opaque/pattern transparent. */
static void rop3_107_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTSoaxxn source transparent/pattern opaque. */
static void rop3_107_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTSoaxxn source/pattern transparent. */
static void rop3_107_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTSoaxxn source/pattern opaque. */
static unsigned xrop3_107_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDTSoaxxn source opaque/pattern transparent. */
static unsigned xrop3_107_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTSoaxxn source transparent/pattern opaque. */
static unsigned xrop3_107_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTSoaxxn source/pattern transparent. */
static unsigned xrop3_107_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTax source/pattern opaque. */
static void rop3_108_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTax source opaque/pattern transparent. */
static void rop3_108_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTax source transparent/pattern opaque. */
static void rop3_108_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTax source/pattern transparent. */
static void rop3_108_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTax source/pattern opaque. */
static unsigned xrop3_108_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTax source opaque/pattern transparent. */
static unsigned xrop3_108_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTax source transparent/pattern opaque. */
static unsigned xrop3_108_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTax source/pattern transparent. */
static unsigned xrop3_108_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTDoaxxn source/pattern opaque. */
static void rop3_109_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | *D;
  stk3 = S & stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTDoaxxn source opaque/pattern transparent. */
static void rop3_109_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | *D;
  stk3 = S & stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTDoaxxn source transparent/pattern opaque. */
static void rop3_109_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | *D;
  stk3 = S & stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTDoaxxn source/pattern transparent. */
static void rop3_109_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | *D;
  stk3 = S & stk4;
  stk2 = *D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTDoaxxn source/pattern opaque. */
static unsigned xrop3_109_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | D;
  stk3 = S & stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTDoaxxn source opaque/pattern transparent. */
static unsigned xrop3_109_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | D;
  stk3 = S & stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTDoaxxn source transparent/pattern opaque. */
static unsigned xrop3_109_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | D;
  stk3 = S & stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTDoaxxn source/pattern transparent. */
static unsigned xrop3_109_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | D;
  stk3 = S & stk4;
  stk2 = D ^ stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSnoax source/pattern opaque. */
static void rop3_110_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSnoax source opaque/pattern transparent. */
static void rop3_110_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSnoax source transparent/pattern opaque. */
static void rop3_110_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSnoax source/pattern transparent. */
static void rop3_110_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSnoax source/pattern opaque. */
static unsigned xrop3_110_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSnoax source opaque/pattern transparent. */
static unsigned xrop3_110_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSnoax source transparent/pattern opaque. */
static unsigned xrop3_110_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSnoax source/pattern transparent. */
static unsigned xrop3_110_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxnan source/pattern opaque. */
static void rop3_111_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSxnan source opaque/pattern transparent. */
static void rop3_111_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxnan source transparent/pattern opaque. */
static void rop3_111_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxnan source/pattern transparent. */
static void rop3_111_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxnan source/pattern opaque. */
static unsigned xrop3_111_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSxnan source opaque/pattern transparent. */
static unsigned xrop3_111_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxnan source transparent/pattern opaque. */
static unsigned xrop3_111_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxnan source/pattern transparent. */
static unsigned xrop3_111_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSana source/pattern opaque. */
static void rop3_112_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDSana source opaque/pattern transparent. */
static void rop3_112_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSana source transparent/pattern opaque. */
static void rop3_112_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSana source/pattern transparent. */
static void rop3_112_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSana source/pattern opaque. */
static unsigned xrop3_112_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return stk1;
}

/* TDSana source opaque/pattern transparent. */
static unsigned xrop3_112_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSana source transparent/pattern opaque. */
static unsigned xrop3_112_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSana source/pattern transparent. */
static unsigned xrop3_112_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSDxTDxaxn source/pattern opaque. */
static void rop3_113_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SSDxTDxaxn source opaque/pattern transparent. */
static void rop3_113_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSDxTDxaxn source transparent/pattern opaque. */
static void rop3_113_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSDxTDxaxn source/pattern transparent. */
static void rop3_113_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSDxTDxaxn source/pattern opaque. */
static unsigned xrop3_113_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SSDxTDxaxn source opaque/pattern transparent. */
static unsigned xrop3_113_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSDxTDxaxn source transparent/pattern opaque. */
static unsigned xrop3_113_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SSDxTDxaxn source/pattern transparent. */
static unsigned xrop3_113_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSxox source/pattern opaque. */
static void rop3_114_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSxox source opaque/pattern transparent. */
static void rop3_114_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSxox source transparent/pattern opaque. */
static void rop3_114_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSxox source/pattern transparent. */
static void rop3_114_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSxox source/pattern opaque. */
static unsigned xrop3_114_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSxox source opaque/pattern transparent. */
static unsigned xrop3_114_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSxox source transparent/pattern opaque. */
static unsigned xrop3_114_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSxox source/pattern transparent. */
static unsigned xrop3_114_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnoan source/pattern opaque. */
static void rop3_115_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTnoan source opaque/pattern transparent. */
static void rop3_115_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnoan source transparent/pattern opaque. */
static void rop3_115_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnoan source/pattern transparent. */
static void rop3_115_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnoan source/pattern opaque. */
static unsigned xrop3_115_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTnoan source opaque/pattern transparent. */
static unsigned xrop3_115_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnoan source transparent/pattern opaque. */
static unsigned xrop3_115_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnoan source/pattern transparent. */
static unsigned xrop3_115_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDxox source/pattern opaque. */
static void rop3_116_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTDxox source opaque/pattern transparent. */
static void rop3_116_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDxox source transparent/pattern opaque. */
static void rop3_116_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDxox source/pattern transparent. */
static void rop3_116_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDxox source/pattern opaque. */
static unsigned xrop3_116_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTDxox source opaque/pattern transparent. */
static unsigned xrop3_116_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDxox source transparent/pattern opaque. */
static unsigned xrop3_116_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDxox source/pattern transparent. */
static unsigned xrop3_116_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTnoan source/pattern opaque. */
static void rop3_117_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTnoan source opaque/pattern transparent. */
static void rop3_117_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTnoan source transparent/pattern opaque. */
static void rop3_117_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTnoan source/pattern transparent. */
static void rop3_117_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTnoan source/pattern opaque. */
static unsigned xrop3_117_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTnoan source opaque/pattern transparent. */
static unsigned xrop3_117_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTnoan source transparent/pattern opaque. */
static unsigned xrop3_117_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTnoan source/pattern transparent. */
static unsigned xrop3_117_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSnaox source/pattern opaque. */
static void rop3_118_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSnaox source opaque/pattern transparent. */
static void rop3_118_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSnaox source transparent/pattern opaque. */
static void rop3_118_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSnaox source/pattern transparent. */
static void rop3_118_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSnaox source/pattern opaque. */
static unsigned xrop3_118_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSnaox source opaque/pattern transparent. */
static unsigned xrop3_118_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSnaox source transparent/pattern opaque. */
static unsigned xrop3_118_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSnaox source/pattern transparent. */
static unsigned xrop3_118_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSan source/pattern opaque. */
static void rop3_119_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSan source opaque/pattern transparent. */
static void rop3_119_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSan source transparent/pattern opaque. */
static void rop3_119_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSan source/pattern transparent. */
static void rop3_119_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSan source/pattern opaque. */
static unsigned xrop3_119_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D & S;
  stk1 = ~stk1;
  return stk1;
}

/* DSan source opaque/pattern transparent. */
static unsigned xrop3_119_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & S;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSan source transparent/pattern opaque. */
static unsigned xrop3_119_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D & S;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSan source/pattern transparent. */
static unsigned xrop3_119_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & S;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSax source/pattern opaque. */
static void rop3_120_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSax source opaque/pattern transparent. */
static void rop3_120_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSax source transparent/pattern opaque. */
static void rop3_120_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSax source/pattern transparent. */
static void rop3_120_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSax source/pattern opaque. */
static unsigned xrop3_120_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSax source opaque/pattern transparent. */
static unsigned xrop3_120_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSax source transparent/pattern opaque. */
static unsigned xrop3_120_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSax source/pattern transparent. */
static unsigned xrop3_120_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDSoaxxn source/pattern opaque. */
static void rop3_121_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDSoaxxn source opaque/pattern transparent. */
static void rop3_121_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDSoaxxn source transparent/pattern opaque. */
static void rop3_121_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDSoaxxn source/pattern transparent. */
static void rop3_121_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDSoaxxn source/pattern opaque. */
static unsigned xrop3_121_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDSoaxxn source opaque/pattern transparent. */
static unsigned xrop3_121_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDSoaxxn source transparent/pattern opaque. */
static unsigned xrop3_121_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDSoaxxn source/pattern transparent. */
static unsigned xrop3_121_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDnoax source/pattern opaque. */
static void rop3_122_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDnoax source opaque/pattern transparent. */
static void rop3_122_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDnoax source transparent/pattern opaque. */
static void rop3_122_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDnoax source/pattern transparent. */
static void rop3_122_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~*D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDnoax source/pattern opaque. */
static unsigned xrop3_122_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDnoax source opaque/pattern transparent. */
static unsigned xrop3_122_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDnoax source transparent/pattern opaque. */
static unsigned xrop3_122_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDnoax source/pattern transparent. */
static unsigned xrop3_122_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~D;
  stk3 = S | stk4;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxnan source/pattern opaque. */
static void rop3_123_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTxnan source opaque/pattern transparent. */
static void rop3_123_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxnan source transparent/pattern opaque. */
static void rop3_123_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxnan source/pattern transparent. */
static void rop3_123_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxnan source/pattern opaque. */
static unsigned xrop3_123_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTxnan source opaque/pattern transparent. */
static unsigned xrop3_123_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxnan source transparent/pattern opaque. */
static unsigned xrop3_123_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxnan source/pattern transparent. */
static unsigned xrop3_123_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSnoax source/pattern opaque. */
static void rop3_124_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSnoax source opaque/pattern transparent. */
static void rop3_124_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSnoax source transparent/pattern opaque. */
static void rop3_124_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSnoax source/pattern transparent. */
static void rop3_124_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSnoax source/pattern opaque. */
static unsigned xrop3_124_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSnoax source opaque/pattern transparent. */
static unsigned xrop3_124_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSnoax source transparent/pattern opaque. */
static unsigned xrop3_124_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSnoax source/pattern transparent. */
static unsigned xrop3_124_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxnan source/pattern opaque. */
static void rop3_125_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSxnan source opaque/pattern transparent. */
static void rop3_125_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxnan source transparent/pattern opaque. */
static void rop3_125_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxnan source/pattern transparent. */
static void rop3_125_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxnan source/pattern opaque. */
static unsigned xrop3_125_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSxnan source opaque/pattern transparent. */
static unsigned xrop3_125_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxnan source transparent/pattern opaque. */
static unsigned xrop3_125_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxnan source/pattern transparent. */
static unsigned xrop3_125_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STxDSxo source/pattern opaque. */
static void rop3_126_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  *D = stk1;
}

/* STxDSxo source opaque/pattern transparent. */
static void rop3_126_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STxDSxo source transparent/pattern opaque. */
static void rop3_126_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STxDSxo source/pattern transparent. */
static void rop3_126_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STxDSxo source/pattern opaque. */
static unsigned xrop3_126_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  return stk1;
}

/* STxDSxo source opaque/pattern transparent. */
static unsigned xrop3_126_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STxDSxo source transparent/pattern opaque. */
static unsigned xrop3_126_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STxDSxo source/pattern transparent. */
static unsigned xrop3_126_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSaan source/pattern opaque. */
static void rop3_127_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSaan source opaque/pattern transparent. */
static void rop3_127_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSaan source transparent/pattern opaque. */
static void rop3_127_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSaan source/pattern transparent. */
static void rop3_127_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSaan source/pattern opaque. */
static unsigned xrop3_127_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSaan source opaque/pattern transparent. */
static unsigned xrop3_127_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSaan source transparent/pattern opaque. */
static unsigned xrop3_127_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSaan source/pattern transparent. */
static unsigned xrop3_127_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSaa source/pattern opaque. */
static void rop3_128_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSaa source opaque/pattern transparent. */
static void rop3_128_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSaa source transparent/pattern opaque. */
static void rop3_128_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSaa source/pattern transparent. */
static void rop3_128_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSaa source/pattern opaque. */
static unsigned xrop3_128_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  return stk1;
}

/* DTSaa source opaque/pattern transparent. */
static unsigned xrop3_128_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSaa source transparent/pattern opaque. */
static unsigned xrop3_128_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSaa source/pattern transparent. */
static unsigned xrop3_128_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STxDSxon source/pattern opaque. */
static void rop3_129_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STxDSxon source opaque/pattern transparent. */
static void rop3_129_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STxDSxon source transparent/pattern opaque. */
static void rop3_129_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STxDSxon source/pattern transparent. */
static void rop3_129_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STxDSxon source/pattern opaque. */
static unsigned xrop3_129_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STxDSxon source opaque/pattern transparent. */
static unsigned xrop3_129_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STxDSxon source transparent/pattern opaque. */
static unsigned xrop3_129_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STxDSxon source/pattern transparent. */
static unsigned xrop3_129_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 | stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxna source/pattern opaque. */
static void rop3_130_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSxna source opaque/pattern transparent. */
static void rop3_130_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxna source transparent/pattern opaque. */
static void rop3_130_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxna source/pattern transparent. */
static void rop3_130_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxna source/pattern opaque. */
static unsigned xrop3_130_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return stk1;
}

/* DTSxna source opaque/pattern transparent. */
static unsigned xrop3_130_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxna source transparent/pattern opaque. */
static unsigned xrop3_130_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxna source/pattern transparent. */
static unsigned xrop3_130_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSnoaxn source/pattern opaque. */
static void rop3_131_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSnoaxn source opaque/pattern transparent. */
static void rop3_131_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSnoaxn source transparent/pattern opaque. */
static void rop3_131_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSnoaxn source/pattern transparent. */
static void rop3_131_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSnoaxn source/pattern opaque. */
static unsigned xrop3_131_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSnoaxn source opaque/pattern transparent. */
static unsigned xrop3_131_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSnoaxn source transparent/pattern opaque. */
static unsigned xrop3_131_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSnoaxn source/pattern transparent. */
static unsigned xrop3_131_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D | stk4;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxna source/pattern opaque. */
static void rop3_132_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDTxna source opaque/pattern transparent. */
static void rop3_132_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxna source transparent/pattern opaque. */
static void rop3_132_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxna source/pattern transparent. */
static void rop3_132_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxna source/pattern opaque. */
static unsigned xrop3_132_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return stk1;
}

/* SDTxna source opaque/pattern transparent. */
static unsigned xrop3_132_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxna source transparent/pattern opaque. */
static unsigned xrop3_132_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxna source/pattern transparent. */
static unsigned xrop3_132_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTnoaxn source/pattern opaque. */
static void rop3_133_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTnoaxn source opaque/pattern transparent. */
static void rop3_133_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTnoaxn source transparent/pattern opaque. */
static void rop3_133_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTnoaxn source/pattern transparent. */
static void rop3_133_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTnoaxn source/pattern opaque. */
static unsigned xrop3_133_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTnoaxn source opaque/pattern transparent. */
static unsigned xrop3_133_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTnoaxn source transparent/pattern opaque. */
static unsigned xrop3_133_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTnoaxn source/pattern transparent. */
static unsigned xrop3_133_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S | stk4;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDSoaxx source/pattern opaque. */
static void rop3_134_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTDSoaxx source opaque/pattern transparent. */
static void rop3_134_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDSoaxx source transparent/pattern opaque. */
static void rop3_134_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDSoaxx source/pattern transparent. */
static void rop3_134_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDSoaxx source/pattern opaque. */
static unsigned xrop3_134_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTDSoaxx source opaque/pattern transparent. */
static unsigned xrop3_134_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDSoaxx source transparent/pattern opaque. */
static unsigned xrop3_134_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDSoaxx source/pattern transparent. */
static unsigned xrop3_134_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | S;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSaxn source/pattern opaque. */
static void rop3_135_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSaxn source opaque/pattern transparent. */
static void rop3_135_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSaxn source transparent/pattern opaque. */
static void rop3_135_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSaxn source/pattern transparent. */
static void rop3_135_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSaxn source/pattern opaque. */
static unsigned xrop3_135_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSaxn source opaque/pattern transparent. */
static unsigned xrop3_135_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSaxn source transparent/pattern opaque. */
static unsigned xrop3_135_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSaxn source/pattern transparent. */
static unsigned xrop3_135_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSa source/pattern opaque. */
static void rop3_136_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  *D = stk1;
}

/* DSa source opaque/pattern transparent. */
static void rop3_136_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSa source transparent/pattern opaque. */
static void rop3_136_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSa source/pattern transparent. */
static void rop3_136_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSa source/pattern opaque. */
static unsigned xrop3_136_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D & S;
  return stk1;
}

/* DSa source opaque/pattern transparent. */
static unsigned xrop3_136_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSa source transparent/pattern opaque. */
static unsigned xrop3_136_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D & S;
  return (stk1 & (~S)) | (D & S);
}

/* DSa source/pattern transparent. */
static unsigned xrop3_136_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSnaoxn source/pattern opaque. */
static void rop3_137_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSnaoxn source opaque/pattern transparent. */
static void rop3_137_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSnaoxn source transparent/pattern opaque. */
static void rop3_137_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSnaoxn source/pattern transparent. */
static void rop3_137_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSnaoxn source/pattern opaque. */
static unsigned xrop3_137_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSnaoxn source opaque/pattern transparent. */
static unsigned xrop3_137_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSnaoxn source transparent/pattern opaque. */
static unsigned xrop3_137_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSnaoxn source/pattern transparent. */
static unsigned xrop3_137_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T & stk4;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTnoa source/pattern opaque. */
static void rop3_138_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DSTnoa source opaque/pattern transparent. */
static void rop3_138_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTnoa source transparent/pattern opaque. */
static void rop3_138_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTnoa source/pattern transparent. */
static void rop3_138_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTnoa source/pattern opaque. */
static unsigned xrop3_138_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  return stk1;
}

/* DSTnoa source opaque/pattern transparent. */
static unsigned xrop3_138_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTnoa source transparent/pattern opaque. */
static unsigned xrop3_138_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTnoa source/pattern transparent. */
static unsigned xrop3_138_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S | stk3;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDxoxn source/pattern opaque. */
static void rop3_139_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDxoxn source opaque/pattern transparent. */
static void rop3_139_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDxoxn source transparent/pattern opaque. */
static void rop3_139_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDxoxn source/pattern transparent. */
static void rop3_139_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDxoxn source/pattern opaque. */
static unsigned xrop3_139_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDxoxn source opaque/pattern transparent. */
static unsigned xrop3_139_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDxoxn source transparent/pattern opaque. */
static unsigned xrop3_139_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDxoxn source/pattern transparent. */
static unsigned xrop3_139_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnoa source/pattern opaque. */
static void rop3_140_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDTnoa source opaque/pattern transparent. */
static void rop3_140_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnoa source transparent/pattern opaque. */
static void rop3_140_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnoa source/pattern transparent. */
static void rop3_140_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnoa source/pattern opaque. */
static unsigned xrop3_140_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  return stk1;
}

/* SDTnoa source opaque/pattern transparent. */
static unsigned xrop3_140_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnoa source transparent/pattern opaque. */
static unsigned xrop3_140_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnoa source/pattern transparent. */
static unsigned xrop3_140_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSxoxn source/pattern opaque. */
static void rop3_141_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSxoxn source opaque/pattern transparent. */
static void rop3_141_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSxoxn source transparent/pattern opaque. */
static void rop3_141_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSxoxn source/pattern transparent. */
static void rop3_141_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSxoxn source/pattern opaque. */
static unsigned xrop3_141_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSxoxn source opaque/pattern transparent. */
static unsigned xrop3_141_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSxoxn source transparent/pattern opaque. */
static unsigned xrop3_141_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSxoxn source/pattern transparent. */
static unsigned xrop3_141_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSDxTDxax source/pattern opaque. */
static void rop3_142_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SSDxTDxax source opaque/pattern transparent. */
static void rop3_142_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSDxTDxax source transparent/pattern opaque. */
static void rop3_142_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSDxTDxax source/pattern transparent. */
static void rop3_142_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ *D;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSDxTDxax source/pattern opaque. */
static unsigned xrop3_142_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SSDxTDxax source opaque/pattern transparent. */
static unsigned xrop3_142_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSDxTDxax source transparent/pattern opaque. */
static unsigned xrop3_142_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SSDxTDxax source/pattern transparent. */
static unsigned xrop3_142_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ D;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSanan source/pattern opaque. */
static void rop3_143_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSanan source opaque/pattern transparent. */
static void rop3_143_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSanan source transparent/pattern opaque. */
static void rop3_143_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSanan source/pattern transparent. */
static void rop3_143_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSanan source/pattern opaque. */
static unsigned xrop3_143_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSanan source opaque/pattern transparent. */
static unsigned xrop3_143_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSanan source transparent/pattern opaque. */
static unsigned xrop3_143_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSanan source/pattern transparent. */
static unsigned xrop3_143_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxna source/pattern opaque. */
static void rop3_144_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDSxna source opaque/pattern transparent. */
static void rop3_144_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxna source transparent/pattern opaque. */
static void rop3_144_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxna source/pattern transparent. */
static void rop3_144_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxna source/pattern opaque. */
static unsigned xrop3_144_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return stk1;
}

/* TDSxna source opaque/pattern transparent. */
static unsigned xrop3_144_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxna source transparent/pattern opaque. */
static unsigned xrop3_144_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxna source/pattern transparent. */
static unsigned xrop3_144_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSnoaxn source/pattern opaque. */
static void rop3_145_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSnoaxn source opaque/pattern transparent. */
static void rop3_145_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSnoaxn source transparent/pattern opaque. */
static void rop3_145_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSnoaxn source/pattern transparent. */
static void rop3_145_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSnoaxn source/pattern opaque. */
static unsigned xrop3_145_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSnoaxn source opaque/pattern transparent. */
static unsigned xrop3_145_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSnoaxn source transparent/pattern opaque. */
static unsigned xrop3_145_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSnoaxn source/pattern transparent. */
static unsigned xrop3_145_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = T | stk4;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDToaxx source/pattern opaque. */
static void rop3_146_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDToaxx source opaque/pattern transparent. */
static void rop3_146_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDToaxx source transparent/pattern opaque. */
static void rop3_146_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDToaxx source/pattern transparent. */
static void rop3_146_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDToaxx source/pattern opaque. */
static unsigned xrop3_146_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDToaxx source opaque/pattern transparent. */
static unsigned xrop3_146_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDToaxx source transparent/pattern opaque. */
static unsigned xrop3_146_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDToaxx source/pattern transparent. */
static unsigned xrop3_146_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D | T;
  stk3 = S & stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDaxn source/pattern opaque. */
static void rop3_147_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDaxn source opaque/pattern transparent. */
static void rop3_147_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDaxn source transparent/pattern opaque. */
static void rop3_147_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDaxn source/pattern transparent. */
static void rop3_147_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDaxn source/pattern opaque. */
static unsigned xrop3_147_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDaxn source opaque/pattern transparent. */
static unsigned xrop3_147_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDaxn source transparent/pattern opaque. */
static unsigned xrop3_147_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDaxn source/pattern transparent. */
static unsigned xrop3_147_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTSoaxx source/pattern opaque. */
static void rop3_148_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTSoaxx source opaque/pattern transparent. */
static void rop3_148_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTSoaxx source transparent/pattern opaque. */
static void rop3_148_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTSoaxx source/pattern transparent. */
static void rop3_148_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk3 = *D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTSoaxx source/pattern opaque. */
static unsigned xrop3_148_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTSoaxx source opaque/pattern transparent. */
static unsigned xrop3_148_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTSoaxx source transparent/pattern opaque. */
static unsigned xrop3_148_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTSoaxx source/pattern transparent. */
static unsigned xrop3_148_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk3 = D & stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSaxn source/pattern opaque. */
static void rop3_149_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSaxn source opaque/pattern transparent. */
static void rop3_149_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSaxn source transparent/pattern opaque. */
static void rop3_149_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSaxn source/pattern transparent. */
static void rop3_149_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSaxn source/pattern opaque. */
static unsigned xrop3_149_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSaxn source opaque/pattern transparent. */
static unsigned xrop3_149_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSaxn source transparent/pattern opaque. */
static unsigned xrop3_149_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSaxn source/pattern transparent. */
static unsigned xrop3_149_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxx source/pattern opaque. */
static void rop3_150_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSxx source opaque/pattern transparent. */
static void rop3_150_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxx source transparent/pattern opaque. */
static void rop3_150_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxx source/pattern transparent. */
static void rop3_150_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxx source/pattern opaque. */
static unsigned xrop3_150_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSxx source opaque/pattern transparent. */
static unsigned xrop3_150_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxx source transparent/pattern opaque. */
static unsigned xrop3_150_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxx source/pattern transparent. */
static unsigned xrop3_150_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTSonoxx source/pattern opaque. */
static void rop3_151_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTSonoxx source opaque/pattern transparent. */
static void rop3_151_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTSonoxx source transparent/pattern opaque. */
static void rop3_151_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTSonoxx source/pattern transparent. */
static void rop3_151_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTSonoxx source/pattern opaque. */
static unsigned xrop3_151_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTSonoxx source opaque/pattern transparent. */
static unsigned xrop3_151_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTSonoxx source transparent/pattern opaque. */
static unsigned xrop3_151_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTSonoxx source/pattern transparent. */
static unsigned xrop3_151_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T | S;
  stk4 = ~stk4;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSonoxn source/pattern opaque. */
static void rop3_152_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSonoxn source opaque/pattern transparent. */
static void rop3_152_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSonoxn source transparent/pattern opaque. */
static void rop3_152_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSonoxn source/pattern transparent. */
static void rop3_152_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSonoxn source/pattern opaque. */
static unsigned xrop3_152_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSonoxn source opaque/pattern transparent. */
static unsigned xrop3_152_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSonoxn source transparent/pattern opaque. */
static unsigned xrop3_152_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSonoxn source/pattern transparent. */
static unsigned xrop3_152_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSxn source/pattern opaque. */
static void rop3_153_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSxn source opaque/pattern transparent. */
static void rop3_153_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSxn source transparent/pattern opaque. */
static void rop3_153_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSxn source/pattern transparent. */
static void rop3_153_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D ^ S;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSxn source/pattern opaque. */
static unsigned xrop3_153_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D ^ S;
  stk1 = ~stk1;
  return stk1;
}

/* DSxn source opaque/pattern transparent. */
static unsigned xrop3_153_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ S;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSxn source transparent/pattern opaque. */
static unsigned xrop3_153_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D ^ S;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSxn source/pattern transparent. */
static unsigned xrop3_153_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D ^ S;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnax source/pattern opaque. */
static void rop3_154_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSnax source opaque/pattern transparent. */
static void rop3_154_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnax source transparent/pattern opaque. */
static void rop3_154_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnax source/pattern transparent. */
static void rop3_154_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnax source/pattern opaque. */
static unsigned xrop3_154_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSnax source opaque/pattern transparent. */
static unsigned xrop3_154_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnax source transparent/pattern opaque. */
static unsigned xrop3_154_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnax source/pattern transparent. */
static unsigned xrop3_154_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSoaxn source/pattern opaque. */
static void rop3_155_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSoaxn source opaque/pattern transparent. */
static void rop3_155_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSoaxn source transparent/pattern opaque. */
static void rop3_155_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSoaxn source/pattern transparent. */
static void rop3_155_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSoaxn source/pattern opaque. */
static unsigned xrop3_155_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSoaxn source opaque/pattern transparent. */
static unsigned xrop3_155_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSoaxn source transparent/pattern opaque. */
static unsigned xrop3_155_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSoaxn source/pattern transparent. */
static unsigned xrop3_155_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDnax source/pattern opaque. */
static void rop3_156_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDnax source opaque/pattern transparent. */
static void rop3_156_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDnax source transparent/pattern opaque. */
static void rop3_156_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDnax source/pattern transparent. */
static void rop3_156_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDnax source/pattern opaque. */
static unsigned xrop3_156_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDnax source opaque/pattern transparent. */
static unsigned xrop3_156_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDnax source transparent/pattern opaque. */
static unsigned xrop3_156_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDnax source/pattern transparent. */
static unsigned xrop3_156_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDoaxn source/pattern opaque. */
static void rop3_157_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDoaxn source opaque/pattern transparent. */
static void rop3_157_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDoaxn source transparent/pattern opaque. */
static void rop3_157_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDoaxn source/pattern transparent. */
static void rop3_157_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T | *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDoaxn source/pattern opaque. */
static unsigned xrop3_157_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDoaxn source opaque/pattern transparent. */
static unsigned xrop3_157_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDoaxn source transparent/pattern opaque. */
static unsigned xrop3_157_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDoaxn source/pattern transparent. */
static unsigned xrop3_157_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T | D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDSaoxx source/pattern opaque. */
static void rop3_158_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTDSaoxx source opaque/pattern transparent. */
static void rop3_158_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDSaoxx source transparent/pattern opaque. */
static void rop3_158_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDSaoxx source/pattern transparent. */
static void rop3_158_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDSaoxx source/pattern opaque. */
static unsigned xrop3_158_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTDSaoxx source opaque/pattern transparent. */
static unsigned xrop3_158_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDSaoxx source transparent/pattern opaque. */
static unsigned xrop3_158_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDSaoxx source/pattern transparent. */
static unsigned xrop3_158_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk3 = T | stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxan source/pattern opaque. */
static void rop3_159_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSxan source opaque/pattern transparent. */
static void rop3_159_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxan source transparent/pattern opaque. */
static void rop3_159_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxan source/pattern transparent. */
static void rop3_159_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxan source/pattern opaque. */
static unsigned xrop3_159_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSxan source opaque/pattern transparent. */
static unsigned xrop3_159_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxan source transparent/pattern opaque. */
static unsigned xrop3_159_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxan source/pattern transparent. */
static unsigned xrop3_159_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTa source/pattern opaque. */
static void rop3_160_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  *D = stk1;
}

/* DTa source opaque/pattern transparent. */
static void rop3_160_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTa source transparent/pattern opaque. */
static void rop3_160_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTa source/pattern transparent. */
static void rop3_160_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D & T;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTa source/pattern opaque. */
static unsigned xrop3_160_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  return stk1;
}

/* DTa source opaque/pattern transparent. */
static unsigned xrop3_160_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTa source transparent/pattern opaque. */
static unsigned xrop3_160_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  return (stk1 & (~S)) | (D & S);
}

/* DTa source/pattern transparent. */
static unsigned xrop3_160_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D & T;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTnaoxn source/pattern opaque. */
static void rop3_161_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTnaoxn source opaque/pattern transparent. */
static void rop3_161_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTnaoxn source transparent/pattern opaque. */
static void rop3_161_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTnaoxn source/pattern transparent. */
static void rop3_161_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTnaoxn source/pattern opaque. */
static unsigned xrop3_161_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTnaoxn source opaque/pattern transparent. */
static unsigned xrop3_161_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTnaoxn source transparent/pattern opaque. */
static unsigned xrop3_161_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTnaoxn source/pattern transparent. */
static unsigned xrop3_161_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~T;
  stk3 = S & stk4;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnoa source/pattern opaque. */
static void rop3_162_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSnoa source opaque/pattern transparent. */
static void rop3_162_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnoa source transparent/pattern opaque. */
static void rop3_162_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnoa source/pattern transparent. */
static void rop3_162_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnoa source/pattern opaque. */
static unsigned xrop3_162_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  return stk1;
}

/* DTSnoa source opaque/pattern transparent. */
static unsigned xrop3_162_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnoa source transparent/pattern opaque. */
static unsigned xrop3_162_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnoa source/pattern transparent. */
static unsigned xrop3_162_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDxoxn source/pattern opaque. */
static void rop3_163_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSDxoxn source opaque/pattern transparent. */
static void rop3_163_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDxoxn source transparent/pattern opaque. */
static void rop3_163_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDxoxn source/pattern transparent. */
static void rop3_163_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDxoxn source/pattern opaque. */
static unsigned xrop3_163_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSDxoxn source opaque/pattern transparent. */
static unsigned xrop3_163_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDxoxn source transparent/pattern opaque. */
static unsigned xrop3_163_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDxoxn source/pattern transparent. */
static unsigned xrop3_163_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTonoxn source/pattern opaque. */
static void rop3_164_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTonoxn source opaque/pattern transparent. */
static void rop3_164_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTonoxn source transparent/pattern opaque. */
static void rop3_164_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTonoxn source/pattern transparent. */
static void rop3_164_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTonoxn source/pattern opaque. */
static unsigned xrop3_164_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTonoxn source opaque/pattern transparent. */
static unsigned xrop3_164_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTonoxn source transparent/pattern opaque. */
static unsigned xrop3_164_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTonoxn source/pattern transparent. */
static unsigned xrop3_164_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk3 = ~stk3;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDxn source/pattern opaque. */
static void rop3_165_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ *D;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDxn source opaque/pattern transparent. */
static void rop3_165_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ *D;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDxn source transparent/pattern opaque. */
static void rop3_165_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ *D;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDxn source/pattern transparent. */
static void rop3_165_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ *D;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDxn source/pattern opaque. */
static unsigned xrop3_165_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ D;
  stk1 = ~stk1;
  return stk1;
}

/* TDxn source opaque/pattern transparent. */
static unsigned xrop3_165_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ D;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDxn source transparent/pattern opaque. */
static unsigned xrop3_165_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ D;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDxn source/pattern transparent. */
static unsigned xrop3_165_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ D;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTnax source/pattern opaque. */
static void rop3_166_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTnax source opaque/pattern transparent. */
static void rop3_166_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTnax source transparent/pattern opaque. */
static void rop3_166_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTnax source/pattern transparent. */
static void rop3_166_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTnax source/pattern opaque. */
static unsigned xrop3_166_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTnax source opaque/pattern transparent. */
static unsigned xrop3_166_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTnax source transparent/pattern opaque. */
static unsigned xrop3_166_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTnax source/pattern transparent. */
static unsigned xrop3_166_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSToaxn source/pattern opaque. */
static void rop3_167_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSToaxn source opaque/pattern transparent. */
static void rop3_167_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSToaxn source transparent/pattern opaque. */
static void rop3_167_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSToaxn source/pattern transparent. */
static void rop3_167_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSToaxn source/pattern opaque. */
static unsigned xrop3_167_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSToaxn source opaque/pattern transparent. */
static unsigned xrop3_167_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSToaxn source transparent/pattern opaque. */
static unsigned xrop3_167_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSToaxn source/pattern transparent. */
static unsigned xrop3_167_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSoa source/pattern opaque. */
static void rop3_168_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  *D = stk1;
}

/* DTSoa source opaque/pattern transparent. */
static void rop3_168_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSoa source transparent/pattern opaque. */
static void rop3_168_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSoa source/pattern transparent. */
static void rop3_168_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSoa source/pattern opaque. */
static unsigned xrop3_168_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  return stk1;
}

/* DTSoa source opaque/pattern transparent. */
static unsigned xrop3_168_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSoa source transparent/pattern opaque. */
static unsigned xrop3_168_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSoa source/pattern transparent. */
static unsigned xrop3_168_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSoxn source/pattern opaque. */
static void rop3_169_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSoxn source opaque/pattern transparent. */
static void rop3_169_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSoxn source transparent/pattern opaque. */
static void rop3_169_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSoxn source/pattern transparent. */
static void rop3_169_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSoxn source/pattern opaque. */
static unsigned xrop3_169_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSoxn source opaque/pattern transparent. */
static unsigned xrop3_169_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSoxn source transparent/pattern opaque. */
static unsigned xrop3_169_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSoxn source/pattern transparent. */
static unsigned xrop3_169_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* D source/pattern opaque. */
static void rop3_170_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
}

/* D source opaque/pattern transparent. */
static void rop3_170_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (*D & S) | (*D & (~T)) | (T & (~S) & *D);
}

/* D source transparent/pattern opaque. */
static void rop3_170_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (*D & (~S)) | (*D & S);
}

/* D source/pattern transparent. */
static void rop3_170_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (*D & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* D source/pattern opaque. */
static unsigned xrop3_170_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  return D;
}

/* D source opaque/pattern transparent. */
static unsigned xrop3_170_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (D & S) | (D & (~T)) | (T & (~S) & D);
}

/* D source transparent/pattern opaque. */
static unsigned xrop3_170_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  return (D & (~S)) | (D & S);
}

/* D source/pattern transparent. */
static unsigned xrop3_170_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (D & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSono source/pattern opaque. */
static void rop3_171_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSono source opaque/pattern transparent. */
static void rop3_171_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSono source transparent/pattern opaque. */
static void rop3_171_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSono source/pattern transparent. */
static void rop3_171_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSono source/pattern opaque. */
static unsigned xrop3_171_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return stk1;
}

/* DTSono source opaque/pattern transparent. */
static unsigned xrop3_171_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSono source transparent/pattern opaque. */
static unsigned xrop3_171_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSono source/pattern transparent. */
static unsigned xrop3_171_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSxax source/pattern opaque. */
static void rop3_172_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSxax source opaque/pattern transparent. */
static void rop3_172_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSxax source transparent/pattern opaque. */
static void rop3_172_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSxax source/pattern transparent. */
static void rop3_172_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSxax source/pattern opaque. */
static unsigned xrop3_172_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSxax source opaque/pattern transparent. */
static unsigned xrop3_172_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSxax source transparent/pattern opaque. */
static unsigned xrop3_172_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSxax source/pattern transparent. */
static unsigned xrop3_172_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDaoxn source/pattern opaque. */
static void rop3_173_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSDaoxn source opaque/pattern transparent. */
static void rop3_173_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDaoxn source transparent/pattern opaque. */
static void rop3_173_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDaoxn source/pattern transparent. */
static void rop3_173_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk2 = T | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDaoxn source/pattern opaque. */
static unsigned xrop3_173_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSDaoxn source opaque/pattern transparent. */
static unsigned xrop3_173_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDaoxn source transparent/pattern opaque. */
static unsigned xrop3_173_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDaoxn source/pattern transparent. */
static unsigned xrop3_173_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk2 = T | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTnao source/pattern opaque. */
static void rop3_174_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DSTnao source opaque/pattern transparent. */
static void rop3_174_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTnao source transparent/pattern opaque. */
static void rop3_174_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTnao source/pattern transparent. */
static void rop3_174_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTnao source/pattern opaque. */
static unsigned xrop3_174_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  return stk1;
}

/* DSTnao source opaque/pattern transparent. */
static unsigned xrop3_174_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTnao source transparent/pattern opaque. */
static unsigned xrop3_174_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTnao source/pattern transparent. */
static unsigned xrop3_174_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = S & stk3;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTno source/pattern opaque. */
static void rop3_175_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTno source opaque/pattern transparent. */
static void rop3_175_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTno source transparent/pattern opaque. */
static void rop3_175_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTno source/pattern transparent. */
static void rop3_175_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTno source/pattern opaque. */
static unsigned xrop3_175_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D | stk2;
  return stk1;
}

/* DTno source opaque/pattern transparent. */
static unsigned xrop3_175_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTno source transparent/pattern opaque. */
static unsigned xrop3_175_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTno source/pattern transparent. */
static unsigned xrop3_175_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSnoa source/pattern opaque. */
static void rop3_176_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDSnoa source opaque/pattern transparent. */
static void rop3_176_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSnoa source transparent/pattern opaque. */
static void rop3_176_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSnoa source/pattern transparent. */
static void rop3_176_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D | stk3;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSnoa source/pattern opaque. */
static unsigned xrop3_176_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  return stk1;
}

/* TDSnoa source opaque/pattern transparent. */
static unsigned xrop3_176_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSnoa source transparent/pattern opaque. */
static unsigned xrop3_176_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSnoa source/pattern transparent. */
static unsigned xrop3_176_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D | stk3;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTxoxn source/pattern opaque. */
static void rop3_177_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTxoxn source opaque/pattern transparent. */
static void rop3_177_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTxoxn source transparent/pattern opaque. */
static void rop3_177_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTxoxn source/pattern transparent. */
static void rop3_177_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTxoxn source/pattern opaque. */
static unsigned xrop3_177_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTxoxn source opaque/pattern transparent. */
static unsigned xrop3_177_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTxoxn source transparent/pattern opaque. */
static unsigned xrop3_177_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTxoxn source/pattern transparent. */
static unsigned xrop3_177_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSTxDSxox source/pattern opaque. */
static void rop3_178_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SSTxDSxox source opaque/pattern transparent. */
static void rop3_178_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSTxDSxox source transparent/pattern opaque. */
static void rop3_178_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSTxDSxox source/pattern transparent. */
static void rop3_178_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSTxDSxox source/pattern opaque. */
static unsigned xrop3_178_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SSTxDSxox source opaque/pattern transparent. */
static unsigned xrop3_178_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSTxDSxox source transparent/pattern opaque. */
static unsigned xrop3_178_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SSTxDSxox source/pattern transparent. */
static unsigned xrop3_178_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 | stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTanan source/pattern opaque. */
static void rop3_179_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTanan source opaque/pattern transparent. */
static void rop3_179_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTanan source transparent/pattern opaque. */
static void rop3_179_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTanan source/pattern transparent. */
static void rop3_179_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTanan source/pattern opaque. */
static unsigned xrop3_179_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTanan source opaque/pattern transparent. */
static unsigned xrop3_179_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTanan source transparent/pattern opaque. */
static unsigned xrop3_179_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTanan source/pattern transparent. */
static unsigned xrop3_179_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnax source/pattern opaque. */
static void rop3_180_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDnax source opaque/pattern transparent. */
static void rop3_180_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnax source transparent/pattern opaque. */
static void rop3_180_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnax source/pattern transparent. */
static void rop3_180_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnax source/pattern opaque. */
static unsigned xrop3_180_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDnax source opaque/pattern transparent. */
static unsigned xrop3_180_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnax source transparent/pattern opaque. */
static unsigned xrop3_180_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnax source/pattern transparent. */
static unsigned xrop3_180_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDoaxn source/pattern opaque. */
static void rop3_181_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSDoaxn source opaque/pattern transparent. */
static void rop3_181_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDoaxn source transparent/pattern opaque. */
static void rop3_181_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDoaxn source/pattern transparent. */
static void rop3_181_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S | *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDoaxn source/pattern opaque. */
static unsigned xrop3_181_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSDoaxn source opaque/pattern transparent. */
static unsigned xrop3_181_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDoaxn source transparent/pattern opaque. */
static unsigned xrop3_181_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDoaxn source/pattern transparent. */
static unsigned xrop3_181_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S | D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDTaoxx source/pattern opaque. */
static void rop3_182_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDTaoxx source opaque/pattern transparent. */
static void rop3_182_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDTaoxx source transparent/pattern opaque. */
static void rop3_182_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDTaoxx source/pattern transparent. */
static void rop3_182_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDTaoxx source/pattern opaque. */
static unsigned xrop3_182_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDTaoxx source opaque/pattern transparent. */
static unsigned xrop3_182_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDTaoxx source transparent/pattern opaque. */
static unsigned xrop3_182_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDTaoxx source/pattern transparent. */
static unsigned xrop3_182_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & T;
  stk3 = S | stk4;
  stk2 = T ^ stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxan source/pattern opaque. */
static void rop3_183_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTxan source opaque/pattern transparent. */
static void rop3_183_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxan source transparent/pattern opaque. */
static void rop3_183_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxan source/pattern transparent. */
static void rop3_183_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxan source/pattern opaque. */
static unsigned xrop3_183_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTxan source opaque/pattern transparent. */
static unsigned xrop3_183_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxan source transparent/pattern opaque. */
static unsigned xrop3_183_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxan source/pattern transparent. */
static unsigned xrop3_183_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTxax source/pattern opaque. */
static void rop3_184_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTxax source opaque/pattern transparent. */
static void rop3_184_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTxax source transparent/pattern opaque. */
static void rop3_184_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTxax source/pattern transparent. */
static void rop3_184_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTxax source/pattern opaque. */
static unsigned xrop3_184_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTxax source opaque/pattern transparent. */
static unsigned xrop3_184_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTxax source transparent/pattern opaque. */
static unsigned xrop3_184_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTxax source/pattern transparent. */
static unsigned xrop3_184_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDaoxn source/pattern opaque. */
static void rop3_185_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDaoxn source opaque/pattern transparent. */
static void rop3_185_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDaoxn source transparent/pattern opaque. */
static void rop3_185_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDaoxn source/pattern transparent. */
static void rop3_185_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & *D;
  stk2 = S | stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDaoxn source/pattern opaque. */
static unsigned xrop3_185_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDaoxn source opaque/pattern transparent. */
static unsigned xrop3_185_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDaoxn source transparent/pattern opaque. */
static unsigned xrop3_185_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDaoxn source/pattern transparent. */
static unsigned xrop3_185_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & D;
  stk2 = S | stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnao source/pattern opaque. */
static void rop3_186_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSnao source opaque/pattern transparent. */
static void rop3_186_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnao source transparent/pattern opaque. */
static void rop3_186_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnao source/pattern transparent. */
static void rop3_186_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnao source/pattern opaque. */
static unsigned xrop3_186_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  return stk1;
}

/* DTSnao source opaque/pattern transparent. */
static unsigned xrop3_186_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnao source transparent/pattern opaque. */
static unsigned xrop3_186_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnao source/pattern transparent. */
static unsigned xrop3_186_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T & stk3;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSno source/pattern opaque. */
static void rop3_187_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DSno source opaque/pattern transparent. */
static void rop3_187_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSno source transparent/pattern opaque. */
static void rop3_187_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSno source/pattern transparent. */
static void rop3_187_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSno source/pattern opaque. */
static unsigned xrop3_187_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D | stk2;
  return stk1;
}

/* DSno source opaque/pattern transparent. */
static unsigned xrop3_187_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSno source transparent/pattern opaque. */
static unsigned xrop3_187_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSno source/pattern transparent. */
static unsigned xrop3_187_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSanax source/pattern opaque. */
static void rop3_188_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* STDSanax source opaque/pattern transparent. */
static void rop3_188_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSanax source transparent/pattern opaque. */
static void rop3_188_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSanax source/pattern transparent. */
static void rop3_188_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSanax source/pattern opaque. */
static unsigned xrop3_188_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* STDSanax source opaque/pattern transparent. */
static unsigned xrop3_188_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSanax source transparent/pattern opaque. */
static unsigned xrop3_188_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDSanax source/pattern transparent. */
static unsigned xrop3_188_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDxTDxan source/pattern opaque. */
static void rop3_189_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDxTDxan source opaque/pattern transparent. */
static void rop3_189_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDxTDxan source transparent/pattern opaque. */
static void rop3_189_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDxTDxan source/pattern transparent. */
static void rop3_189_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ *D;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDxTDxan source/pattern opaque. */
static unsigned xrop3_189_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDxTDxan source opaque/pattern transparent. */
static unsigned xrop3_189_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDxTDxan source transparent/pattern opaque. */
static unsigned xrop3_189_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDxTDxan source/pattern transparent. */
static unsigned xrop3_189_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ D;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxo source/pattern opaque. */
static void rop3_190_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSxo source opaque/pattern transparent. */
static void rop3_190_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxo source transparent/pattern opaque. */
static void rop3_190_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxo source/pattern transparent. */
static void rop3_190_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxo source/pattern opaque. */
static unsigned xrop3_190_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  return stk1;
}

/* DTSxo source opaque/pattern transparent. */
static unsigned xrop3_190_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxo source transparent/pattern opaque. */
static unsigned xrop3_190_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxo source/pattern transparent. */
static unsigned xrop3_190_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSano source/pattern opaque. */
static void rop3_191_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSano source opaque/pattern transparent. */
static void rop3_191_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSano source transparent/pattern opaque. */
static void rop3_191_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSano source/pattern transparent. */
static void rop3_191_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSano source/pattern opaque. */
static unsigned xrop3_191_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return stk1;
}

/* DTSano source opaque/pattern transparent. */
static unsigned xrop3_191_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSano source transparent/pattern opaque. */
static unsigned xrop3_191_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSano source/pattern transparent. */
static unsigned xrop3_191_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSa source/pattern opaque. */
static void rop3_192_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  *D = stk1;
}

/* TSa source opaque/pattern transparent. */
static void rop3_192_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSa source transparent/pattern opaque. */
static void rop3_192_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSa source/pattern transparent. */
static void rop3_192_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T & S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSa source/pattern opaque. */
static unsigned xrop3_192_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  return stk1;
}

/* TSa source opaque/pattern transparent. */
static unsigned xrop3_192_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSa source transparent/pattern opaque. */
static unsigned xrop3_192_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  return (stk1 & (~S)) | (D & S);
}

/* TSa source/pattern transparent. */
static unsigned xrop3_192_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T & S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSnaoxn source/pattern opaque. */
static void rop3_193_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSnaoxn source opaque/pattern transparent. */
static void rop3_193_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSnaoxn source transparent/pattern opaque. */
static void rop3_193_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSnaoxn source/pattern transparent. */
static void rop3_193_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = ~S;
  stk3 = *D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSnaoxn source/pattern opaque. */
static unsigned xrop3_193_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSnaoxn source opaque/pattern transparent. */
static unsigned xrop3_193_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSnaoxn source transparent/pattern opaque. */
static unsigned xrop3_193_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSnaoxn source/pattern transparent. */
static unsigned xrop3_193_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = ~S;
  stk3 = D & stk4;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSonoxn source/pattern opaque. */
static void rop3_194_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSonoxn source opaque/pattern transparent. */
static void rop3_194_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSonoxn source transparent/pattern opaque. */
static void rop3_194_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSonoxn source/pattern transparent. */
static void rop3_194_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSonoxn source/pattern opaque. */
static unsigned xrop3_194_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSonoxn source opaque/pattern transparent. */
static unsigned xrop3_194_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSonoxn source transparent/pattern opaque. */
static unsigned xrop3_194_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSonoxn source/pattern transparent. */
static unsigned xrop3_194_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk3 = ~stk3;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSxn source/pattern opaque. */
static void rop3_195_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSxn source opaque/pattern transparent. */
static void rop3_195_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSxn source transparent/pattern opaque. */
static void rop3_195_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSxn source/pattern transparent. */
static void rop3_195_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSxn source/pattern opaque. */
static unsigned xrop3_195_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  return stk1;
}

/* TSxn source opaque/pattern transparent. */
static unsigned xrop3_195_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSxn source transparent/pattern opaque. */
static unsigned xrop3_195_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSxn source/pattern transparent. */
static unsigned xrop3_195_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T ^ S;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDnoa source/pattern opaque. */
static void rop3_196_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  *D = stk1;
}

/* STDnoa source opaque/pattern transparent. */
static void rop3_196_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDnoa source transparent/pattern opaque. */
static void rop3_196_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDnoa source/pattern transparent. */
static void rop3_196_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDnoa source/pattern opaque. */
static unsigned xrop3_196_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  return stk1;
}

/* STDnoa source opaque/pattern transparent. */
static unsigned xrop3_196_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDnoa source transparent/pattern opaque. */
static unsigned xrop3_196_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDnoa source/pattern transparent. */
static unsigned xrop3_196_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T | stk3;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSxoxn source/pattern opaque. */
static void rop3_197_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSxoxn source opaque/pattern transparent. */
static void rop3_197_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSxoxn source transparent/pattern opaque. */
static void rop3_197_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSxoxn source/pattern transparent. */
static void rop3_197_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSxoxn source/pattern opaque. */
static unsigned xrop3_197_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSxoxn source opaque/pattern transparent. */
static unsigned xrop3_197_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSxoxn source transparent/pattern opaque. */
static unsigned xrop3_197_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSxoxn source/pattern transparent. */
static unsigned xrop3_197_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnax source/pattern opaque. */
static void rop3_198_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTnax source opaque/pattern transparent. */
static void rop3_198_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnax source transparent/pattern opaque. */
static void rop3_198_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnax source/pattern transparent. */
static void rop3_198_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnax source/pattern opaque. */
static unsigned xrop3_198_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTnax source opaque/pattern transparent. */
static unsigned xrop3_198_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnax source transparent/pattern opaque. */
static unsigned xrop3_198_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnax source/pattern transparent. */
static unsigned xrop3_198_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDToaxn source/pattern opaque. */
static void rop3_199_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDToaxn source opaque/pattern transparent. */
static void rop3_199_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDToaxn source transparent/pattern opaque. */
static void rop3_199_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDToaxn source/pattern transparent. */
static void rop3_199_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDToaxn source/pattern opaque. */
static unsigned xrop3_199_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDToaxn source opaque/pattern transparent. */
static unsigned xrop3_199_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDToaxn source transparent/pattern opaque. */
static unsigned xrop3_199_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDToaxn source/pattern transparent. */
static unsigned xrop3_199_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | T;
  stk2 = S & stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDToa source/pattern opaque. */
static void rop3_200_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  *D = stk1;
}

/* SDToa source opaque/pattern transparent. */
static void rop3_200_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDToa source transparent/pattern opaque. */
static void rop3_200_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDToa source/pattern transparent. */
static void rop3_200_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk1 = S & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDToa source/pattern opaque. */
static unsigned xrop3_200_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  return stk1;
}

/* SDToa source opaque/pattern transparent. */
static unsigned xrop3_200_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDToa source transparent/pattern opaque. */
static unsigned xrop3_200_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDToa source/pattern transparent. */
static unsigned xrop3_200_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk1 = S & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDoxn source/pattern opaque. */
static void rop3_201_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDoxn source opaque/pattern transparent. */
static void rop3_201_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDoxn source transparent/pattern opaque. */
static void rop3_201_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDoxn source/pattern transparent. */
static void rop3_201_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | *D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDoxn source/pattern opaque. */
static unsigned xrop3_201_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDoxn source opaque/pattern transparent. */
static unsigned xrop3_201_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDoxn source transparent/pattern opaque. */
static unsigned xrop3_201_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDoxn source/pattern transparent. */
static unsigned xrop3_201_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | D;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDxax source/pattern opaque. */
static void rop3_202_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDxax source opaque/pattern transparent. */
static void rop3_202_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDxax source transparent/pattern opaque. */
static void rop3_202_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDxax source/pattern transparent. */
static void rop3_202_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ *D;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDxax source/pattern opaque. */
static unsigned xrop3_202_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDxax source opaque/pattern transparent. */
static unsigned xrop3_202_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDxax source transparent/pattern opaque. */
static unsigned xrop3_202_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDxax source/pattern transparent. */
static unsigned xrop3_202_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ D;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSaoxn source/pattern opaque. */
static void rop3_203_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSaoxn source opaque/pattern transparent. */
static void rop3_203_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSaoxn source transparent/pattern opaque. */
static void rop3_203_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSaoxn source/pattern transparent. */
static void rop3_203_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSaoxn source/pattern opaque. */
static unsigned xrop3_203_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSaoxn source opaque/pattern transparent. */
static unsigned xrop3_203_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSaoxn source transparent/pattern opaque. */
static unsigned xrop3_203_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSaoxn source/pattern transparent. */
static unsigned xrop3_203_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & S;
  stk2 = T | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* S source/pattern opaque. */
static void rop3_204_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = S;
}

/* S source opaque/pattern transparent. */
static void rop3_204_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (S & S) | (S & (~T)) | (T & (~S) & *D);
}

/* S source transparent/pattern opaque. */
static void rop3_204_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (S & (~S)) | (*D & S);
}

/* S source/pattern transparent. */
static void rop3_204_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (S & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* S source/pattern opaque. */
static unsigned xrop3_204_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  return S;
}

/* S source opaque/pattern transparent. */
static unsigned xrop3_204_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (S & S) | (S & (~T)) | (T & (~S) & D);
}

/* S source transparent/pattern opaque. */
static unsigned xrop3_204_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  return (S & (~S)) | (D & S);
}

/* S source/pattern transparent. */
static unsigned xrop3_204_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (S & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTono source/pattern opaque. */
static void rop3_205_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTono source opaque/pattern transparent. */
static void rop3_205_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTono source transparent/pattern opaque. */
static void rop3_205_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTono source/pattern transparent. */
static void rop3_205_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTono source/pattern opaque. */
static unsigned xrop3_205_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return stk1;
}

/* SDTono source opaque/pattern transparent. */
static unsigned xrop3_205_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTono source transparent/pattern opaque. */
static unsigned xrop3_205_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTono source/pattern transparent. */
static unsigned xrop3_205_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnao source/pattern opaque. */
static void rop3_206_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTnao source opaque/pattern transparent. */
static void rop3_206_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnao source transparent/pattern opaque. */
static void rop3_206_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnao source/pattern transparent. */
static void rop3_206_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D & stk3;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnao source/pattern opaque. */
static unsigned xrop3_206_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  return stk1;
}

/* SDTnao source opaque/pattern transparent. */
static unsigned xrop3_206_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnao source transparent/pattern opaque. */
static unsigned xrop3_206_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnao source/pattern transparent. */
static unsigned xrop3_206_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D & stk3;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STno source/pattern opaque. */
static void rop3_207_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  *D = stk1;
}

/* STno source opaque/pattern transparent. */
static void rop3_207_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STno source transparent/pattern opaque. */
static void rop3_207_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STno source/pattern transparent. */
static void rop3_207_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STno source/pattern opaque. */
static unsigned xrop3_207_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  return stk1;
}

/* STno source opaque/pattern transparent. */
static unsigned xrop3_207_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STno source transparent/pattern opaque. */
static unsigned xrop3_207_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STno source/pattern transparent. */
static unsigned xrop3_207_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~T;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnoa source/pattern opaque. */
static void rop3_208_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  *D = stk1;
}

/* TSDnoa source opaque/pattern transparent. */
static void rop3_208_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnoa source transparent/pattern opaque. */
static void rop3_208_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnoa source/pattern transparent. */
static void rop3_208_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnoa source/pattern opaque. */
static unsigned xrop3_208_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  return stk1;
}

/* TSDnoa source opaque/pattern transparent. */
static unsigned xrop3_208_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnoa source transparent/pattern opaque. */
static unsigned xrop3_208_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnoa source/pattern transparent. */
static unsigned xrop3_208_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTxoxn source/pattern opaque. */
static void rop3_209_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDTxoxn source opaque/pattern transparent. */
static void rop3_209_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTxoxn source transparent/pattern opaque. */
static void rop3_209_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTxoxn source/pattern transparent. */
static void rop3_209_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTxoxn source/pattern opaque. */
static unsigned xrop3_209_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDTxoxn source opaque/pattern transparent. */
static unsigned xrop3_209_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTxoxn source transparent/pattern opaque. */
static unsigned xrop3_209_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTxoxn source/pattern transparent. */
static unsigned xrop3_209_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D ^ T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSnax source/pattern opaque. */
static void rop3_210_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSnax source opaque/pattern transparent. */
static void rop3_210_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSnax source transparent/pattern opaque. */
static void rop3_210_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSnax source/pattern transparent. */
static void rop3_210_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSnax source/pattern opaque. */
static unsigned xrop3_210_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSnax source opaque/pattern transparent. */
static unsigned xrop3_210_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSnax source transparent/pattern opaque. */
static unsigned xrop3_210_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSnax source/pattern transparent. */
static unsigned xrop3_210_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDSoaxn source/pattern opaque. */
static void rop3_211_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STDSoaxn source opaque/pattern transparent. */
static void rop3_211_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDSoaxn source transparent/pattern opaque. */
static void rop3_211_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDSoaxn source/pattern transparent. */
static void rop3_211_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDSoaxn source/pattern opaque. */
static unsigned xrop3_211_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STDSoaxn source opaque/pattern transparent. */
static unsigned xrop3_211_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDSoaxn source transparent/pattern opaque. */
static unsigned xrop3_211_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STDSoaxn source/pattern transparent. */
static unsigned xrop3_211_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D | S;
  stk2 = T & stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSTxTDxax source/pattern opaque. */
static void rop3_212_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SSTxTDxax source opaque/pattern transparent. */
static void rop3_212_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSTxTDxax source transparent/pattern opaque. */
static void rop3_212_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSTxTDxax source/pattern transparent. */
static void rop3_212_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = T ^ *D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSTxTDxax source/pattern opaque. */
static unsigned xrop3_212_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SSTxTDxax source opaque/pattern transparent. */
static unsigned xrop3_212_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSTxTDxax source transparent/pattern opaque. */
static unsigned xrop3_212_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SSTxTDxax source/pattern transparent. */
static unsigned xrop3_212_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = T ^ D;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSanan source/pattern opaque. */
static void rop3_213_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSanan source opaque/pattern transparent. */
static void rop3_213_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSanan source transparent/pattern opaque. */
static void rop3_213_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSanan source/pattern transparent. */
static void rop3_213_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSanan source/pattern opaque. */
static unsigned xrop3_213_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSanan source opaque/pattern transparent. */
static unsigned xrop3_213_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSanan source transparent/pattern opaque. */
static unsigned xrop3_213_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSanan source/pattern transparent. */
static unsigned xrop3_213_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk2 = ~stk2;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTSaoxx source/pattern opaque. */
static void rop3_214_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TSDTSaoxx source opaque/pattern transparent. */
static void rop3_214_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTSaoxx source transparent/pattern opaque. */
static void rop3_214_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTSaoxx source/pattern transparent. */
static void rop3_214_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = T & S;
  stk3 = *D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTSaoxx source/pattern opaque. */
static unsigned xrop3_214_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TSDTSaoxx source opaque/pattern transparent. */
static unsigned xrop3_214_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTSaoxx source transparent/pattern opaque. */
static unsigned xrop3_214_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTSaoxx source/pattern transparent. */
static unsigned xrop3_214_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = T & S;
  stk3 = D | stk4;
  stk2 = S ^ stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxan source/pattern opaque. */
static void rop3_215_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DTSxan source opaque/pattern transparent. */
static void rop3_215_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxan source transparent/pattern opaque. */
static void rop3_215_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxan source/pattern transparent. */
static void rop3_215_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk1 = *D & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxan source/pattern opaque. */
static unsigned xrop3_215_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DTSxan source opaque/pattern transparent. */
static unsigned xrop3_215_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxan source transparent/pattern opaque. */
static unsigned xrop3_215_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxan source/pattern transparent. */
static unsigned xrop3_215_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk1 = D & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTxax source/pattern opaque. */
static void rop3_216_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = stk1;
}

/* TDSTxax source opaque/pattern transparent. */
static void rop3_216_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTxax source transparent/pattern opaque. */
static void rop3_216_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTxax source/pattern transparent. */
static void rop3_216_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S ^ T;
  stk2 = *D & stk3;
  stk1 = T ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTxax source/pattern opaque. */
static unsigned xrop3_216_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return stk1;
}

/* TDSTxax source opaque/pattern transparent. */
static unsigned xrop3_216_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTxax source transparent/pattern opaque. */
static unsigned xrop3_216_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTxax source/pattern transparent. */
static unsigned xrop3_216_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S ^ T;
  stk2 = D & stk3;
  stk1 = T ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSaoxn source/pattern opaque. */
static void rop3_217_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* SDTSaoxn source opaque/pattern transparent. */
static void rop3_217_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSaoxn source transparent/pattern opaque. */
static void rop3_217_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSaoxn source/pattern transparent. */
static void rop3_217_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk2 = *D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSaoxn source/pattern opaque. */
static unsigned xrop3_217_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* SDTSaoxn source opaque/pattern transparent. */
static unsigned xrop3_217_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSaoxn source transparent/pattern opaque. */
static unsigned xrop3_217_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSaoxn source/pattern transparent. */
static unsigned xrop3_217_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk2 = D | stk3;
  stk1 = S ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSDanax source/pattern opaque. */
static void rop3_218_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DTSDanax source opaque/pattern transparent. */
static void rop3_218_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSDanax source transparent/pattern opaque. */
static void rop3_218_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSDanax source/pattern transparent. */
static void rop3_218_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & *D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSDanax source/pattern opaque. */
static unsigned xrop3_218_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DTSDanax source opaque/pattern transparent. */
static unsigned xrop3_218_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSDanax source transparent/pattern opaque. */
static unsigned xrop3_218_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSDanax source/pattern transparent. */
static unsigned xrop3_218_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & D;
  stk3 = ~stk3;
  stk2 = T & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STxDSxan source/pattern opaque. */
static void rop3_219_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STxDSxan source opaque/pattern transparent. */
static void rop3_219_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STxDSxan source transparent/pattern opaque. */
static void rop3_219_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STxDSxan source/pattern transparent. */
static void rop3_219_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = *D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STxDSxan source/pattern opaque. */
static unsigned xrop3_219_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STxDSxan source opaque/pattern transparent. */
static unsigned xrop3_219_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STxDSxan source transparent/pattern opaque. */
static unsigned xrop3_219_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STxDSxan source/pattern transparent. */
static unsigned xrop3_219_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = D ^ S;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STDnao source/pattern opaque. */
static void rop3_220_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  *D = stk1;
}

/* STDnao source opaque/pattern transparent. */
static void rop3_220_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STDnao source transparent/pattern opaque. */
static void rop3_220_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STDnao source/pattern transparent. */
static void rop3_220_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STDnao source/pattern opaque. */
static unsigned xrop3_220_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  return stk1;
}

/* STDnao source opaque/pattern transparent. */
static unsigned xrop3_220_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STDnao source transparent/pattern opaque. */
static unsigned xrop3_220_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* STDnao source/pattern transparent. */
static unsigned xrop3_220_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = T & stk3;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDno source/pattern opaque. */
static void rop3_221_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDno source opaque/pattern transparent. */
static void rop3_221_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDno source transparent/pattern opaque. */
static void rop3_221_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDno source/pattern transparent. */
static void rop3_221_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDno source/pattern opaque. */
static unsigned xrop3_221_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S | stk2;
  return stk1;
}

/* SDno source opaque/pattern transparent. */
static unsigned xrop3_221_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDno source transparent/pattern opaque. */
static unsigned xrop3_221_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDno source/pattern transparent. */
static unsigned xrop3_221_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxo source/pattern opaque. */
static void rop3_222_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTxo source opaque/pattern transparent. */
static void rop3_222_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxo source transparent/pattern opaque. */
static void rop3_222_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxo source/pattern transparent. */
static void rop3_222_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxo source/pattern opaque. */
static unsigned xrop3_222_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  return stk1;
}

/* SDTxo source opaque/pattern transparent. */
static unsigned xrop3_222_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxo source transparent/pattern opaque. */
static unsigned xrop3_222_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxo source/pattern transparent. */
static unsigned xrop3_222_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTano source/pattern opaque. */
static void rop3_223_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTano source opaque/pattern transparent. */
static void rop3_223_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTano source transparent/pattern opaque. */
static void rop3_223_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTano source/pattern transparent. */
static void rop3_223_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTano source/pattern opaque. */
static unsigned xrop3_223_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return stk1;
}

/* SDTano source opaque/pattern transparent. */
static unsigned xrop3_223_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTano source transparent/pattern opaque. */
static unsigned xrop3_223_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTano source/pattern transparent. */
static unsigned xrop3_223_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSoa source/pattern opaque. */
static void rop3_224_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  *D = stk1;
}

/* TDSoa source opaque/pattern transparent. */
static void rop3_224_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSoa source transparent/pattern opaque. */
static void rop3_224_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSoa source/pattern transparent. */
static void rop3_224_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T & stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSoa source/pattern opaque. */
static unsigned xrop3_224_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  return stk1;
}

/* TDSoa source opaque/pattern transparent. */
static unsigned xrop3_224_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSoa source transparent/pattern opaque. */
static unsigned xrop3_224_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSoa source/pattern transparent. */
static unsigned xrop3_224_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T & stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSoxn source/pattern opaque. */
static void rop3_225_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSoxn source opaque/pattern transparent. */
static void rop3_225_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSoxn source transparent/pattern opaque. */
static void rop3_225_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSoxn source/pattern transparent. */
static void rop3_225_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSoxn source/pattern opaque. */
static unsigned xrop3_225_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSoxn source opaque/pattern transparent. */
static unsigned xrop3_225_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSoxn source transparent/pattern opaque. */
static unsigned xrop3_225_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSoxn source/pattern transparent. */
static unsigned xrop3_225_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDxax source/pattern opaque. */
static void rop3_226_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = stk1;
}

/* DSTDxax source opaque/pattern transparent. */
static void rop3_226_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDxax source transparent/pattern opaque. */
static void rop3_226_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDxax source/pattern transparent. */
static void rop3_226_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ *D;
  stk2 = S & stk3;
  stk1 = *D ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDxax source/pattern opaque. */
static unsigned xrop3_226_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return stk1;
}

/* DSTDxax source opaque/pattern transparent. */
static unsigned xrop3_226_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDxax source transparent/pattern opaque. */
static unsigned xrop3_226_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDxax source/pattern transparent. */
static unsigned xrop3_226_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ D;
  stk2 = S & stk3;
  stk1 = D ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDTaoxn source/pattern opaque. */
static void rop3_227_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TSDTaoxn source opaque/pattern transparent. */
static void rop3_227_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDTaoxn source transparent/pattern opaque. */
static void rop3_227_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDTaoxn source/pattern transparent. */
static void rop3_227_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = *D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDTaoxn source/pattern opaque. */
static unsigned xrop3_227_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TSDTaoxn source opaque/pattern transparent. */
static unsigned xrop3_227_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDTaoxn source transparent/pattern opaque. */
static unsigned xrop3_227_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TSDTaoxn source/pattern transparent. */
static unsigned xrop3_227_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = D & T;
  stk2 = S | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSxax source/pattern opaque. */
static void rop3_228_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSxax source opaque/pattern transparent. */
static void rop3_228_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSxax source transparent/pattern opaque. */
static void rop3_228_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSxax source/pattern transparent. */
static void rop3_228_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T ^ S;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSxax source/pattern opaque. */
static unsigned xrop3_228_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSxax source opaque/pattern transparent. */
static unsigned xrop3_228_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSxax source transparent/pattern opaque. */
static unsigned xrop3_228_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSxax source/pattern transparent. */
static unsigned xrop3_228_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T ^ S;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSTaoxn source/pattern opaque. */
static void rop3_229_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* TDSTaoxn source opaque/pattern transparent. */
static void rop3_229_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSTaoxn source transparent/pattern opaque. */
static void rop3_229_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSTaoxn source/pattern transparent. */
static void rop3_229_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = S & T;
  stk2 = *D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSTaoxn source/pattern opaque. */
static unsigned xrop3_229_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* TDSTaoxn source opaque/pattern transparent. */
static unsigned xrop3_229_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSTaoxn source transparent/pattern opaque. */
static unsigned xrop3_229_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* TDSTaoxn source/pattern transparent. */
static unsigned xrop3_229_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = S & T;
  stk2 = D | stk3;
  stk1 = T ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTSanax source/pattern opaque. */
static void rop3_230_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SDTSanax source opaque/pattern transparent. */
static void rop3_230_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTSanax source transparent/pattern opaque. */
static void rop3_230_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTSanax source/pattern transparent. */
static void rop3_230_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = *D & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTSanax source/pattern opaque. */
static unsigned xrop3_230_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SDTSanax source opaque/pattern transparent. */
static unsigned xrop3_230_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTSanax source transparent/pattern opaque. */
static unsigned xrop3_230_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTSanax source/pattern transparent. */
static unsigned xrop3_230_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = T & S;
  stk3 = ~stk3;
  stk2 = D & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* STxTDxan source/pattern opaque. */
static void rop3_231_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* STxTDxan source opaque/pattern transparent. */
static void rop3_231_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* STxTDxan source transparent/pattern opaque. */
static void rop3_231_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* STxTDxan source/pattern transparent. */
static void rop3_231_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk1 = S ^ T;
  stk2 = T ^ *D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* STxTDxan source/pattern opaque. */
static unsigned xrop3_231_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return stk1;
}

/* STxTDxan source opaque/pattern transparent. */
static unsigned xrop3_231_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* STxTDxan source transparent/pattern opaque. */
static unsigned xrop3_231_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* STxTDxan source/pattern transparent. */
static unsigned xrop3_231_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk1 = S ^ T;
  stk2 = T ^ D;
  stk1 = stk1 & stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SSTxDSxax source/pattern opaque. */
static void rop3_232_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = stk1;
}

/* SSTxDSxax source opaque/pattern transparent. */
static void rop3_232_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SSTxDSxax source transparent/pattern opaque. */
static void rop3_232_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SSTxDSxax source/pattern transparent. */
static void rop3_232_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk2 = S ^ T;
  stk3 = *D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SSTxDSxax source/pattern opaque. */
static unsigned xrop3_232_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return stk1;
}

/* SSTxDSxax source opaque/pattern transparent. */
static unsigned xrop3_232_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SSTxDSxax source transparent/pattern opaque. */
static unsigned xrop3_232_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SSTxDSxax source/pattern transparent. */
static unsigned xrop3_232_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk2 = S ^ T;
  stk3 = D ^ S;
  stk2 = stk2 & stk3;
  stk1 = S ^ stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSTDSanaxxn source/pattern opaque. */
static void rop3_233_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = stk1;
}

/* DSTDSanaxxn source opaque/pattern transparent. */
static void rop3_233_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSTDSanaxxn source transparent/pattern opaque. */
static void rop3_233_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSTDSanaxxn source/pattern transparent. */
static void rop3_233_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  unsigned char stk4;
  stk4 = *D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = *D ^ stk2;
  stk1 = ~stk1;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSTDSanaxxn source/pattern opaque. */
static unsigned xrop3_233_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return stk1;
}

/* DSTDSanaxxn source opaque/pattern transparent. */
static unsigned xrop3_233_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSTDSanaxxn source transparent/pattern opaque. */
static unsigned xrop3_233_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S)) | (D & S);
}

/* DSTDSanaxxn source/pattern transparent. */
static unsigned xrop3_233_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  unsigned stk4;
  stk4 = D & S;
  stk4 = ~stk4;
  stk3 = T & stk4;
  stk2 = S ^ stk3;
  stk1 = D ^ stk2;
  stk1 = ~stk1;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSao source/pattern opaque. */
static void rop3_234_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSao source opaque/pattern transparent. */
static void rop3_234_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSao source transparent/pattern opaque. */
static void rop3_234_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSao source/pattern transparent. */
static void rop3_234_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T & S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSao source/pattern opaque. */
static unsigned xrop3_234_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  return stk1;
}

/* DTSao source opaque/pattern transparent. */
static unsigned xrop3_234_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSao source transparent/pattern opaque. */
static unsigned xrop3_234_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSao source/pattern transparent. */
static unsigned xrop3_234_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T & S;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSxno source/pattern opaque. */
static void rop3_235_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSxno source opaque/pattern transparent. */
static void rop3_235_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSxno source transparent/pattern opaque. */
static void rop3_235_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSxno source/pattern transparent. */
static void rop3_235_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSxno source/pattern opaque. */
static unsigned xrop3_235_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return stk1;
}

/* DTSxno source opaque/pattern transparent. */
static unsigned xrop3_235_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSxno source transparent/pattern opaque. */
static unsigned xrop3_235_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSxno source/pattern transparent. */
static unsigned xrop3_235_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T ^ S;
  stk2 = ~stk2;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTao source/pattern opaque. */
static void rop3_236_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTao source opaque/pattern transparent. */
static void rop3_236_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTao source transparent/pattern opaque. */
static void rop3_236_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTao source/pattern transparent. */
static void rop3_236_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & T;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTao source/pattern opaque. */
static unsigned xrop3_236_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  return stk1;
}

/* SDTao source opaque/pattern transparent. */
static unsigned xrop3_236_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTao source transparent/pattern opaque. */
static unsigned xrop3_236_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTao source/pattern transparent. */
static unsigned xrop3_236_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & T;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTxno source/pattern opaque. */
static void rop3_237_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTxno source opaque/pattern transparent. */
static void rop3_237_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTxno source transparent/pattern opaque. */
static void rop3_237_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTxno source/pattern transparent. */
static void rop3_237_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTxno source/pattern opaque. */
static unsigned xrop3_237_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return stk1;
}

/* SDTxno source opaque/pattern transparent. */
static unsigned xrop3_237_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTxno source transparent/pattern opaque. */
static unsigned xrop3_237_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTxno source/pattern transparent. */
static unsigned xrop3_237_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ T;
  stk2 = ~stk2;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DSo source/pattern opaque. */
static void rop3_238_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  *D = stk1;
}

/* DSo source opaque/pattern transparent. */
static void rop3_238_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DSo source transparent/pattern opaque. */
static void rop3_238_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DSo source/pattern transparent. */
static void rop3_238_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DSo source/pattern opaque. */
static unsigned xrop3_238_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D | S;
  return stk1;
}

/* DSo source opaque/pattern transparent. */
static unsigned xrop3_238_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DSo source transparent/pattern opaque. */
static unsigned xrop3_238_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = D | S;
  return (stk1 & (~S)) | (D & S);
}

/* DSo source/pattern transparent. */
static unsigned xrop3_238_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* SDTnoo source/pattern opaque. */
static void rop3_239_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S | stk2;
  *D = stk1;
}

/* SDTnoo source opaque/pattern transparent. */
static void rop3_239_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* SDTnoo source transparent/pattern opaque. */
static void rop3_239_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* SDTnoo source/pattern transparent. */
static void rop3_239_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~T;
  stk2 = *D | stk3;
  stk1 = S | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* SDTnoo source/pattern opaque. */
static unsigned xrop3_239_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S | stk2;
  return stk1;
}

/* SDTnoo source opaque/pattern transparent. */
static unsigned xrop3_239_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* SDTnoo source transparent/pattern opaque. */
static unsigned xrop3_239_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* SDTnoo source/pattern transparent. */
static unsigned xrop3_239_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~T;
  stk2 = D | stk3;
  stk1 = S | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* T source/pattern opaque. */
static void rop3_240_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = T;
}

/* T source opaque/pattern transparent. */
static void rop3_240_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (T & S) | (T & (~T)) | (T & (~S) & *D);
}

/* T source transparent/pattern opaque. */
static void rop3_240_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (T & (~S)) | (*D & S);
}

/* T source/pattern transparent. */
static void rop3_240_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  *D = (T & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* T source/pattern opaque. */
static unsigned xrop3_240_0_0 (unsigned char s, unsigned char t)
{
  unsigned T = ((unsigned)t << 8) | t;
  return T;
}

/* T source opaque/pattern transparent. */
static unsigned xrop3_240_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (T & S) | (T & (~T)) | (T & (~S) & D);
}

/* T source transparent/pattern opaque. */
static unsigned xrop3_240_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (T & (~S)) | (D & S);
}

/* T source/pattern transparent. */
static unsigned xrop3_240_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  return (T & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSono source/pattern opaque. */
static void rop3_241_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDSono source opaque/pattern transparent. */
static void rop3_241_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSono source transparent/pattern opaque. */
static void rop3_241_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSono source/pattern transparent. */
static void rop3_241_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSono source/pattern opaque. */
static unsigned xrop3_241_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return stk1;
}

/* TDSono source opaque/pattern transparent. */
static unsigned xrop3_241_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSono source transparent/pattern opaque. */
static unsigned xrop3_241_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSono source/pattern transparent. */
static unsigned xrop3_241_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D | S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSnao source/pattern opaque. */
static void rop3_242_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDSnao source opaque/pattern transparent. */
static void rop3_242_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSnao source transparent/pattern opaque. */
static void rop3_242_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSnao source/pattern transparent. */
static void rop3_242_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = *D & stk3;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSnao source/pattern opaque. */
static unsigned xrop3_242_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  return stk1;
}

/* TDSnao source opaque/pattern transparent. */
static unsigned xrop3_242_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSnao source transparent/pattern opaque. */
static unsigned xrop3_242_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSnao source/pattern transparent. */
static unsigned xrop3_242_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = D & stk3;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSno source/pattern opaque. */
static void rop3_243_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  *D = stk1;
}

/* TSno source opaque/pattern transparent. */
static void rop3_243_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSno source transparent/pattern opaque. */
static void rop3_243_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSno source/pattern transparent. */
static void rop3_243_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSno source/pattern opaque. */
static unsigned xrop3_243_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  return stk1;
}

/* TSno source opaque/pattern transparent. */
static unsigned xrop3_243_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSno source transparent/pattern opaque. */
static unsigned xrop3_243_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSno source/pattern transparent. */
static unsigned xrop3_243_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~S;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnao source/pattern opaque. */
static void rop3_244_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  *D = stk1;
}

/* TSDnao source opaque/pattern transparent. */
static void rop3_244_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnao source transparent/pattern opaque. */
static void rop3_244_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnao source/pattern transparent. */
static void rop3_244_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnao source/pattern opaque. */
static unsigned xrop3_244_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  return stk1;
}

/* TSDnao source opaque/pattern transparent. */
static unsigned xrop3_244_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnao source transparent/pattern opaque. */
static unsigned xrop3_244_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnao source/pattern transparent. */
static unsigned xrop3_244_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S & stk3;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDno source/pattern opaque. */
static void rop3_245_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDno source opaque/pattern transparent. */
static void rop3_245_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDno source transparent/pattern opaque. */
static void rop3_245_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDno source/pattern transparent. */
static void rop3_245_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = ~*D;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDno source/pattern opaque. */
static unsigned xrop3_245_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T | stk2;
  return stk1;
}

/* TDno source opaque/pattern transparent. */
static unsigned xrop3_245_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDno source transparent/pattern opaque. */
static unsigned xrop3_245_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDno source/pattern transparent. */
static unsigned xrop3_245_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = ~D;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxo source/pattern opaque. */
static void rop3_246_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDSxo source opaque/pattern transparent. */
static void rop3_246_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxo source transparent/pattern opaque. */
static void rop3_246_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxo source/pattern transparent. */
static void rop3_246_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxo source/pattern opaque. */
static unsigned xrop3_246_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  return stk1;
}

/* TDSxo source opaque/pattern transparent. */
static unsigned xrop3_246_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxo source transparent/pattern opaque. */
static unsigned xrop3_246_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxo source/pattern transparent. */
static unsigned xrop3_246_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSano source/pattern opaque. */
static void rop3_247_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDSano source opaque/pattern transparent. */
static void rop3_247_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSano source transparent/pattern opaque. */
static void rop3_247_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSano source/pattern transparent. */
static void rop3_247_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSano source/pattern opaque. */
static unsigned xrop3_247_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return stk1;
}

/* TDSano source opaque/pattern transparent. */
static unsigned xrop3_247_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSano source transparent/pattern opaque. */
static unsigned xrop3_247_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSano source/pattern transparent. */
static unsigned xrop3_247_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSao source/pattern opaque. */
static void rop3_248_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDSao source opaque/pattern transparent. */
static void rop3_248_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSao source transparent/pattern opaque. */
static void rop3_248_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSao source/pattern transparent. */
static void rop3_248_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D & S;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSao source/pattern opaque. */
static unsigned xrop3_248_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  return stk1;
}

/* TDSao source opaque/pattern transparent. */
static unsigned xrop3_248_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSao source transparent/pattern opaque. */
static unsigned xrop3_248_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSao source/pattern transparent. */
static unsigned xrop3_248_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D & S;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TDSxno source/pattern opaque. */
static void rop3_249_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = stk1;
}

/* TDSxno source opaque/pattern transparent. */
static void rop3_249_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TDSxno source transparent/pattern opaque. */
static void rop3_249_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TDSxno source/pattern transparent. */
static void rop3_249_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = *D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TDSxno source/pattern opaque. */
static unsigned xrop3_249_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return stk1;
}

/* TDSxno source opaque/pattern transparent. */
static unsigned xrop3_249_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TDSxno source transparent/pattern opaque. */
static unsigned xrop3_249_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TDSxno source/pattern transparent. */
static unsigned xrop3_249_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = D ^ S;
  stk2 = ~stk2;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTo source/pattern opaque. */
static void rop3_250_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  *D = stk1;
}

/* DTo source opaque/pattern transparent. */
static void rop3_250_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTo source transparent/pattern opaque. */
static void rop3_250_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTo source/pattern transparent. */
static void rop3_250_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = *D | T;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTo source/pattern opaque. */
static unsigned xrop3_250_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  return stk1;
}

/* DTo source opaque/pattern transparent. */
static unsigned xrop3_250_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTo source transparent/pattern opaque. */
static unsigned xrop3_250_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  return (stk1 & (~S)) | (D & S);
}

/* DTo source/pattern transparent. */
static unsigned xrop3_250_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = D | T;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSnoo source/pattern opaque. */
static void rop3_251_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSnoo source opaque/pattern transparent. */
static void rop3_251_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSnoo source transparent/pattern opaque. */
static void rop3_251_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSnoo source/pattern transparent. */
static void rop3_251_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSnoo source/pattern opaque. */
static unsigned xrop3_251_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D | stk2;
  return stk1;
}

/* DTSnoo source opaque/pattern transparent. */
static unsigned xrop3_251_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSnoo source transparent/pattern opaque. */
static unsigned xrop3_251_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSnoo source/pattern transparent. */
static unsigned xrop3_251_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~S;
  stk2 = T | stk3;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSo source/pattern opaque. */
static void rop3_252_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  *D = stk1;
}

/* TSo source opaque/pattern transparent. */
static void rop3_252_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSo source transparent/pattern opaque. */
static void rop3_252_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSo source/pattern transparent. */
static void rop3_252_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = T | S;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSo source/pattern opaque. */
static unsigned xrop3_252_0_0 (unsigned char s, unsigned char t)
{
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  return stk1;
}

/* TSo source opaque/pattern transparent. */
static unsigned xrop3_252_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSo source transparent/pattern opaque. */
static unsigned xrop3_252_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  return (stk1 & (~S)) | (D & S);
}

/* TSo source/pattern transparent. */
static unsigned xrop3_252_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = T | S;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* TSDnoo source/pattern opaque. */
static void rop3_253_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  *D = stk1;
}

/* TSDnoo source opaque/pattern transparent. */
static void rop3_253_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* TSDnoo source transparent/pattern opaque. */
static void rop3_253_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* TSDnoo source/pattern transparent. */
static void rop3_253_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  unsigned char stk3;
  stk3 = ~*D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* TSDnoo source/pattern opaque. */
static unsigned xrop3_253_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  return stk1;
}

/* TSDnoo source opaque/pattern transparent. */
static unsigned xrop3_253_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* TSDnoo source transparent/pattern opaque. */
static unsigned xrop3_253_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* TSDnoo source/pattern transparent. */
static unsigned xrop3_253_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  unsigned stk3;
  stk3 = ~D;
  stk2 = S | stk3;
  stk1 = T | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* DTSoo source/pattern opaque. */
static void rop3_254_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  *D = stk1;
}

/* DTSoo source opaque/pattern transparent. */
static void rop3_254_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* DTSoo source transparent/pattern opaque. */
static void rop3_254_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S)) | (*D & S);
}

/* DTSoo source/pattern transparent. */
static void rop3_254_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  unsigned char stk2;
  stk2 = T | S;
  stk1 = *D | stk2;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* DTSoo source/pattern opaque. */
static unsigned xrop3_254_0_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  return stk1;
}

/* DTSoo source opaque/pattern transparent. */
static unsigned xrop3_254_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* DTSoo source transparent/pattern opaque. */
static unsigned xrop3_254_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  return (stk1 & (~S)) | (D & S);
}

/* DTSoo source/pattern transparent. */
static unsigned xrop3_254_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  unsigned stk2;
  stk2 = T | S;
  stk1 = D | stk2;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

/* 1 source/pattern opaque. */
static void rop3_255_0_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 255;
  *D = stk1;
}

/* 1 source opaque/pattern transparent. */
static void rop3_255_0_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 255;
  *D = (stk1 & S) | (stk1 & (~T)) | (T & (~S) & *D);
}

/* 1 source transparent/pattern opaque. */
static void rop3_255_1_0 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 255;
  *D = (stk1 & (~S)) | (*D & S);
}

/* 1 source/pattern transparent. */
static void rop3_255_1_1 (unsigned char *D, unsigned char S, unsigned char T)
{
  unsigned char stk1;
  stk1 = 255;
  *D = (stk1 & (~S) & (~T)) | (*D & S) | (*D & T);
}

/* 1 source/pattern opaque. */
static unsigned xrop3_255_0_0 (unsigned char s, unsigned char t)
{
  unsigned stk1;
  stk1 = 0xffff;
  return stk1;
}

/* 1 source opaque/pattern transparent. */
static unsigned xrop3_255_0_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = 0xffff;
  return (stk1 & S) | (stk1 & (~T)) | (T & (~S) & D);
}

/* 1 source transparent/pattern opaque. */
static unsigned xrop3_255_1_0 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned stk1;
  stk1 = 0xffff;
  return (stk1 & (~S)) | (D & S);
}

/* 1 source/pattern transparent. */
static unsigned xrop3_255_1_1 (unsigned char s, unsigned char t)
{
  unsigned D = 0x00ff;
  unsigned S = ((unsigned)s << 8) | s;
  unsigned T = ((unsigned)t << 8) | t;
  unsigned stk1;
  stk1 = 0xffff;
  return (stk1 & (~S) & (~T)) | (D & S) | (D & T);
}

static hpgs_rop3_func_t rop3_table[][2][2] = {
  {{rop3_0_0_0,rop3_0_0_1},{rop3_0_1_0,rop3_0_1_1}},
  {{rop3_1_0_0,rop3_1_0_1},{rop3_1_1_0,rop3_1_1_1}},
  {{rop3_2_0_0,rop3_2_0_1},{rop3_2_1_0,rop3_2_1_1}},
  {{rop3_3_0_0,rop3_3_0_1},{rop3_3_1_0,rop3_3_1_1}},
  {{rop3_4_0_0,rop3_4_0_1},{rop3_4_1_0,rop3_4_1_1}},
  {{rop3_5_0_0,rop3_5_0_1},{rop3_5_1_0,rop3_5_1_1}},
  {{rop3_6_0_0,rop3_6_0_1},{rop3_6_1_0,rop3_6_1_1}},
  {{rop3_7_0_0,rop3_7_0_1},{rop3_7_1_0,rop3_7_1_1}},
  {{rop3_8_0_0,rop3_8_0_1},{rop3_8_1_0,rop3_8_1_1}},
  {{rop3_9_0_0,rop3_9_0_1},{rop3_9_1_0,rop3_9_1_1}},
  {{rop3_10_0_0,rop3_10_0_1},{rop3_10_1_0,rop3_10_1_1}},
  {{rop3_11_0_0,rop3_11_0_1},{rop3_11_1_0,rop3_11_1_1}},
  {{rop3_12_0_0,rop3_12_0_1},{rop3_12_1_0,rop3_12_1_1}},
  {{rop3_13_0_0,rop3_13_0_1},{rop3_13_1_0,rop3_13_1_1}},
  {{rop3_14_0_0,rop3_14_0_1},{rop3_14_1_0,rop3_14_1_1}},
  {{rop3_15_0_0,rop3_15_0_1},{rop3_15_1_0,rop3_15_1_1}},
  {{rop3_16_0_0,rop3_16_0_1},{rop3_16_1_0,rop3_16_1_1}},
  {{rop3_17_0_0,rop3_17_0_1},{rop3_17_1_0,rop3_17_1_1}},
  {{rop3_18_0_0,rop3_18_0_1},{rop3_18_1_0,rop3_18_1_1}},
  {{rop3_19_0_0,rop3_19_0_1},{rop3_19_1_0,rop3_19_1_1}},
  {{rop3_20_0_0,rop3_20_0_1},{rop3_20_1_0,rop3_20_1_1}},
  {{rop3_21_0_0,rop3_21_0_1},{rop3_21_1_0,rop3_21_1_1}},
  {{rop3_22_0_0,rop3_22_0_1},{rop3_22_1_0,rop3_22_1_1}},
  {{rop3_23_0_0,rop3_23_0_1},{rop3_23_1_0,rop3_23_1_1}},
  {{rop3_24_0_0,rop3_24_0_1},{rop3_24_1_0,rop3_24_1_1}},
  {{rop3_25_0_0,rop3_25_0_1},{rop3_25_1_0,rop3_25_1_1}},
  {{rop3_26_0_0,rop3_26_0_1},{rop3_26_1_0,rop3_26_1_1}},
  {{rop3_27_0_0,rop3_27_0_1},{rop3_27_1_0,rop3_27_1_1}},
  {{rop3_28_0_0,rop3_28_0_1},{rop3_28_1_0,rop3_28_1_1}},
  {{rop3_29_0_0,rop3_29_0_1},{rop3_29_1_0,rop3_29_1_1}},
  {{rop3_30_0_0,rop3_30_0_1},{rop3_30_1_0,rop3_30_1_1}},
  {{rop3_31_0_0,rop3_31_0_1},{rop3_31_1_0,rop3_31_1_1}},
  {{rop3_32_0_0,rop3_32_0_1},{rop3_32_1_0,rop3_32_1_1}},
  {{rop3_33_0_0,rop3_33_0_1},{rop3_33_1_0,rop3_33_1_1}},
  {{rop3_34_0_0,rop3_34_0_1},{rop3_34_1_0,rop3_34_1_1}},
  {{rop3_35_0_0,rop3_35_0_1},{rop3_35_1_0,rop3_35_1_1}},
  {{rop3_36_0_0,rop3_36_0_1},{rop3_36_1_0,rop3_36_1_1}},
  {{rop3_37_0_0,rop3_37_0_1},{rop3_37_1_0,rop3_37_1_1}},
  {{rop3_38_0_0,rop3_38_0_1},{rop3_38_1_0,rop3_38_1_1}},
  {{rop3_39_0_0,rop3_39_0_1},{rop3_39_1_0,rop3_39_1_1}},
  {{rop3_40_0_0,rop3_40_0_1},{rop3_40_1_0,rop3_40_1_1}},
  {{rop3_41_0_0,rop3_41_0_1},{rop3_41_1_0,rop3_41_1_1}},
  {{rop3_42_0_0,rop3_42_0_1},{rop3_42_1_0,rop3_42_1_1}},
  {{rop3_43_0_0,rop3_43_0_1},{rop3_43_1_0,rop3_43_1_1}},
  {{rop3_44_0_0,rop3_44_0_1},{rop3_44_1_0,rop3_44_1_1}},
  {{rop3_45_0_0,rop3_45_0_1},{rop3_45_1_0,rop3_45_1_1}},
  {{rop3_46_0_0,rop3_46_0_1},{rop3_46_1_0,rop3_46_1_1}},
  {{rop3_47_0_0,rop3_47_0_1},{rop3_47_1_0,rop3_47_1_1}},
  {{rop3_48_0_0,rop3_48_0_1},{rop3_48_1_0,rop3_48_1_1}},
  {{rop3_49_0_0,rop3_49_0_1},{rop3_49_1_0,rop3_49_1_1}},
  {{rop3_50_0_0,rop3_50_0_1},{rop3_50_1_0,rop3_50_1_1}},
  {{rop3_51_0_0,rop3_51_0_1},{rop3_51_1_0,rop3_51_1_1}},
  {{rop3_52_0_0,rop3_52_0_1},{rop3_52_1_0,rop3_52_1_1}},
  {{rop3_53_0_0,rop3_53_0_1},{rop3_53_1_0,rop3_53_1_1}},
  {{rop3_54_0_0,rop3_54_0_1},{rop3_54_1_0,rop3_54_1_1}},
  {{rop3_55_0_0,rop3_55_0_1},{rop3_55_1_0,rop3_55_1_1}},
  {{rop3_56_0_0,rop3_56_0_1},{rop3_56_1_0,rop3_56_1_1}},
  {{rop3_57_0_0,rop3_57_0_1},{rop3_57_1_0,rop3_57_1_1}},
  {{rop3_58_0_0,rop3_58_0_1},{rop3_58_1_0,rop3_58_1_1}},
  {{rop3_59_0_0,rop3_59_0_1},{rop3_59_1_0,rop3_59_1_1}},
  {{rop3_60_0_0,rop3_60_0_1},{rop3_60_1_0,rop3_60_1_1}},
  {{rop3_61_0_0,rop3_61_0_1},{rop3_61_1_0,rop3_61_1_1}},
  {{rop3_62_0_0,rop3_62_0_1},{rop3_62_1_0,rop3_62_1_1}},
  {{rop3_63_0_0,rop3_63_0_1},{rop3_63_1_0,rop3_63_1_1}},
  {{rop3_64_0_0,rop3_64_0_1},{rop3_64_1_0,rop3_64_1_1}},
  {{rop3_65_0_0,rop3_65_0_1},{rop3_65_1_0,rop3_65_1_1}},
  {{rop3_66_0_0,rop3_66_0_1},{rop3_66_1_0,rop3_66_1_1}},
  {{rop3_67_0_0,rop3_67_0_1},{rop3_67_1_0,rop3_67_1_1}},
  {{rop3_68_0_0,rop3_68_0_1},{rop3_68_1_0,rop3_68_1_1}},
  {{rop3_69_0_0,rop3_69_0_1},{rop3_69_1_0,rop3_69_1_1}},
  {{rop3_70_0_0,rop3_70_0_1},{rop3_70_1_0,rop3_70_1_1}},
  {{rop3_71_0_0,rop3_71_0_1},{rop3_71_1_0,rop3_71_1_1}},
  {{rop3_72_0_0,rop3_72_0_1},{rop3_72_1_0,rop3_72_1_1}},
  {{rop3_73_0_0,rop3_73_0_1},{rop3_73_1_0,rop3_73_1_1}},
  {{rop3_74_0_0,rop3_74_0_1},{rop3_74_1_0,rop3_74_1_1}},
  {{rop3_75_0_0,rop3_75_0_1},{rop3_75_1_0,rop3_75_1_1}},
  {{rop3_76_0_0,rop3_76_0_1},{rop3_76_1_0,rop3_76_1_1}},
  {{rop3_77_0_0,rop3_77_0_1},{rop3_77_1_0,rop3_77_1_1}},
  {{rop3_78_0_0,rop3_78_0_1},{rop3_78_1_0,rop3_78_1_1}},
  {{rop3_79_0_0,rop3_79_0_1},{rop3_79_1_0,rop3_79_1_1}},
  {{rop3_80_0_0,rop3_80_0_1},{rop3_80_1_0,rop3_80_1_1}},
  {{rop3_81_0_0,rop3_81_0_1},{rop3_81_1_0,rop3_81_1_1}},
  {{rop3_82_0_0,rop3_82_0_1},{rop3_82_1_0,rop3_82_1_1}},
  {{rop3_83_0_0,rop3_83_0_1},{rop3_83_1_0,rop3_83_1_1}},
  {{rop3_84_0_0,rop3_84_0_1},{rop3_84_1_0,rop3_84_1_1}},
  {{rop3_85_0_0,rop3_85_0_1},{rop3_85_1_0,rop3_85_1_1}},
  {{rop3_86_0_0,rop3_86_0_1},{rop3_86_1_0,rop3_86_1_1}},
  {{rop3_87_0_0,rop3_87_0_1},{rop3_87_1_0,rop3_87_1_1}},
  {{rop3_88_0_0,rop3_88_0_1},{rop3_88_1_0,rop3_88_1_1}},
  {{rop3_89_0_0,rop3_89_0_1},{rop3_89_1_0,rop3_89_1_1}},
  {{rop3_90_0_0,rop3_90_0_1},{rop3_90_1_0,rop3_90_1_1}},
  {{rop3_91_0_0,rop3_91_0_1},{rop3_91_1_0,rop3_91_1_1}},
  {{rop3_92_0_0,rop3_92_0_1},{rop3_92_1_0,rop3_92_1_1}},
  {{rop3_93_0_0,rop3_93_0_1},{rop3_93_1_0,rop3_93_1_1}},
  {{rop3_94_0_0,rop3_94_0_1},{rop3_94_1_0,rop3_94_1_1}},
  {{rop3_95_0_0,rop3_95_0_1},{rop3_95_1_0,rop3_95_1_1}},
  {{rop3_96_0_0,rop3_96_0_1},{rop3_96_1_0,rop3_96_1_1}},
  {{rop3_97_0_0,rop3_97_0_1},{rop3_97_1_0,rop3_97_1_1}},
  {{rop3_98_0_0,rop3_98_0_1},{rop3_98_1_0,rop3_98_1_1}},
  {{rop3_99_0_0,rop3_99_0_1},{rop3_99_1_0,rop3_99_1_1}},
  {{rop3_100_0_0,rop3_100_0_1},{rop3_100_1_0,rop3_100_1_1}},
  {{rop3_101_0_0,rop3_101_0_1},{rop3_101_1_0,rop3_101_1_1}},
  {{rop3_102_0_0,rop3_102_0_1},{rop3_102_1_0,rop3_102_1_1}},
  {{rop3_103_0_0,rop3_103_0_1},{rop3_103_1_0,rop3_103_1_1}},
  {{rop3_104_0_0,rop3_104_0_1},{rop3_104_1_0,rop3_104_1_1}},
  {{rop3_105_0_0,rop3_105_0_1},{rop3_105_1_0,rop3_105_1_1}},
  {{rop3_106_0_0,rop3_106_0_1},{rop3_106_1_0,rop3_106_1_1}},
  {{rop3_107_0_0,rop3_107_0_1},{rop3_107_1_0,rop3_107_1_1}},
  {{rop3_108_0_0,rop3_108_0_1},{rop3_108_1_0,rop3_108_1_1}},
  {{rop3_109_0_0,rop3_109_0_1},{rop3_109_1_0,rop3_109_1_1}},
  {{rop3_110_0_0,rop3_110_0_1},{rop3_110_1_0,rop3_110_1_1}},
  {{rop3_111_0_0,rop3_111_0_1},{rop3_111_1_0,rop3_111_1_1}},
  {{rop3_112_0_0,rop3_112_0_1},{rop3_112_1_0,rop3_112_1_1}},
  {{rop3_113_0_0,rop3_113_0_1},{rop3_113_1_0,rop3_113_1_1}},
  {{rop3_114_0_0,rop3_114_0_1},{rop3_114_1_0,rop3_114_1_1}},
  {{rop3_115_0_0,rop3_115_0_1},{rop3_115_1_0,rop3_115_1_1}},
  {{rop3_116_0_0,rop3_116_0_1},{rop3_116_1_0,rop3_116_1_1}},
  {{rop3_117_0_0,rop3_117_0_1},{rop3_117_1_0,rop3_117_1_1}},
  {{rop3_118_0_0,rop3_118_0_1},{rop3_118_1_0,rop3_118_1_1}},
  {{rop3_119_0_0,rop3_119_0_1},{rop3_119_1_0,rop3_119_1_1}},
  {{rop3_120_0_0,rop3_120_0_1},{rop3_120_1_0,rop3_120_1_1}},
  {{rop3_121_0_0,rop3_121_0_1},{rop3_121_1_0,rop3_121_1_1}},
  {{rop3_122_0_0,rop3_122_0_1},{rop3_122_1_0,rop3_122_1_1}},
  {{rop3_123_0_0,rop3_123_0_1},{rop3_123_1_0,rop3_123_1_1}},
  {{rop3_124_0_0,rop3_124_0_1},{rop3_124_1_0,rop3_124_1_1}},
  {{rop3_125_0_0,rop3_125_0_1},{rop3_125_1_0,rop3_125_1_1}},
  {{rop3_126_0_0,rop3_126_0_1},{rop3_126_1_0,rop3_126_1_1}},
  {{rop3_127_0_0,rop3_127_0_1},{rop3_127_1_0,rop3_127_1_1}},
  {{rop3_128_0_0,rop3_128_0_1},{rop3_128_1_0,rop3_128_1_1}},
  {{rop3_129_0_0,rop3_129_0_1},{rop3_129_1_0,rop3_129_1_1}},
  {{rop3_130_0_0,rop3_130_0_1},{rop3_130_1_0,rop3_130_1_1}},
  {{rop3_131_0_0,rop3_131_0_1},{rop3_131_1_0,rop3_131_1_1}},
  {{rop3_132_0_0,rop3_132_0_1},{rop3_132_1_0,rop3_132_1_1}},
  {{rop3_133_0_0,rop3_133_0_1},{rop3_133_1_0,rop3_133_1_1}},
  {{rop3_134_0_0,rop3_134_0_1},{rop3_134_1_0,rop3_134_1_1}},
  {{rop3_135_0_0,rop3_135_0_1},{rop3_135_1_0,rop3_135_1_1}},
  {{rop3_136_0_0,rop3_136_0_1},{rop3_136_1_0,rop3_136_1_1}},
  {{rop3_137_0_0,rop3_137_0_1},{rop3_137_1_0,rop3_137_1_1}},
  {{rop3_138_0_0,rop3_138_0_1},{rop3_138_1_0,rop3_138_1_1}},
  {{rop3_139_0_0,rop3_139_0_1},{rop3_139_1_0,rop3_139_1_1}},
  {{rop3_140_0_0,rop3_140_0_1},{rop3_140_1_0,rop3_140_1_1}},
  {{rop3_141_0_0,rop3_141_0_1},{rop3_141_1_0,rop3_141_1_1}},
  {{rop3_142_0_0,rop3_142_0_1},{rop3_142_1_0,rop3_142_1_1}},
  {{rop3_143_0_0,rop3_143_0_1},{rop3_143_1_0,rop3_143_1_1}},
  {{rop3_144_0_0,rop3_144_0_1},{rop3_144_1_0,rop3_144_1_1}},
  {{rop3_145_0_0,rop3_145_0_1},{rop3_145_1_0,rop3_145_1_1}},
  {{rop3_146_0_0,rop3_146_0_1},{rop3_146_1_0,rop3_146_1_1}},
  {{rop3_147_0_0,rop3_147_0_1},{rop3_147_1_0,rop3_147_1_1}},
  {{rop3_148_0_0,rop3_148_0_1},{rop3_148_1_0,rop3_148_1_1}},
  {{rop3_149_0_0,rop3_149_0_1},{rop3_149_1_0,rop3_149_1_1}},
  {{rop3_150_0_0,rop3_150_0_1},{rop3_150_1_0,rop3_150_1_1}},
  {{rop3_151_0_0,rop3_151_0_1},{rop3_151_1_0,rop3_151_1_1}},
  {{rop3_152_0_0,rop3_152_0_1},{rop3_152_1_0,rop3_152_1_1}},
  {{rop3_153_0_0,rop3_153_0_1},{rop3_153_1_0,rop3_153_1_1}},
  {{rop3_154_0_0,rop3_154_0_1},{rop3_154_1_0,rop3_154_1_1}},
  {{rop3_155_0_0,rop3_155_0_1},{rop3_155_1_0,rop3_155_1_1}},
  {{rop3_156_0_0,rop3_156_0_1},{rop3_156_1_0,rop3_156_1_1}},
  {{rop3_157_0_0,rop3_157_0_1},{rop3_157_1_0,rop3_157_1_1}},
  {{rop3_158_0_0,rop3_158_0_1},{rop3_158_1_0,rop3_158_1_1}},
  {{rop3_159_0_0,rop3_159_0_1},{rop3_159_1_0,rop3_159_1_1}},
  {{rop3_160_0_0,rop3_160_0_1},{rop3_160_1_0,rop3_160_1_1}},
  {{rop3_161_0_0,rop3_161_0_1},{rop3_161_1_0,rop3_161_1_1}},
  {{rop3_162_0_0,rop3_162_0_1},{rop3_162_1_0,rop3_162_1_1}},
  {{rop3_163_0_0,rop3_163_0_1},{rop3_163_1_0,rop3_163_1_1}},
  {{rop3_164_0_0,rop3_164_0_1},{rop3_164_1_0,rop3_164_1_1}},
  {{rop3_165_0_0,rop3_165_0_1},{rop3_165_1_0,rop3_165_1_1}},
  {{rop3_166_0_0,rop3_166_0_1},{rop3_166_1_0,rop3_166_1_1}},
  {{rop3_167_0_0,rop3_167_0_1},{rop3_167_1_0,rop3_167_1_1}},
  {{rop3_168_0_0,rop3_168_0_1},{rop3_168_1_0,rop3_168_1_1}},
  {{rop3_169_0_0,rop3_169_0_1},{rop3_169_1_0,rop3_169_1_1}},
  {{rop3_170_0_0,rop3_170_0_1},{rop3_170_1_0,rop3_170_1_1}},
  {{rop3_171_0_0,rop3_171_0_1},{rop3_171_1_0,rop3_171_1_1}},
  {{rop3_172_0_0,rop3_172_0_1},{rop3_172_1_0,rop3_172_1_1}},
  {{rop3_173_0_0,rop3_173_0_1},{rop3_173_1_0,rop3_173_1_1}},
  {{rop3_174_0_0,rop3_174_0_1},{rop3_174_1_0,rop3_174_1_1}},
  {{rop3_175_0_0,rop3_175_0_1},{rop3_175_1_0,rop3_175_1_1}},
  {{rop3_176_0_0,rop3_176_0_1},{rop3_176_1_0,rop3_176_1_1}},
  {{rop3_177_0_0,rop3_177_0_1},{rop3_177_1_0,rop3_177_1_1}},
  {{rop3_178_0_0,rop3_178_0_1},{rop3_178_1_0,rop3_178_1_1}},
  {{rop3_179_0_0,rop3_179_0_1},{rop3_179_1_0,rop3_179_1_1}},
  {{rop3_180_0_0,rop3_180_0_1},{rop3_180_1_0,rop3_180_1_1}},
  {{rop3_181_0_0,rop3_181_0_1},{rop3_181_1_0,rop3_181_1_1}},
  {{rop3_182_0_0,rop3_182_0_1},{rop3_182_1_0,rop3_182_1_1}},
  {{rop3_183_0_0,rop3_183_0_1},{rop3_183_1_0,rop3_183_1_1}},
  {{rop3_184_0_0,rop3_184_0_1},{rop3_184_1_0,rop3_184_1_1}},
  {{rop3_185_0_0,rop3_185_0_1},{rop3_185_1_0,rop3_185_1_1}},
  {{rop3_186_0_0,rop3_186_0_1},{rop3_186_1_0,rop3_186_1_1}},
  {{rop3_187_0_0,rop3_187_0_1},{rop3_187_1_0,rop3_187_1_1}},
  {{rop3_188_0_0,rop3_188_0_1},{rop3_188_1_0,rop3_188_1_1}},
  {{rop3_189_0_0,rop3_189_0_1},{rop3_189_1_0,rop3_189_1_1}},
  {{rop3_190_0_0,rop3_190_0_1},{rop3_190_1_0,rop3_190_1_1}},
  {{rop3_191_0_0,rop3_191_0_1},{rop3_191_1_0,rop3_191_1_1}},
  {{rop3_192_0_0,rop3_192_0_1},{rop3_192_1_0,rop3_192_1_1}},
  {{rop3_193_0_0,rop3_193_0_1},{rop3_193_1_0,rop3_193_1_1}},
  {{rop3_194_0_0,rop3_194_0_1},{rop3_194_1_0,rop3_194_1_1}},
  {{rop3_195_0_0,rop3_195_0_1},{rop3_195_1_0,rop3_195_1_1}},
  {{rop3_196_0_0,rop3_196_0_1},{rop3_196_1_0,rop3_196_1_1}},
  {{rop3_197_0_0,rop3_197_0_1},{rop3_197_1_0,rop3_197_1_1}},
  {{rop3_198_0_0,rop3_198_0_1},{rop3_198_1_0,rop3_198_1_1}},
  {{rop3_199_0_0,rop3_199_0_1},{rop3_199_1_0,rop3_199_1_1}},
  {{rop3_200_0_0,rop3_200_0_1},{rop3_200_1_0,rop3_200_1_1}},
  {{rop3_201_0_0,rop3_201_0_1},{rop3_201_1_0,rop3_201_1_1}},
  {{rop3_202_0_0,rop3_202_0_1},{rop3_202_1_0,rop3_202_1_1}},
  {{rop3_203_0_0,rop3_203_0_1},{rop3_203_1_0,rop3_203_1_1}},
  {{rop3_204_0_0,rop3_204_0_1},{rop3_204_1_0,rop3_204_1_1}},
  {{rop3_205_0_0,rop3_205_0_1},{rop3_205_1_0,rop3_205_1_1}},
  {{rop3_206_0_0,rop3_206_0_1},{rop3_206_1_0,rop3_206_1_1}},
  {{rop3_207_0_0,rop3_207_0_1},{rop3_207_1_0,rop3_207_1_1}},
  {{rop3_208_0_0,rop3_208_0_1},{rop3_208_1_0,rop3_208_1_1}},
  {{rop3_209_0_0,rop3_209_0_1},{rop3_209_1_0,rop3_209_1_1}},
  {{rop3_210_0_0,rop3_210_0_1},{rop3_210_1_0,rop3_210_1_1}},
  {{rop3_211_0_0,rop3_211_0_1},{rop3_211_1_0,rop3_211_1_1}},
  {{rop3_212_0_0,rop3_212_0_1},{rop3_212_1_0,rop3_212_1_1}},
  {{rop3_213_0_0,rop3_213_0_1},{rop3_213_1_0,rop3_213_1_1}},
  {{rop3_214_0_0,rop3_214_0_1},{rop3_214_1_0,rop3_214_1_1}},
  {{rop3_215_0_0,rop3_215_0_1},{rop3_215_1_0,rop3_215_1_1}},
  {{rop3_216_0_0,rop3_216_0_1},{rop3_216_1_0,rop3_216_1_1}},
  {{rop3_217_0_0,rop3_217_0_1},{rop3_217_1_0,rop3_217_1_1}},
  {{rop3_218_0_0,rop3_218_0_1},{rop3_218_1_0,rop3_218_1_1}},
  {{rop3_219_0_0,rop3_219_0_1},{rop3_219_1_0,rop3_219_1_1}},
  {{rop3_220_0_0,rop3_220_0_1},{rop3_220_1_0,rop3_220_1_1}},
  {{rop3_221_0_0,rop3_221_0_1},{rop3_221_1_0,rop3_221_1_1}},
  {{rop3_222_0_0,rop3_222_0_1},{rop3_222_1_0,rop3_222_1_1}},
  {{rop3_223_0_0,rop3_223_0_1},{rop3_223_1_0,rop3_223_1_1}},
  {{rop3_224_0_0,rop3_224_0_1},{rop3_224_1_0,rop3_224_1_1}},
  {{rop3_225_0_0,rop3_225_0_1},{rop3_225_1_0,rop3_225_1_1}},
  {{rop3_226_0_0,rop3_226_0_1},{rop3_226_1_0,rop3_226_1_1}},
  {{rop3_227_0_0,rop3_227_0_1},{rop3_227_1_0,rop3_227_1_1}},
  {{rop3_228_0_0,rop3_228_0_1},{rop3_228_1_0,rop3_228_1_1}},
  {{rop3_229_0_0,rop3_229_0_1},{rop3_229_1_0,rop3_229_1_1}},
  {{rop3_230_0_0,rop3_230_0_1},{rop3_230_1_0,rop3_230_1_1}},
  {{rop3_231_0_0,rop3_231_0_1},{rop3_231_1_0,rop3_231_1_1}},
  {{rop3_232_0_0,rop3_232_0_1},{rop3_232_1_0,rop3_232_1_1}},
  {{rop3_233_0_0,rop3_233_0_1},{rop3_233_1_0,rop3_233_1_1}},
  {{rop3_234_0_0,rop3_234_0_1},{rop3_234_1_0,rop3_234_1_1}},
  {{rop3_235_0_0,rop3_235_0_1},{rop3_235_1_0,rop3_235_1_1}},
  {{rop3_236_0_0,rop3_236_0_1},{rop3_236_1_0,rop3_236_1_1}},
  {{rop3_237_0_0,rop3_237_0_1},{rop3_237_1_0,rop3_237_1_1}},
  {{rop3_238_0_0,rop3_238_0_1},{rop3_238_1_0,rop3_238_1_1}},
  {{rop3_239_0_0,rop3_239_0_1},{rop3_239_1_0,rop3_239_1_1}},
  {{rop3_240_0_0,rop3_240_0_1},{rop3_240_1_0,rop3_240_1_1}},
  {{rop3_241_0_0,rop3_241_0_1},{rop3_241_1_0,rop3_241_1_1}},
  {{rop3_242_0_0,rop3_242_0_1},{rop3_242_1_0,rop3_242_1_1}},
  {{rop3_243_0_0,rop3_243_0_1},{rop3_243_1_0,rop3_243_1_1}},
  {{rop3_244_0_0,rop3_244_0_1},{rop3_244_1_0,rop3_244_1_1}},
  {{rop3_245_0_0,rop3_245_0_1},{rop3_245_1_0,rop3_245_1_1}},
  {{rop3_246_0_0,rop3_246_0_1},{rop3_246_1_0,rop3_246_1_1}},
  {{rop3_247_0_0,rop3_247_0_1},{rop3_247_1_0,rop3_247_1_1}},
  {{rop3_248_0_0,rop3_248_0_1},{rop3_248_1_0,rop3_248_1_1}},
  {{rop3_249_0_0,rop3_249_0_1},{rop3_249_1_0,rop3_249_1_1}},
  {{rop3_250_0_0,rop3_250_0_1},{rop3_250_1_0,rop3_250_1_1}},
  {{rop3_251_0_0,rop3_251_0_1},{rop3_251_1_0,rop3_251_1_1}},
  {{rop3_252_0_0,rop3_252_0_1},{rop3_252_1_0,rop3_252_1_1}},
  {{rop3_253_0_0,rop3_253_0_1},{rop3_253_1_0,rop3_253_1_1}},
  {{rop3_254_0_0,rop3_254_0_1},{rop3_254_1_0,rop3_254_1_1}},
  {{rop3_255_0_0,rop3_255_0_1},{rop3_255_1_0,rop3_255_1_1}}
};

hpgs_rop3_func_t hpgs_rop3_func(int rop3,
                                hpgs_bool src_transparency,
                                hpgs_bool pattern_transparency)
{
  if (rop3 < 0 || rop3 >= 256) return 0;
  return rop3_table[rop3][src_transparency!=0][pattern_transparency!=0];
}
static hpgs_xrop3_func_t xrop3_table[][2][2] = {
  {{xrop3_0_0_0,xrop3_0_0_1},{xrop3_0_1_0,xrop3_0_1_1}},
  {{xrop3_1_0_0,xrop3_1_0_1},{xrop3_1_1_0,xrop3_1_1_1}},
  {{xrop3_2_0_0,xrop3_2_0_1},{xrop3_2_1_0,xrop3_2_1_1}},
  {{xrop3_3_0_0,xrop3_3_0_1},{xrop3_3_1_0,xrop3_3_1_1}},
  {{xrop3_4_0_0,xrop3_4_0_1},{xrop3_4_1_0,xrop3_4_1_1}},
  {{xrop3_5_0_0,xrop3_5_0_1},{xrop3_5_1_0,xrop3_5_1_1}},
  {{xrop3_6_0_0,xrop3_6_0_1},{xrop3_6_1_0,xrop3_6_1_1}},
  {{xrop3_7_0_0,xrop3_7_0_1},{xrop3_7_1_0,xrop3_7_1_1}},
  {{xrop3_8_0_0,xrop3_8_0_1},{xrop3_8_1_0,xrop3_8_1_1}},
  {{xrop3_9_0_0,xrop3_9_0_1},{xrop3_9_1_0,xrop3_9_1_1}},
  {{xrop3_10_0_0,xrop3_10_0_1},{xrop3_10_1_0,xrop3_10_1_1}},
  {{xrop3_11_0_0,xrop3_11_0_1},{xrop3_11_1_0,xrop3_11_1_1}},
  {{xrop3_12_0_0,xrop3_12_0_1},{xrop3_12_1_0,xrop3_12_1_1}},
  {{xrop3_13_0_0,xrop3_13_0_1},{xrop3_13_1_0,xrop3_13_1_1}},
  {{xrop3_14_0_0,xrop3_14_0_1},{xrop3_14_1_0,xrop3_14_1_1}},
  {{xrop3_15_0_0,xrop3_15_0_1},{xrop3_15_1_0,xrop3_15_1_1}},
  {{xrop3_16_0_0,xrop3_16_0_1},{xrop3_16_1_0,xrop3_16_1_1}},
  {{xrop3_17_0_0,xrop3_17_0_1},{xrop3_17_1_0,xrop3_17_1_1}},
  {{xrop3_18_0_0,xrop3_18_0_1},{xrop3_18_1_0,xrop3_18_1_1}},
  {{xrop3_19_0_0,xrop3_19_0_1},{xrop3_19_1_0,xrop3_19_1_1}},
  {{xrop3_20_0_0,xrop3_20_0_1},{xrop3_20_1_0,xrop3_20_1_1}},
  {{xrop3_21_0_0,xrop3_21_0_1},{xrop3_21_1_0,xrop3_21_1_1}},
  {{xrop3_22_0_0,xrop3_22_0_1},{xrop3_22_1_0,xrop3_22_1_1}},
  {{xrop3_23_0_0,xrop3_23_0_1},{xrop3_23_1_0,xrop3_23_1_1}},
  {{xrop3_24_0_0,xrop3_24_0_1},{xrop3_24_1_0,xrop3_24_1_1}},
  {{xrop3_25_0_0,xrop3_25_0_1},{xrop3_25_1_0,xrop3_25_1_1}},
  {{xrop3_26_0_0,xrop3_26_0_1},{xrop3_26_1_0,xrop3_26_1_1}},
  {{xrop3_27_0_0,xrop3_27_0_1},{xrop3_27_1_0,xrop3_27_1_1}},
  {{xrop3_28_0_0,xrop3_28_0_1},{xrop3_28_1_0,xrop3_28_1_1}},
  {{xrop3_29_0_0,xrop3_29_0_1},{xrop3_29_1_0,xrop3_29_1_1}},
  {{xrop3_30_0_0,xrop3_30_0_1},{xrop3_30_1_0,xrop3_30_1_1}},
  {{xrop3_31_0_0,xrop3_31_0_1},{xrop3_31_1_0,xrop3_31_1_1}},
  {{xrop3_32_0_0,xrop3_32_0_1},{xrop3_32_1_0,xrop3_32_1_1}},
  {{xrop3_33_0_0,xrop3_33_0_1},{xrop3_33_1_0,xrop3_33_1_1}},
  {{xrop3_34_0_0,xrop3_34_0_1},{xrop3_34_1_0,xrop3_34_1_1}},
  {{xrop3_35_0_0,xrop3_35_0_1},{xrop3_35_1_0,xrop3_35_1_1}},
  {{xrop3_36_0_0,xrop3_36_0_1},{xrop3_36_1_0,xrop3_36_1_1}},
  {{xrop3_37_0_0,xrop3_37_0_1},{xrop3_37_1_0,xrop3_37_1_1}},
  {{xrop3_38_0_0,xrop3_38_0_1},{xrop3_38_1_0,xrop3_38_1_1}},
  {{xrop3_39_0_0,xrop3_39_0_1},{xrop3_39_1_0,xrop3_39_1_1}},
  {{xrop3_40_0_0,xrop3_40_0_1},{xrop3_40_1_0,xrop3_40_1_1}},
  {{xrop3_41_0_0,xrop3_41_0_1},{xrop3_41_1_0,xrop3_41_1_1}},
  {{xrop3_42_0_0,xrop3_42_0_1},{xrop3_42_1_0,xrop3_42_1_1}},
  {{xrop3_43_0_0,xrop3_43_0_1},{xrop3_43_1_0,xrop3_43_1_1}},
  {{xrop3_44_0_0,xrop3_44_0_1},{xrop3_44_1_0,xrop3_44_1_1}},
  {{xrop3_45_0_0,xrop3_45_0_1},{xrop3_45_1_0,xrop3_45_1_1}},
  {{xrop3_46_0_0,xrop3_46_0_1},{xrop3_46_1_0,xrop3_46_1_1}},
  {{xrop3_47_0_0,xrop3_47_0_1},{xrop3_47_1_0,xrop3_47_1_1}},
  {{xrop3_48_0_0,xrop3_48_0_1},{xrop3_48_1_0,xrop3_48_1_1}},
  {{xrop3_49_0_0,xrop3_49_0_1},{xrop3_49_1_0,xrop3_49_1_1}},
  {{xrop3_50_0_0,xrop3_50_0_1},{xrop3_50_1_0,xrop3_50_1_1}},
  {{xrop3_51_0_0,xrop3_51_0_1},{xrop3_51_1_0,xrop3_51_1_1}},
  {{xrop3_52_0_0,xrop3_52_0_1},{xrop3_52_1_0,xrop3_52_1_1}},
  {{xrop3_53_0_0,xrop3_53_0_1},{xrop3_53_1_0,xrop3_53_1_1}},
  {{xrop3_54_0_0,xrop3_54_0_1},{xrop3_54_1_0,xrop3_54_1_1}},
  {{xrop3_55_0_0,xrop3_55_0_1},{xrop3_55_1_0,xrop3_55_1_1}},
  {{xrop3_56_0_0,xrop3_56_0_1},{xrop3_56_1_0,xrop3_56_1_1}},
  {{xrop3_57_0_0,xrop3_57_0_1},{xrop3_57_1_0,xrop3_57_1_1}},
  {{xrop3_58_0_0,xrop3_58_0_1},{xrop3_58_1_0,xrop3_58_1_1}},
  {{xrop3_59_0_0,xrop3_59_0_1},{xrop3_59_1_0,xrop3_59_1_1}},
  {{xrop3_60_0_0,xrop3_60_0_1},{xrop3_60_1_0,xrop3_60_1_1}},
  {{xrop3_61_0_0,xrop3_61_0_1},{xrop3_61_1_0,xrop3_61_1_1}},
  {{xrop3_62_0_0,xrop3_62_0_1},{xrop3_62_1_0,xrop3_62_1_1}},
  {{xrop3_63_0_0,xrop3_63_0_1},{xrop3_63_1_0,xrop3_63_1_1}},
  {{xrop3_64_0_0,xrop3_64_0_1},{xrop3_64_1_0,xrop3_64_1_1}},
  {{xrop3_65_0_0,xrop3_65_0_1},{xrop3_65_1_0,xrop3_65_1_1}},
  {{xrop3_66_0_0,xrop3_66_0_1},{xrop3_66_1_0,xrop3_66_1_1}},
  {{xrop3_67_0_0,xrop3_67_0_1},{xrop3_67_1_0,xrop3_67_1_1}},
  {{xrop3_68_0_0,xrop3_68_0_1},{xrop3_68_1_0,xrop3_68_1_1}},
  {{xrop3_69_0_0,xrop3_69_0_1},{xrop3_69_1_0,xrop3_69_1_1}},
  {{xrop3_70_0_0,xrop3_70_0_1},{xrop3_70_1_0,xrop3_70_1_1}},
  {{xrop3_71_0_0,xrop3_71_0_1},{xrop3_71_1_0,xrop3_71_1_1}},
  {{xrop3_72_0_0,xrop3_72_0_1},{xrop3_72_1_0,xrop3_72_1_1}},
  {{xrop3_73_0_0,xrop3_73_0_1},{xrop3_73_1_0,xrop3_73_1_1}},
  {{xrop3_74_0_0,xrop3_74_0_1},{xrop3_74_1_0,xrop3_74_1_1}},
  {{xrop3_75_0_0,xrop3_75_0_1},{xrop3_75_1_0,xrop3_75_1_1}},
  {{xrop3_76_0_0,xrop3_76_0_1},{xrop3_76_1_0,xrop3_76_1_1}},
  {{xrop3_77_0_0,xrop3_77_0_1},{xrop3_77_1_0,xrop3_77_1_1}},
  {{xrop3_78_0_0,xrop3_78_0_1},{xrop3_78_1_0,xrop3_78_1_1}},
  {{xrop3_79_0_0,xrop3_79_0_1},{xrop3_79_1_0,xrop3_79_1_1}},
  {{xrop3_80_0_0,xrop3_80_0_1},{xrop3_80_1_0,xrop3_80_1_1}},
  {{xrop3_81_0_0,xrop3_81_0_1},{xrop3_81_1_0,xrop3_81_1_1}},
  {{xrop3_82_0_0,xrop3_82_0_1},{xrop3_82_1_0,xrop3_82_1_1}},
  {{xrop3_83_0_0,xrop3_83_0_1},{xrop3_83_1_0,xrop3_83_1_1}},
  {{xrop3_84_0_0,xrop3_84_0_1},{xrop3_84_1_0,xrop3_84_1_1}},
  {{xrop3_85_0_0,xrop3_85_0_1},{xrop3_85_1_0,xrop3_85_1_1}},
  {{xrop3_86_0_0,xrop3_86_0_1},{xrop3_86_1_0,xrop3_86_1_1}},
  {{xrop3_87_0_0,xrop3_87_0_1},{xrop3_87_1_0,xrop3_87_1_1}},
  {{xrop3_88_0_0,xrop3_88_0_1},{xrop3_88_1_0,xrop3_88_1_1}},
  {{xrop3_89_0_0,xrop3_89_0_1},{xrop3_89_1_0,xrop3_89_1_1}},
  {{xrop3_90_0_0,xrop3_90_0_1},{xrop3_90_1_0,xrop3_90_1_1}},
  {{xrop3_91_0_0,xrop3_91_0_1},{xrop3_91_1_0,xrop3_91_1_1}},
  {{xrop3_92_0_0,xrop3_92_0_1},{xrop3_92_1_0,xrop3_92_1_1}},
  {{xrop3_93_0_0,xrop3_93_0_1},{xrop3_93_1_0,xrop3_93_1_1}},
  {{xrop3_94_0_0,xrop3_94_0_1},{xrop3_94_1_0,xrop3_94_1_1}},
  {{xrop3_95_0_0,xrop3_95_0_1},{xrop3_95_1_0,xrop3_95_1_1}},
  {{xrop3_96_0_0,xrop3_96_0_1},{xrop3_96_1_0,xrop3_96_1_1}},
  {{xrop3_97_0_0,xrop3_97_0_1},{xrop3_97_1_0,xrop3_97_1_1}},
  {{xrop3_98_0_0,xrop3_98_0_1},{xrop3_98_1_0,xrop3_98_1_1}},
  {{xrop3_99_0_0,xrop3_99_0_1},{xrop3_99_1_0,xrop3_99_1_1}},
  {{xrop3_100_0_0,xrop3_100_0_1},{xrop3_100_1_0,xrop3_100_1_1}},
  {{xrop3_101_0_0,xrop3_101_0_1},{xrop3_101_1_0,xrop3_101_1_1}},
  {{xrop3_102_0_0,xrop3_102_0_1},{xrop3_102_1_0,xrop3_102_1_1}},
  {{xrop3_103_0_0,xrop3_103_0_1},{xrop3_103_1_0,xrop3_103_1_1}},
  {{xrop3_104_0_0,xrop3_104_0_1},{xrop3_104_1_0,xrop3_104_1_1}},
  {{xrop3_105_0_0,xrop3_105_0_1},{xrop3_105_1_0,xrop3_105_1_1}},
  {{xrop3_106_0_0,xrop3_106_0_1},{xrop3_106_1_0,xrop3_106_1_1}},
  {{xrop3_107_0_0,xrop3_107_0_1},{xrop3_107_1_0,xrop3_107_1_1}},
  {{xrop3_108_0_0,xrop3_108_0_1},{xrop3_108_1_0,xrop3_108_1_1}},
  {{xrop3_109_0_0,xrop3_109_0_1},{xrop3_109_1_0,xrop3_109_1_1}},
  {{xrop3_110_0_0,xrop3_110_0_1},{xrop3_110_1_0,xrop3_110_1_1}},
  {{xrop3_111_0_0,xrop3_111_0_1},{xrop3_111_1_0,xrop3_111_1_1}},
  {{xrop3_112_0_0,xrop3_112_0_1},{xrop3_112_1_0,xrop3_112_1_1}},
  {{xrop3_113_0_0,xrop3_113_0_1},{xrop3_113_1_0,xrop3_113_1_1}},
  {{xrop3_114_0_0,xrop3_114_0_1},{xrop3_114_1_0,xrop3_114_1_1}},
  {{xrop3_115_0_0,xrop3_115_0_1},{xrop3_115_1_0,xrop3_115_1_1}},
  {{xrop3_116_0_0,xrop3_116_0_1},{xrop3_116_1_0,xrop3_116_1_1}},
  {{xrop3_117_0_0,xrop3_117_0_1},{xrop3_117_1_0,xrop3_117_1_1}},
  {{xrop3_118_0_0,xrop3_118_0_1},{xrop3_118_1_0,xrop3_118_1_1}},
  {{xrop3_119_0_0,xrop3_119_0_1},{xrop3_119_1_0,xrop3_119_1_1}},
  {{xrop3_120_0_0,xrop3_120_0_1},{xrop3_120_1_0,xrop3_120_1_1}},
  {{xrop3_121_0_0,xrop3_121_0_1},{xrop3_121_1_0,xrop3_121_1_1}},
  {{xrop3_122_0_0,xrop3_122_0_1},{xrop3_122_1_0,xrop3_122_1_1}},
  {{xrop3_123_0_0,xrop3_123_0_1},{xrop3_123_1_0,xrop3_123_1_1}},
  {{xrop3_124_0_0,xrop3_124_0_1},{xrop3_124_1_0,xrop3_124_1_1}},
  {{xrop3_125_0_0,xrop3_125_0_1},{xrop3_125_1_0,xrop3_125_1_1}},
  {{xrop3_126_0_0,xrop3_126_0_1},{xrop3_126_1_0,xrop3_126_1_1}},
  {{xrop3_127_0_0,xrop3_127_0_1},{xrop3_127_1_0,xrop3_127_1_1}},
  {{xrop3_128_0_0,xrop3_128_0_1},{xrop3_128_1_0,xrop3_128_1_1}},
  {{xrop3_129_0_0,xrop3_129_0_1},{xrop3_129_1_0,xrop3_129_1_1}},
  {{xrop3_130_0_0,xrop3_130_0_1},{xrop3_130_1_0,xrop3_130_1_1}},
  {{xrop3_131_0_0,xrop3_131_0_1},{xrop3_131_1_0,xrop3_131_1_1}},
  {{xrop3_132_0_0,xrop3_132_0_1},{xrop3_132_1_0,xrop3_132_1_1}},
  {{xrop3_133_0_0,xrop3_133_0_1},{xrop3_133_1_0,xrop3_133_1_1}},
  {{xrop3_134_0_0,xrop3_134_0_1},{xrop3_134_1_0,xrop3_134_1_1}},
  {{xrop3_135_0_0,xrop3_135_0_1},{xrop3_135_1_0,xrop3_135_1_1}},
  {{xrop3_136_0_0,xrop3_136_0_1},{xrop3_136_1_0,xrop3_136_1_1}},
  {{xrop3_137_0_0,xrop3_137_0_1},{xrop3_137_1_0,xrop3_137_1_1}},
  {{xrop3_138_0_0,xrop3_138_0_1},{xrop3_138_1_0,xrop3_138_1_1}},
  {{xrop3_139_0_0,xrop3_139_0_1},{xrop3_139_1_0,xrop3_139_1_1}},
  {{xrop3_140_0_0,xrop3_140_0_1},{xrop3_140_1_0,xrop3_140_1_1}},
  {{xrop3_141_0_0,xrop3_141_0_1},{xrop3_141_1_0,xrop3_141_1_1}},
  {{xrop3_142_0_0,xrop3_142_0_1},{xrop3_142_1_0,xrop3_142_1_1}},
  {{xrop3_143_0_0,xrop3_143_0_1},{xrop3_143_1_0,xrop3_143_1_1}},
  {{xrop3_144_0_0,xrop3_144_0_1},{xrop3_144_1_0,xrop3_144_1_1}},
  {{xrop3_145_0_0,xrop3_145_0_1},{xrop3_145_1_0,xrop3_145_1_1}},
  {{xrop3_146_0_0,xrop3_146_0_1},{xrop3_146_1_0,xrop3_146_1_1}},
  {{xrop3_147_0_0,xrop3_147_0_1},{xrop3_147_1_0,xrop3_147_1_1}},
  {{xrop3_148_0_0,xrop3_148_0_1},{xrop3_148_1_0,xrop3_148_1_1}},
  {{xrop3_149_0_0,xrop3_149_0_1},{xrop3_149_1_0,xrop3_149_1_1}},
  {{xrop3_150_0_0,xrop3_150_0_1},{xrop3_150_1_0,xrop3_150_1_1}},
  {{xrop3_151_0_0,xrop3_151_0_1},{xrop3_151_1_0,xrop3_151_1_1}},
  {{xrop3_152_0_0,xrop3_152_0_1},{xrop3_152_1_0,xrop3_152_1_1}},
  {{xrop3_153_0_0,xrop3_153_0_1},{xrop3_153_1_0,xrop3_153_1_1}},
  {{xrop3_154_0_0,xrop3_154_0_1},{xrop3_154_1_0,xrop3_154_1_1}},
  {{xrop3_155_0_0,xrop3_155_0_1},{xrop3_155_1_0,xrop3_155_1_1}},
  {{xrop3_156_0_0,xrop3_156_0_1},{xrop3_156_1_0,xrop3_156_1_1}},
  {{xrop3_157_0_0,xrop3_157_0_1},{xrop3_157_1_0,xrop3_157_1_1}},
  {{xrop3_158_0_0,xrop3_158_0_1},{xrop3_158_1_0,xrop3_158_1_1}},
  {{xrop3_159_0_0,xrop3_159_0_1},{xrop3_159_1_0,xrop3_159_1_1}},
  {{xrop3_160_0_0,xrop3_160_0_1},{xrop3_160_1_0,xrop3_160_1_1}},
  {{xrop3_161_0_0,xrop3_161_0_1},{xrop3_161_1_0,xrop3_161_1_1}},
  {{xrop3_162_0_0,xrop3_162_0_1},{xrop3_162_1_0,xrop3_162_1_1}},
  {{xrop3_163_0_0,xrop3_163_0_1},{xrop3_163_1_0,xrop3_163_1_1}},
  {{xrop3_164_0_0,xrop3_164_0_1},{xrop3_164_1_0,xrop3_164_1_1}},
  {{xrop3_165_0_0,xrop3_165_0_1},{xrop3_165_1_0,xrop3_165_1_1}},
  {{xrop3_166_0_0,xrop3_166_0_1},{xrop3_166_1_0,xrop3_166_1_1}},
  {{xrop3_167_0_0,xrop3_167_0_1},{xrop3_167_1_0,xrop3_167_1_1}},
  {{xrop3_168_0_0,xrop3_168_0_1},{xrop3_168_1_0,xrop3_168_1_1}},
  {{xrop3_169_0_0,xrop3_169_0_1},{xrop3_169_1_0,xrop3_169_1_1}},
  {{xrop3_170_0_0,xrop3_170_0_1},{xrop3_170_1_0,xrop3_170_1_1}},
  {{xrop3_171_0_0,xrop3_171_0_1},{xrop3_171_1_0,xrop3_171_1_1}},
  {{xrop3_172_0_0,xrop3_172_0_1},{xrop3_172_1_0,xrop3_172_1_1}},
  {{xrop3_173_0_0,xrop3_173_0_1},{xrop3_173_1_0,xrop3_173_1_1}},
  {{xrop3_174_0_0,xrop3_174_0_1},{xrop3_174_1_0,xrop3_174_1_1}},
  {{xrop3_175_0_0,xrop3_175_0_1},{xrop3_175_1_0,xrop3_175_1_1}},
  {{xrop3_176_0_0,xrop3_176_0_1},{xrop3_176_1_0,xrop3_176_1_1}},
  {{xrop3_177_0_0,xrop3_177_0_1},{xrop3_177_1_0,xrop3_177_1_1}},
  {{xrop3_178_0_0,xrop3_178_0_1},{xrop3_178_1_0,xrop3_178_1_1}},
  {{xrop3_179_0_0,xrop3_179_0_1},{xrop3_179_1_0,xrop3_179_1_1}},
  {{xrop3_180_0_0,xrop3_180_0_1},{xrop3_180_1_0,xrop3_180_1_1}},
  {{xrop3_181_0_0,xrop3_181_0_1},{xrop3_181_1_0,xrop3_181_1_1}},
  {{xrop3_182_0_0,xrop3_182_0_1},{xrop3_182_1_0,xrop3_182_1_1}},
  {{xrop3_183_0_0,xrop3_183_0_1},{xrop3_183_1_0,xrop3_183_1_1}},
  {{xrop3_184_0_0,xrop3_184_0_1},{xrop3_184_1_0,xrop3_184_1_1}},
  {{xrop3_185_0_0,xrop3_185_0_1},{xrop3_185_1_0,xrop3_185_1_1}},
  {{xrop3_186_0_0,xrop3_186_0_1},{xrop3_186_1_0,xrop3_186_1_1}},
  {{xrop3_187_0_0,xrop3_187_0_1},{xrop3_187_1_0,xrop3_187_1_1}},
  {{xrop3_188_0_0,xrop3_188_0_1},{xrop3_188_1_0,xrop3_188_1_1}},
  {{xrop3_189_0_0,xrop3_189_0_1},{xrop3_189_1_0,xrop3_189_1_1}},
  {{xrop3_190_0_0,xrop3_190_0_1},{xrop3_190_1_0,xrop3_190_1_1}},
  {{xrop3_191_0_0,xrop3_191_0_1},{xrop3_191_1_0,xrop3_191_1_1}},
  {{xrop3_192_0_0,xrop3_192_0_1},{xrop3_192_1_0,xrop3_192_1_1}},
  {{xrop3_193_0_0,xrop3_193_0_1},{xrop3_193_1_0,xrop3_193_1_1}},
  {{xrop3_194_0_0,xrop3_194_0_1},{xrop3_194_1_0,xrop3_194_1_1}},
  {{xrop3_195_0_0,xrop3_195_0_1},{xrop3_195_1_0,xrop3_195_1_1}},
  {{xrop3_196_0_0,xrop3_196_0_1},{xrop3_196_1_0,xrop3_196_1_1}},
  {{xrop3_197_0_0,xrop3_197_0_1},{xrop3_197_1_0,xrop3_197_1_1}},
  {{xrop3_198_0_0,xrop3_198_0_1},{xrop3_198_1_0,xrop3_198_1_1}},
  {{xrop3_199_0_0,xrop3_199_0_1},{xrop3_199_1_0,xrop3_199_1_1}},
  {{xrop3_200_0_0,xrop3_200_0_1},{xrop3_200_1_0,xrop3_200_1_1}},
  {{xrop3_201_0_0,xrop3_201_0_1},{xrop3_201_1_0,xrop3_201_1_1}},
  {{xrop3_202_0_0,xrop3_202_0_1},{xrop3_202_1_0,xrop3_202_1_1}},
  {{xrop3_203_0_0,xrop3_203_0_1},{xrop3_203_1_0,xrop3_203_1_1}},
  {{xrop3_204_0_0,xrop3_204_0_1},{xrop3_204_1_0,xrop3_204_1_1}},
  {{xrop3_205_0_0,xrop3_205_0_1},{xrop3_205_1_0,xrop3_205_1_1}},
  {{xrop3_206_0_0,xrop3_206_0_1},{xrop3_206_1_0,xrop3_206_1_1}},
  {{xrop3_207_0_0,xrop3_207_0_1},{xrop3_207_1_0,xrop3_207_1_1}},
  {{xrop3_208_0_0,xrop3_208_0_1},{xrop3_208_1_0,xrop3_208_1_1}},
  {{xrop3_209_0_0,xrop3_209_0_1},{xrop3_209_1_0,xrop3_209_1_1}},
  {{xrop3_210_0_0,xrop3_210_0_1},{xrop3_210_1_0,xrop3_210_1_1}},
  {{xrop3_211_0_0,xrop3_211_0_1},{xrop3_211_1_0,xrop3_211_1_1}},
  {{xrop3_212_0_0,xrop3_212_0_1},{xrop3_212_1_0,xrop3_212_1_1}},
  {{xrop3_213_0_0,xrop3_213_0_1},{xrop3_213_1_0,xrop3_213_1_1}},
  {{xrop3_214_0_0,xrop3_214_0_1},{xrop3_214_1_0,xrop3_214_1_1}},
  {{xrop3_215_0_0,xrop3_215_0_1},{xrop3_215_1_0,xrop3_215_1_1}},
  {{xrop3_216_0_0,xrop3_216_0_1},{xrop3_216_1_0,xrop3_216_1_1}},
  {{xrop3_217_0_0,xrop3_217_0_1},{xrop3_217_1_0,xrop3_217_1_1}},
  {{xrop3_218_0_0,xrop3_218_0_1},{xrop3_218_1_0,xrop3_218_1_1}},
  {{xrop3_219_0_0,xrop3_219_0_1},{xrop3_219_1_0,xrop3_219_1_1}},
  {{xrop3_220_0_0,xrop3_220_0_1},{xrop3_220_1_0,xrop3_220_1_1}},
  {{xrop3_221_0_0,xrop3_221_0_1},{xrop3_221_1_0,xrop3_221_1_1}},
  {{xrop3_222_0_0,xrop3_222_0_1},{xrop3_222_1_0,xrop3_222_1_1}},
  {{xrop3_223_0_0,xrop3_223_0_1},{xrop3_223_1_0,xrop3_223_1_1}},
  {{xrop3_224_0_0,xrop3_224_0_1},{xrop3_224_1_0,xrop3_224_1_1}},
  {{xrop3_225_0_0,xrop3_225_0_1},{xrop3_225_1_0,xrop3_225_1_1}},
  {{xrop3_226_0_0,xrop3_226_0_1},{xrop3_226_1_0,xrop3_226_1_1}},
  {{xrop3_227_0_0,xrop3_227_0_1},{xrop3_227_1_0,xrop3_227_1_1}},
  {{xrop3_228_0_0,xrop3_228_0_1},{xrop3_228_1_0,xrop3_228_1_1}},
  {{xrop3_229_0_0,xrop3_229_0_1},{xrop3_229_1_0,xrop3_229_1_1}},
  {{xrop3_230_0_0,xrop3_230_0_1},{xrop3_230_1_0,xrop3_230_1_1}},
  {{xrop3_231_0_0,xrop3_231_0_1},{xrop3_231_1_0,xrop3_231_1_1}},
  {{xrop3_232_0_0,xrop3_232_0_1},{xrop3_232_1_0,xrop3_232_1_1}},
  {{xrop3_233_0_0,xrop3_233_0_1},{xrop3_233_1_0,xrop3_233_1_1}},
  {{xrop3_234_0_0,xrop3_234_0_1},{xrop3_234_1_0,xrop3_234_1_1}},
  {{xrop3_235_0_0,xrop3_235_0_1},{xrop3_235_1_0,xrop3_235_1_1}},
  {{xrop3_236_0_0,xrop3_236_0_1},{xrop3_236_1_0,xrop3_236_1_1}},
  {{xrop3_237_0_0,xrop3_237_0_1},{xrop3_237_1_0,xrop3_237_1_1}},
  {{xrop3_238_0_0,xrop3_238_0_1},{xrop3_238_1_0,xrop3_238_1_1}},
  {{xrop3_239_0_0,xrop3_239_0_1},{xrop3_239_1_0,xrop3_239_1_1}},
  {{xrop3_240_0_0,xrop3_240_0_1},{xrop3_240_1_0,xrop3_240_1_1}},
  {{xrop3_241_0_0,xrop3_241_0_1},{xrop3_241_1_0,xrop3_241_1_1}},
  {{xrop3_242_0_0,xrop3_242_0_1},{xrop3_242_1_0,xrop3_242_1_1}},
  {{xrop3_243_0_0,xrop3_243_0_1},{xrop3_243_1_0,xrop3_243_1_1}},
  {{xrop3_244_0_0,xrop3_244_0_1},{xrop3_244_1_0,xrop3_244_1_1}},
  {{xrop3_245_0_0,xrop3_245_0_1},{xrop3_245_1_0,xrop3_245_1_1}},
  {{xrop3_246_0_0,xrop3_246_0_1},{xrop3_246_1_0,xrop3_246_1_1}},
  {{xrop3_247_0_0,xrop3_247_0_1},{xrop3_247_1_0,xrop3_247_1_1}},
  {{xrop3_248_0_0,xrop3_248_0_1},{xrop3_248_1_0,xrop3_248_1_1}},
  {{xrop3_249_0_0,xrop3_249_0_1},{xrop3_249_1_0,xrop3_249_1_1}},
  {{xrop3_250_0_0,xrop3_250_0_1},{xrop3_250_1_0,xrop3_250_1_1}},
  {{xrop3_251_0_0,xrop3_251_0_1},{xrop3_251_1_0,xrop3_251_1_1}},
  {{xrop3_252_0_0,xrop3_252_0_1},{xrop3_252_1_0,xrop3_252_1_1}},
  {{xrop3_253_0_0,xrop3_253_0_1},{xrop3_253_1_0,xrop3_253_1_1}},
  {{xrop3_254_0_0,xrop3_254_0_1},{xrop3_254_1_0,xrop3_254_1_1}},
  {{xrop3_255_0_0,xrop3_255_0_1},{xrop3_255_1_0,xrop3_255_1_1}}
};

hpgs_xrop3_func_t hpgs_xrop3_func(int rop3,
                                  hpgs_bool src_transparency,
                                  hpgs_bool pattern_transparency)
{
  if (rop3 < 0 || rop3 >= 256) return 0;
  return xrop3_table[rop3][src_transparency!=0][pattern_transparency!=0];
}

#include <stdio.h>
#include <ctype.h>
#include "ansidecl.h"
#include "gdb/callback.h"
#include "opcode/mn10200.h"
#include <limits.h>
#include "gdb/remote-sim.h"

#ifndef INLINE
#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif
#endif

extern host_callback *mn10200_callback;

#define DEBUG_TRACE		0x00000001
#define DEBUG_VALUES		0x00000002

extern int mn10200_debug;

#ifdef __STDC__
#define SIGNED signed
#else
#define SIGNED
#endif

#if UCHAR_MAX == 255
typedef unsigned char uint8;
typedef SIGNED char int8;
#else
error "Char is not an 8-bit type"
#endif

#if SHRT_MAX == 32767
typedef unsigned short uint16;
typedef SIGNED short int16;
#else
error "Short is not a 16-bit type"
#endif

#if INT_MAX == 2147483647

typedef unsigned int uint32;
typedef SIGNED int int32;

#else
#  if LONG_MAX == 2147483647

typedef unsigned long uint32;
typedef SIGNED long int32;

#  else
  error "Neither int nor long is a 32-bit type"
#  endif
#endif

typedef uint32 reg_t;

struct simops 
{
  long opcode;
  long mask;
  void (*func)();
  int length;
  int format;
  int numops;
  int operands[16];
};

/* The current state of the processor; registers, memory, etc.  */

struct _state
{
  reg_t regs[11];		/* registers, d0-d3, a0-a3, pc, mdr, psw */
  uint8 *mem;			/* main memory */
  int exception;		/* Actually a signal number.  */
  int exited;			/* Did the program exit? */
} State;

extern uint32 OP[4];
extern struct simops Simops[];

#define PC	(State.regs[8])

#define PSW (State.regs[10])
#define PSW_ZF 0x1
#define PSW_NF 0x2
#define PSW_CF 0x4
#define PSW_VF 0x8
#define PSW_ZX 0x10
#define PSW_NX 0x20
#define PSW_CX 0x40
#define PSW_VX 0x80

#define REG_D0 0
#define REG_A0 4
#define REG_SP 7
#define REG_PC 8
#define REG_MDR 9
#define REG_PSW 10

#define SEXT3(x)	((((x)&0x7)^(~0x3))+0x4)	

/* sign-extend a 4-bit number */
#define SEXT4(x)	((((x)&0xf)^(~0x7))+0x8)	

/* sign-extend a 5-bit number */
#define SEXT5(x)	((((x)&0x1f)^(~0xf))+0x10)	

/* sign-extend an 8-bit number */
#define SEXT8(x)	((((x)&0xff)^(~0x7f))+0x80)

/* sign-extend a 9-bit number */
#define SEXT9(x)	((((x)&0x1ff)^(~0xff))+0x100)

/* sign-extend a 16-bit number */
#define SEXT16(x)	((((x)&0xffff)^(~0x7fff))+0x8000)

/* sign-extend a 22-bit number */
#define SEXT22(x)	((((x)&0x3fffff)^(~0x1fffff))+0x200000)

/* sign-extend a 24-bit number */
#define SEXT24(x)	((((x)&0xffffff)^(~0x7fffff))+0x800000)

#ifdef _WIN32
#define SIGTRAP 5
#define SIGQUIT 3
#endif

extern int max_mem;

#define load_mem_big(addr,len) \
  (len == 1 ? *(((addr) & 0xffffff) + State.mem) : \
   len == 2 ? ((*(((addr) & 0xffffff) + State.mem) << 8) \
	       | *((((addr) + 1) & 0xffffff) + State.mem)) : \
   	      ((*(((addr) & 0xffffff) + State.mem) << 16) \
	       | (*((((addr) + 1) & 0xffffff) + State.mem) << 8) \
	       | *((((addr) + 2) & 0xffffff) + State.mem)))

static INLINE uint32
load_byte (addr)
     SIM_ADDR addr;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  return p[0];
}

static INLINE uint32
load_half (addr)
     SIM_ADDR addr;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  return p[1] << 8 | p[0];
}

static INLINE uint32
load_3_byte (addr)
     SIM_ADDR addr;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  return p[2] << 16 | p[1] << 8 | p[0];
}

static INLINE uint32
load_word (addr)
     SIM_ADDR addr;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  return p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
}

static INLINE uint32
load_mem (addr, len)
     SIM_ADDR addr;
     int len;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  switch (len)
    {
    case 1:
      return p[0];
    case 2:
      return p[1] << 8 | p[0];
    case 3:
      return p[2] << 16 | p[1] << 8 | p[0];
    case 4:
      return p[3] << 24 | p[2] << 16 | p[1] << 8 | p[0];
    default:
      abort ();
    }
}

static INLINE void
store_byte (addr, data)
     SIM_ADDR addr;
     uint32 data;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  p[0] = data;
}

static INLINE void
store_half (addr, data)
     SIM_ADDR addr;
     uint32 data;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  p[0] = data;
  p[1] = data >> 8;
}

static INLINE void
store_3_byte (addr, data)
     SIM_ADDR addr;
     uint32 data;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  p[0] = data;
  p[1] = data >> 8;
  p[2] = data >> 16;
}

static INLINE void
store_word (addr, data)
     SIM_ADDR addr;
     uint32 data;
{
  uint8 *p = (addr & 0xffffff) + State.mem;

#ifdef CHECK_ADDR
  if ((addr & 0xffffff) > max_mem)
    abort ();
#endif

  p[0] = data;
  p[1] = data >> 8;
  p[2] = data >> 16;
  p[3] = data >> 24;
}

/* Function declarations.  */

uint32 get_word PARAMS ((uint8 *));
void put_word PARAMS ((uint8 *, uint32));

extern uint8 *map PARAMS ((SIM_ADDR addr));

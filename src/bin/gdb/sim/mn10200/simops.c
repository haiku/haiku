#include "config.h"

#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "mn10200_sim.h"
#include "simops.h"
#include "targ-vals.h"
#include "bfd.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>

#define REG0(X) ((X) & 0x3)
#define REG1(X) (((X) & 0xc) >> 2)
#define REG0_4(X) (((X) & 0x30) >> 4)
#define REG0_8(X) (((X) & 0x300) >> 8)
#define REG1_8(X) (((X) & 0xc00) >> 10)
#define REG0_16(X) (((X) & 0x30000) >> 16)
#define REG1_16(X) (((X) & 0xc0000) >> 18)
#define TRUNC(X) ((X) & 0xffffff)

/* mov imm8, dn */
void OP_8000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)] = SEXT8 (insn & 0xff);
}

/* mov dn, dm */
void OP_80 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_D0 + REG1 (insn)];
}

/* mov dm, an */
void OP_F230 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)] = State.regs[REG_D0 + REG1 (insn)];
}

/* mov an, dm */
void OP_F2F0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_A0 + REG1 (insn)];
}

/* mov an, am */
void OP_F270 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)] = State.regs[REG_A0 + REG1 (insn)];
}

/* mov psw, dn */
void OP_F3F0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = PSW & 0xffff;
}

/* mov dn, psw */
void OP_F3D0 (insn, extension)
     unsigned long insn, extension;
{
  PSW = State.regs[REG_D0 + REG1 (insn)] & 0xffff ;
}

/* mov mdr, dn */
void OP_F3E0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_MDR] & 0xffff;
}

/* mov dn, mdr */
void OP_F3C0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_MDR] = State.regs[REG_D0 + REG1 (insn)] & 0xffff;
}

/* mov (an), dm */
void OP_20 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)]
    = SEXT16 (load_half (State.regs[REG_A0 + REG1 (insn)]));
}

/* mov (d8,an), dm */
void OP_6000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = SEXT16 (load_half ((State.regs[REG_A0 + REG1_8 (insn)]
			 + SEXT8 (insn & 0xff))));
}

/* mov (d16,an), dm */
void OP_F7C00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT16 (load_half ((State.regs[REG_A0 + REG1_16 (insn)]
	      + SEXT16 (insn & 0xffff))));
}

/* mov (d24,am), dn */
void OP_F4800000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT16 (load_half ((State.regs[REG_A0 + REG1_16 (insn)]
			 + SEXT24 (((insn & 0xffff) << 8) + extension))));
}

/* mov (di,an), dm */
void OP_F140 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)]
    = SEXT16 (load_half ((State.regs[REG_A0 + REG1 (insn)]
			 + State.regs[REG_D0 + REG0_4 (insn)])));
}

/* mov (abs16), dn */
void OP_C80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)] = SEXT16 (load_half ((insn & 0xffff)));
}

/* mov (abs24), dn */
void OP_F4C00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT16 (load_half ((((insn & 0xffff) << 8) + extension)));
}

/* mov (d8,an), am */
void OP_7000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_8 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1_8 (insn)]
			 + SEXT8 (insn & 0xff))));
}

/* mov (d16,an), am */
void OP_F7B00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1_16 (insn)]
			 + SEXT16 (insn & 0xffff))));
}

/* mov (d24,am), an */
void OP_F4F00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1_16 (insn)]
			   + SEXT24 (((insn & 0xffff) << 8) + extension))));
}

/* mov (di,an), am */
void OP_F100 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1 (insn)]
			   + State.regs[REG_D0 + REG0_4 (insn)])));
}

/* mov (abs16), an */
void OP_F7300000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)] = SEXT24 (load_3_byte ((insn & 0xffff)));
}

/* mov (abs24), an */
void OP_F4D00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = SEXT24 (load_3_byte ((((insn & 0xffff) << 8) + extension)));
}

/* mov dm, (an) */
void OP_0 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_A0 + REG1 (insn)],
	      State.regs[REG_D0 + REG0 (insn)]);
}

/* mov dm, (d8,an) */
void OP_4000 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_A0 + REG1_8 (insn)] + SEXT8 (insn & 0xff),
	      State.regs[REG_D0 + REG0_8 (insn)]);
}

/* mov dm, (d16,an) */
void OP_F7800000 (insn, extension)
     unsigned long insn, extension;
{
  store_half (State.regs[REG_A0 + REG1_16 (insn)] + SEXT16 (insn & 0xffff),
	      State.regs[REG_D0 + REG0_16 (insn)]);
}

/* mov dm, (d24,am) */
void OP_F4000000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((State.regs[REG_A0 + REG1_16 (insn)]
	       + SEXT24 (((insn & 0xffff) << 8) + extension)),
	      State.regs[REG_D0 + REG0_16 (insn)]);
}

/* mov dm, (di,an) */
void OP_F1C0 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((State.regs[REG_A0 + REG1 (insn)]
	       + State.regs[REG_D0 + REG0_4 (insn)]),
	      State.regs[REG_D0 + REG0 (insn)]);
}

/* mov dn, (abs16) */
void OP_C00000 (insn, extension)
     unsigned long insn, extension;
{
  store_half ((insn & 0xffff), State.regs[REG_D0 + REG0_16 (insn)]);
}

/* mov dn, (abs24) */
void OP_F4400000 (insn, extension)
     unsigned long insn, extension;
{
  store_half (SEXT24 (((insn & 0xffff) << 8) + extension),
	     State.regs[REG_D0 + REG0_16 (insn)]);
}

/* mov am, (d8,an) */
void OP_5000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte (State.regs[REG_A0 + REG1_8 (insn)] + SEXT8 (insn & 0xff),
		State.regs[REG_A0 + REG0_8 (insn)]);
}

/* mov am, (d16,an) */
void OP_F7A00000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte (State.regs[REG_A0 + REG1_16 (insn)] + SEXT16 (insn & 0xffff),
		State.regs[REG_A0 + REG0_16 (insn)]);
}

/* mov am, (d24,an) */
void OP_F4100000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte ((State.regs[REG_A0 + REG1_16 (insn)]
		 + SEXT24 (((insn & 0xffff) << 8) + extension)),
		State.regs[REG_A0 + REG0_16 (insn)]);
}

/* mov am, (di,an) */
void OP_F180 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte ((State.regs[REG_A0 + REG1 (insn)]
	         + State.regs[REG_D0 + REG0_4 (insn)]),
		State.regs[REG_A0 + REG0 (insn)]);
}

/* mov an, (abs16) */
void OP_F7200000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte ((insn & 0xffff), State.regs[REG_A0 + REG0_16 (insn)]);
}

/* mov an, (abs24) */
void OP_F4500000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte (SEXT24 (((insn & 0xffff) << 8) + extension),
		State.regs[REG_A0 + REG0_16 (insn)]);
}

/* mov imm16, dn */
void OP_F80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)] = SEXT16 (insn & 0xffff);
}

/* mov imm24, dn */
void OP_F4700000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT24 (((insn & 0xffff) << 8) + extension);
}

/* mov imm16, an */
void OP_DC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)] = insn & 0xffff;
}

/* mov imm24, an */
void OP_F4740000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_A0 + REG0_16 (insn)]
    = SEXT24 (((insn & 0xffff) << 8) + extension);
}

/* movx (d8,an), dm */
void OP_F57000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1_8 (insn)]
			   + SEXT8 (insn & 0xff))));
}

/* movx (d16,an), dm */
void OP_F7700000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1_16 (insn)]
			   + SEXT16 (insn & 0xffff))));
}

/* movx (d24,am), dn */
void OP_F4B00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT24 (load_3_byte ((State.regs[REG_A0 + REG1_16 (insn)]
			   + SEXT24 (((insn & 0xffff) << 8) + extension))));
}

/* movx dm, (d8,an) */
void OP_F55000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte (State.regs[REG_A0 + REG1_8 (insn)] + SEXT8 (insn & 0xff),
		State.regs[REG_D0 + REG0_8 (insn)]);
}

/* movx dm, (d16,an) */
void OP_F7600000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte (State.regs[REG_A0 + REG1_16 (insn)] + SEXT16 (insn & 0xffff),
		State.regs[REG_D0 + REG0_16 (insn)]);
}

/* movx dm, (d24,am) */
void OP_F4300000 (insn, extension)
     unsigned long insn, extension;
{
  store_3_byte ((State.regs[REG_A0 + REG1_16 (insn)]
		 + SEXT24 (((insn & 0xffff) << 8) + extension)),
		State.regs[REG_D0 + REG0_16 (insn)]);
}

/* movb (d8,an), dm */
void OP_F52000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = SEXT8 (load_byte ((State.regs[REG_A0 + REG1_8 (insn)]
			+ SEXT8 (insn & 0xff))));
}

/* movb (d16,an), dm */
void OP_F7D00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT8 (load_byte ((State.regs[REG_A0 + REG1_16 (insn)]
			+ SEXT16 (insn & 0xffff))));
}

/* movb (d24,am), dn */
void OP_F4A00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT8 (load_byte ((State.regs[REG_A0 + REG1_16 (insn)]
			+ SEXT24 (((insn & 0xffff) << 8) + extension))));
}

/* movb (di,an), dm */
void OP_F040 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)]
    = SEXT8 (load_byte ((State.regs[REG_A0 + REG1 (insn)]
			+ State.regs[REG_D0 + REG0_4 (insn)])));
}

/* mov (abs24), dn */
void OP_F4C40000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = SEXT8 (load_byte ((((insn & 0xffff) << 8) + extension)));
}

/* movb dm, (an) */
void OP_10 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_A0 + REG1 (insn)],
	      State.regs[REG_D0 + REG0 (insn)]);
}

/* movb dm, (d8,an) */
void OP_F51000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_A0 + REG1_8 (insn)] + SEXT8 (insn & 0xff),
	      State.regs[REG_D0 + REG0_8 (insn)]);
}

/* movb dm, (d16,an) */
void OP_F7900000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (State.regs[REG_A0 + REG1_16 (insn)] + SEXT16 (insn & 0xffff),
	      State.regs[REG_D0 + REG0_16 (insn)]);
}

/* movb dm, (d24,am) */
void OP_F4200000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((State.regs[REG_A0 + REG1_16 (insn)]
	      + SEXT24 (((insn & 0xffff) << 8) + extension)),
	      State.regs[REG_D0 + REG0_16 (insn)]);
}

/* movb dm, (di,an) */
void OP_F0C0 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((State.regs[REG_A0 + REG1 (insn)]
	      + State.regs[REG_D0 + REG0_4 (insn)]),
	      State.regs[REG_D0 + REG0 (insn)]);
}

/* movb dn, (abs16) */
void OP_C40000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte ((insn & 0xffff), State.regs[REG_D0 + REG0_16 (insn)]);
}

/* movb dn, (abs24) */
void OP_F4440000 (insn, extension)
     unsigned long insn, extension;
{
  store_byte (SEXT24 (((insn & 0xffff) << 8) + extension),
	     State.regs[REG_D0 + REG0_16 (insn)]);
}

/* movbu (an), dm */
void OP_30 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)]
    = load_byte (State.regs[REG_A0 + REG1 (insn)]);
}

/* movbu (d8,an), dm */
void OP_F53000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_8 (insn)]
    = load_byte ((State.regs[REG_A0 + REG1_8 (insn)] + SEXT8 (insn & 0xff)));
}

/* movbu (d16,an), dm */
void OP_F7500000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_byte ((State.regs[REG_A0 + REG1_16 (insn)]
		  + SEXT16 (insn & 0xffff)));
}

/* movbu (d24,am), dn */
void OP_F4900000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_byte ((State.regs[REG_A0 + REG1_16 (insn)]
		  + SEXT24 (((insn & 0xffff) << 8) + extension)));
}

/* movbu (di,an), dm */
void OP_F080 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)]
    = load_byte ((State.regs[REG_A0 + REG1 (insn)]
		  + State.regs[REG_D0 + REG0_4 (insn)]));
}

/* movbu (abs16), dn */
void OP_CC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)] = load_byte ((insn & 0xffff));
}

/* movbu (abs24), dn */
void OP_F4C80000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0_16 (insn)]
    = load_byte ((((insn & 0xffff) << 8) + extension));
}

/* ext dn */
void OP_F3C1 (insn, extension)
     unsigned long insn, extension;
{
  if (State.regs[REG_D0 + REG1 (insn)] & 0x8000)
    State.regs[REG_MDR] = 0xffff;
  else
    State.regs[REG_MDR] = 0;
}

/* extx dn */
void OP_B0 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = SEXT16 (State.regs[REG_D0 + REG0 (insn)]);
}

/* extxu dn */
void OP_B4 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_D0 + REG0 (insn)] & 0xffff;
}

/* extxb dn */
void OP_B8 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = SEXT8 (State.regs[REG_D0 + REG0 (insn)]);
}

/* extxbu dn */
void OP_BC (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_D0 + REG0 (insn)] = State.regs[REG_D0 + REG0 (insn)] & 0xff;
}

/* add dn,dm */
void OP_90 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg1 + reg2);
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (reg2 & 0xffff));
  cx = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x8000) == (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) == (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add dm, an */
void OP_F200 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_A0 + REG0 (insn)]);
  value = TRUNC (reg1 + reg2);
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (reg2 & 0xffff));
  cx = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x8000) == (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) == (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add an, dm */
void OP_F2C0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg1 + reg2);
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (reg2 & 0xffff));
  cx = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x8000) == (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) == (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add an,am */
void OP_F240 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_A0 + REG0 (insn)]);
  value = TRUNC (reg1 + reg2);
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (reg2 & 0xffff));
  cx = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x8000) == (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) == (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add imm8, dn */
void OP_D400 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_8 (insn)]);
  imm = TRUNC (SEXT8 (insn & 0xff));
  value = TRUNC (reg1 + imm);
  State.regs[REG_D0 + REG0_8 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (imm & 0xffff));
  cx = (value < reg1) || (value < imm);
  v = ((reg1 & 0x8000) == (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) == (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add imm16, dn */
void OP_F7180000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_16 (insn)]);
  imm = TRUNC (SEXT16 (insn & 0xffff));
  value = TRUNC (reg1 + imm);
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (imm & 0xffff));
  cx = (value < reg1) || (value < imm);
  v = ((reg1 & 0x8000) == (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) == (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add imm24,dn */
void OP_F4600000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_16 (insn)]);
  imm = TRUNC (((insn & 0xffff) << 8) + extension);
  value = TRUNC (reg1 + imm);
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (imm & 0xffff));
  cx = (value < reg1) || (value < imm);
  v = ((reg1 & 0x8000) == (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) == (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add imm8, an */
void OP_D000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_8 (insn)]);
  imm = TRUNC (SEXT8 (insn & 0xff));
  value = TRUNC (reg1 + imm);
  State.regs[REG_A0 + REG0_8 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (imm & 0xffff));
  cx = (value < reg1) || (value < imm);
  v = ((reg1 & 0x8000) == (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) == (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add imm16, an */
void OP_F7080000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_16 (insn)]);
  imm = TRUNC (SEXT16 (insn & 0xffff));
  value = TRUNC (reg1 + imm);
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (imm & 0xffff));
  cx = (value < reg1) || (value < imm);
  v = ((reg1 & 0x8000) == (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) == (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* add imm24, an */
void OP_F4640000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_16 (insn)]);
  imm = TRUNC (((insn & 0xffff) << 8) + extension);
  value = TRUNC (reg1 + imm);
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff)) || ((value & 0xffff) < (imm & 0xffff));
  cx = (value < reg1) || (value < imm);
  v = ((reg1 & 0x8000) == (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) == (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* addc dm,dn */
void OP_F280 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg1 + reg2 + ((PSW & PSW_CF) != 0));
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((value & 0xffff) < (reg1 & 0xffff))
       || ((value & 0xffff) < (reg2 & 0xffff));
  cx = (value < reg1) || (value < reg2);
  v = ((reg2 & 0x8000) == (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) == (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* addnf imm8, an */
void OP_F50C00 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long reg1, imm, value;

  reg1 = State.regs[REG_A0 + REG0_8 (insn)];
  imm = SEXT8 (insn & 0xff);
  value = reg1 + imm;
  State.regs[REG_A0 + REG0_8 (insn)] = TRUNC (value);
}

/* sub dn, dm */
void OP_A0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub dm, an */
void OP_F210 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_A0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub an, dm */
void OP_F2D0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub an, am */
void OP_F250 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_A0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);
  State.regs[REG_A0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub imm16, dn */
void OP_F71C0000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_16 (insn)]);
  imm = TRUNC (SEXT16 (insn & 0xffff));
  value = TRUNC (reg1 - imm);
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub imm24, dn */
void OP_F4680000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_16 (insn)]);
  imm = TRUNC (((insn & 0xffff) << 8) + extension);
  value = TRUNC (reg1 - imm);
  State.regs[REG_D0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub imm16, an */
void OP_F70C0000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_16 (insn)]);
  imm = TRUNC (SEXT16 (insn & 0xffff));
  value = TRUNC (reg1 - imm);
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* sub imm24, an */
void OP_F46C0000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_16 (insn)]);
  imm = TRUNC (((insn & 0xffff) << 8) + extension);
  value = TRUNC (reg1 - imm);
  State.regs[REG_A0 + REG0_16 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* subc dm, dn */
void OP_F290 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1 - ((PSW & PSW_CF) != 0));
  State.regs[REG_D0 + REG0 (insn)] = value;

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* mul dn, dm */
void OP_F340 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int n, z;

  temp = (SEXT16 (State.regs[REG_D0 + REG0 (insn)] & 0xffff)
          *  SEXT16 ((State.regs[REG_D0 + REG1 (insn)] & 0xffff)));
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffff;
  State.regs[REG_MDR] = temp >> 16;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* mulu dn, dm */
void OP_F350 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int n, z;

  temp = ((State.regs[REG_D0 + REG0 (insn)] & 0xffff)
          *  (State.regs[REG_D0 + REG1 (insn)] & 0xffff));
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffffff;
  State.regs[REG_MDR] = temp >> 16;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}


/* divu dm, dn */
void OP_F360 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int n, z;

  temp = State.regs[REG_MDR];
  temp &= 0xffff;
  temp <<= 16;
  temp |= (State.regs[REG_D0 + REG0 (insn)] & 0xffff);
  State.regs[REG_MDR] = (temp
			  % (unsigned long)(State.regs[REG_D0 + REG1 (insn)] & 0xffff));
  temp /= (unsigned long)(State.regs[REG_D0 + REG1 (insn)] & 0xffff);
  State.regs[REG_D0 + REG0 (insn)] = temp & 0xffff;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* cmp imm8, dn */
void OP_D800 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_8 (insn)]);
  imm = TRUNC (SEXT8 (insn & 0xff));
  value = TRUNC (reg1 - imm);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));
}

/* cmp dn, dm */
void OP_F390 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp dm, an */
void OP_F220 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_A0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp an, dm */
void OP_F2E0 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_D0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp an, am */
void OP_F260 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, reg2, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG1 (insn)]);
  reg2 = TRUNC (State.regs[REG_A0 + REG0 (insn)]);
  value = TRUNC (reg2 - reg1);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) > (reg2 & 0xffff));
  cx = (reg1 > reg2);
  v = ((reg2 & 0x8000) != (reg1 & 0x8000)
       && (reg2 & 0x8000) != (value & 0x8000));
  vx = ((reg2 & 0x800000) != (reg1 & 0x800000)
        && (reg2 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp imm16, dn */
void OP_F7480000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_16 (insn)]);
  imm = TRUNC (SEXT16 (insn & 0xffff));
  value = TRUNC (reg1 - imm);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp imm24, dn */
void OP_F4780000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_D0 + REG0_16 (insn)]);
  imm = TRUNC (((insn & 0xffff) << 8) + extension);
  value = TRUNC (reg1 - imm);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp imm16, an */
void OP_EC0000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_16 (insn)]);
  imm = TRUNC (insn & 0xffff);
  value = TRUNC (reg1 - imm);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* cmp imm24, an */
void OP_F47C0000 (insn, extension)
     unsigned long insn, extension;
{
  int z, c, n, v, zx, cx, nx, vx;
  unsigned long reg1, imm, value;

  reg1 = TRUNC (State.regs[REG_A0 + REG0_16 (insn)]);
  imm = TRUNC (((insn & 0xffff) << 8) + extension);
  value = TRUNC (reg1 - imm);

  z = ((value & 0xffff) == 0);
  zx = (value == 0);
  n = (value & 0x8000);
  nx = (value & 0x800000);
  c = ((reg1 & 0xffff) < (imm & 0xffff));
  cx = (reg1 < imm);
  v = ((reg1 & 0x8000) != (imm & 0x8000)
       && (reg1 & 0x8000) != (value & 0x8000));
  vx = ((reg1 & 0x800000) != (imm & 0x800000)
        && (reg1 & 0x800000) != (value & 0x800000));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF
	   | PSW_ZX | PSW_NX | PSW_CX | PSW_VX);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0)
	  | (zx ? PSW_ZX : 0) | (nx ? PSW_NX : 0)
	  | (cx ? PSW_CX : 0) | (vx ? PSW_VX : 0));

  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | ( n ? PSW_NF : 0)
	  | (c ? PSW_CF : 0) | (v ? PSW_VF : 0));
}

/* and dn, dm */
void OP_F300 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0 (insn)] & State.regs[REG_D0 + REG1 (insn)];
  temp &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* and imm8, dn */
void OP_F50000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0_8 (insn)] & (insn & 0xff);
  temp &= 0xffff;
  State.regs[REG_D0 + REG0_8 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0_8 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0_8 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* and imm16, dn */
void OP_F7000000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0_16 (insn)] & (insn & 0xffff);
  temp &= 0xffff;
  State.regs[REG_D0 + REG0_16 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0_16 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0_16 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* and imm16, psw */
void OP_F7100000 (insn, extension)
     unsigned long insn, extension;
{
  PSW &= (insn & 0xffff);
}

/* or dn, dm */
void OP_F310 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0 (insn)] | State.regs[REG_D0 + REG1 (insn)];
  temp &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* or imm8, dn */
void OP_F50800 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0_8 (insn)] | (insn & 0xff);
  temp &= 0xffff;
  State.regs[REG_D0 + REG0_8 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0_8 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0_8 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0_8 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* or imm16, dn */
void OP_F7400000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0_16 (insn)] | (insn & 0xffff);
  temp &= 0xffff;
  State.regs[REG_D0 + REG0_16 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0_16 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0_16 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* or imm16, psw */
void OP_F7140000 (insn, extension)
     unsigned long insn, extension;
{
  PSW |= (insn & 0xffff);
}

/* xor dn, dm */
void OP_F320 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0 (insn)] ^ State.regs[REG_D0 + REG1 (insn)];
  temp &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* xor imm16, dn */
void OP_F74C0000 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = State.regs[REG_D0 + REG0_16 (insn)] ^ (insn & 0xffff);
  temp &= 0xffff;
  State.regs[REG_D0 + REG0_16 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0_16 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0_16 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0_16 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* not dn */
void OP_F3E4 (insn, extension)
     unsigned long insn, extension;
{
  int n, z;
  unsigned long temp;

  temp = ~State.regs[REG_D0 + REG0 (insn)];
  temp &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0));
}

/* asr dn */
void OP_F338 (insn, extension)
     unsigned long insn, extension;
{
  long temp;
  int z, n, c, high;

  temp = State.regs[REG_D0 + REG0 (insn)] & 0xffff;
  c = temp & 1;
  high = temp & 0x8000;
  temp >>= 1;
  temp |= high;
  temp &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0) | (c ? PSW_CF : 0));
}

/* lsr dn */
void OP_F33C (insn, extension)
     unsigned long insn, extension;
{
  int z, n, c;
  long temp;

  temp = State.regs[REG_D0 + REG0 (insn)] & 0xffff;
  c = temp & 1;
  temp >>= 1;
  temp &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= temp;
  z = (State.regs[REG_D0 + REG0 (insn)] & 0xffff) == 0;
  n = (State.regs[REG_D0 + REG0 (insn)] & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0) | (c ? PSW_CF : 0));
}

/* ror dn */
void OP_F334 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;
  int c,n,z;

  value = State.regs[REG_D0 + REG0 (insn)] & 0xffff;
  c = (value & 0x1);

  value >>= 1;
  value |= (PSW & PSW_CF ? 0x8000 : 0);
  value &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= value;
  z = (value == 0);
  n = (value & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0) | (c ? PSW_CF : 0));
}

/* rol dn */
void OP_F330 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long value;
  int c,n,z;

  value = State.regs[REG_D0 + REG0 (insn)] & 0xffff;
  c = (value & 0x8000) ? 1 : 0;

  value <<= 1;
  value |= (PSW & PSW_CF ? 0x1 : 0);
  value &= 0xffff;
  State.regs[REG_D0 + REG0 (insn)] &= ~0xffff;
  State.regs[REG_D0 + REG0 (insn)] |= value;
  z = (value == 0);
  n = (value & 0x8000) != 0;
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= ((z ? PSW_ZF : 0) | (n ? PSW_NF : 0) | (c ? PSW_CF : 0));
}

/* btst imm8, dn */
void OP_F50400 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = State.regs[REG_D0 + REG0_8 (insn)];
  temp &= (insn & 0xff);
  z = (temp == 0);
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= (z ? PSW_ZF : 0);
}

/* btst imm16, dn */
void OP_F7040000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z, n, c;

  temp = State.regs[REG_D0 + REG0_16 (insn)];
  c = temp & 0x1;
  temp &= (insn & 0xffff);
  n = (temp & 0x8000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= (z ? PSW_ZF : 0) | (n ? PSW_NF : 0) | (c ? PSW_CF : 0);
}

/* bset dm, (an) */
void OP_F020 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte (State.regs[REG_A0 + REG1 (insn)]);
  z = (temp & State.regs[REG_D0 + REG0 (insn)]) == 0;
  temp |= State.regs[REG_D0 + REG0 (insn)];
  store_byte (State.regs[REG_A0 + REG1 (insn)], temp);
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= (z ? PSW_ZF : 0);
}

/* bclr dm, (an) */
void OP_F030 (insn, extension)
     unsigned long insn, extension;
{
  unsigned long temp;
  int z;

  temp = load_byte (State.regs[REG_A0 + REG1 (insn)]);
  z = (temp & State.regs[REG_D0 + REG0 (insn)]) == 0;
  temp = temp & ~State.regs[REG_D0 + REG0 (insn)];
  store_byte (State.regs[REG_A0 + REG1 (insn)], temp);
  PSW &= ~(PSW_ZF | PSW_NF | PSW_CF | PSW_VF);
  PSW |= (z ? PSW_ZF : 0);
}

/* beqx label:8 */
void OP_F5E800 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_ZX)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bnex label:8 */
void OP_F5E900 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_ZX))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bgtx label:8 */
void OP_F5E100 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!((PSW & PSW_ZX)
        || (((PSW & PSW_NX) != 0) ^ ((PSW & PSW_VX) != 0))))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bgex label:8 */
void OP_F5E200 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(((PSW & PSW_NX) != 0) ^ ((PSW & PSW_VX) != 0)))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* blex label:8 */
void OP_F5E300 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if ((PSW & PSW_ZX)
       || (((PSW & PSW_NX) != 0) ^ ((PSW & PSW_VX) != 0)))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bltx label:8 */
void OP_F5E000 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (((PSW & PSW_NX) != 0) ^ ((PSW & PSW_VX) != 0))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bhix label:8 */
void OP_F5E500 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(((PSW & PSW_CX) != 0) || (PSW & PSW_ZX) != 0))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bccx label:8 */
void OP_F5E600 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_CX))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* blsx label:8 */
void OP_F5E700 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (((PSW & PSW_CX) != 0) || (PSW & PSW_ZX) != 0)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bcsx label:8 */
void OP_F5E400 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_CX)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bvcx label:8 */
void OP_F5EC00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_VX))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bvsx label:8 */
void OP_F5ED00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_VX)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bncx label:8 */
void OP_F5EE00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_NX))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bnsx label:8 */
void OP_F5EF00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_NX)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* beq label:8 */
void OP_E800 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_ZF)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bne label:8 */
void OP_E900 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_ZF))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bgt label:8 */
void OP_E100 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!((PSW & PSW_ZF)
        || (((PSW & PSW_NF) != 0) ^ ((PSW & PSW_VF) != 0))))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bge label:8 */
void OP_E200 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(((PSW & PSW_NF) != 0) ^ ((PSW & PSW_VF) != 0)))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* ble label:8 */
void OP_E300 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if ((PSW & PSW_ZF)
       || (((PSW & PSW_NF) != 0) ^ ((PSW & PSW_VF) != 0)))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* blt label:8 */
void OP_E000 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (((PSW & PSW_NF) != 0) ^ ((PSW & PSW_VF) != 0))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bhi label:8 */
void OP_E500 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(((PSW & PSW_CF) != 0) || (PSW & PSW_ZF) != 0))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bcc label:8 */
void OP_E600 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_CF))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bls label:8 */
void OP_E700 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (((PSW & PSW_CF) != 0) || (PSW & PSW_ZF) != 0)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bcs label:8 */
void OP_E400 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_CF)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bvc label:8 */
void OP_F5FC00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_VF))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bvs label:8 */
void OP_F5FD00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_VF)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bnc label:8 */
void OP_F5FE00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (!(PSW & PSW_NF))
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bns label:8 */
void OP_F5FF00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 3 after we return, so
     we subtract two here to make things right.  */
  if (PSW & PSW_NF)
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* bra label:8 */
void OP_EA00 (insn, extension)
     unsigned long insn, extension;
{
  /* The dispatching code will add 2 after we return, so
     we subtract two here to make things right.  */
    State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT8 (insn & 0xff));
}

/* jmp (an) */
void OP_F000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC] = TRUNC (State.regs[REG_A0 + REG1 (insn)] - 2);
}

/* jmp label:16 */
void OP_FC0000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT16 (insn & 0xffff));
}

/* jmp label:24 */
void OP_F4E00000 (insn, extension)
     unsigned long insn, extension;
{
  State.regs[REG_PC]
    = TRUNC (State.regs[REG_PC] + (((insn & 0xffff) << 8) + extension));
}

/* jsr (an) */
void OP_F001 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;

  sp = State.regs[REG_SP];
  sp -= 4;
  State.regs[REG_SP] = sp;
  next_pc = State.regs[REG_PC] + 2;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.regs[REG_PC] = TRUNC (State.regs[REG_A0 + REG1 (insn)] - 2);
}

/* jsr label:16 */
void OP_FD0000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;

  sp = State.regs[REG_SP];
  sp -= 4;
  State.regs[REG_SP] = sp;
  next_pc = State.regs[REG_PC] + 3;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.regs[REG_PC] = TRUNC (State.regs[REG_PC] + SEXT16 (insn & 0xffff));
}

/* jsr label:24 */
void OP_F4E10000 (insn, extension)
     unsigned long insn, extension;
{
  unsigned int next_pc, sp;

  sp = State.regs[REG_SP];
  sp -= 4;
  State.regs[REG_SP] = sp;
  next_pc = State.regs[REG_PC] + 5;
  State.mem[sp] = next_pc & 0xff;
  State.mem[sp+1] = (next_pc & 0xff00) >> 8;
  State.mem[sp+2] = (next_pc & 0xff0000) >> 16;
  State.regs[REG_PC]
    = TRUNC (State.regs[REG_PC] + (((insn & 0xffff) << 8) + extension));
}

/* rts */
void OP_FE (insn, extension)
     unsigned long insn, extension;
{
  unsigned int sp;

  sp = State.regs[REG_SP];
  State.regs[REG_PC] = (State.mem[sp] | (State.mem[sp+1] << 8)
	      | (State.mem[sp+2] << 16));
  State.regs[REG_PC] -= 1;
  State.regs[REG_SP] += 4;
}

/* rti */
void OP_EB (insn, extension)
     unsigned long insn, extension;
{
  PSW = load_half (State.regs[REG_A0 + 3]);
  State.regs[REG_PC] = load_3_byte (State.regs[REG_A0 + 3] + 2) - 1;
  State.regs[REG_A0 + 3] += 6;
}

/* syscall */
void OP_F010 (insn, extension)
     unsigned long insn, extension;
{
  /* We use this for simulated system calls; we may need to change
     it to a reserved instruction if we conflict with uses at
     Matsushita.  */
  int save_errno = errno;	
  int offset = 6;
  errno = 0;

/* Registers passed to syscall 0 */

/* Function number.  */
#define FUNC   (State.regs[0])

#define PARM1  (State.regs[1])

/* Parameters.  */
#define PARM(x, y) (load_mem (State.regs[REG_SP] + x, y))

/* Registers set by syscall 0 */

#define RETVAL State.regs[0]	/* return value */
#define RETERR State.regs[1]	/* return error code */

/* Turn a pointer in a register into a pointer into real memory. */

#define MEMPTR(x) (State.mem + (x & 0xffffff))

  switch (FUNC)
    {
#if !defined(__GO32__) && !defined(_WIN32)
#ifdef TARGET_SYS_fork
      case TARGET_SYS_fork:
      RETVAL = fork ();
      break;
#endif
#ifdef TARGET_SYS_execve
    case TARGET_SYS_execve:
      RETVAL = execve (MEMPTR (PARM1), (char **) MEMPTR (PARM (4, 4)),
		       (char **)MEMPTR (PARM (8, 4)));
      break;
#endif
#ifdef TARGET_SYS_execv
    case TARGET_SYS_execv:
      RETVAL = execve (MEMPTR (PARM1), (char **) MEMPTR (PARM (4, 4)), NULL);
      break;
#endif
#endif

    case TARGET_SYS_read:
      RETVAL = mn10200_callback->read (mn10200_callback, PARM1, 
				       MEMPTR (PARM (4, 4)), PARM (8, 4));
      break;
    case TARGET_SYS_write:
      RETVAL = (int)mn10200_callback->write (mn10200_callback, PARM1,
					     MEMPTR (PARM (4, 4)),
					     PARM (8, 4));
      break;
    case TARGET_SYS_lseek:
      RETVAL = mn10200_callback->lseek (mn10200_callback, PARM1,
					PARM (4, 4), PARM (8, 2));
      break;
    case TARGET_SYS_close:
      RETVAL = mn10200_callback->close (mn10200_callback, PARM1);
      break;
    case TARGET_SYS_open:
      RETVAL = mn10200_callback->open (mn10200_callback, MEMPTR (PARM1),
				       PARM (4, 2));
      break;
    case TARGET_SYS_exit:
      /* EXIT - caller can look in PARM1 to work out the 
	 reason */
      if (PARM1 == 0xdead)
	State.exception = SIGABRT;
      else
	State.exception = SIGQUIT;
      State.exited = 1;
      break;

    case TARGET_SYS_stat:	/* added at hmsi */
      /* stat system call */
      {
	struct stat host_stat;
	reg_t buf;

	RETVAL = stat (MEMPTR (PARM1), &host_stat);
	
	buf = PARM (4, 4);

	/* Just wild-assed guesses.  */
	store_half (buf, host_stat.st_dev);
	store_half (buf + 2, host_stat.st_ino);
	store_word (buf + 4, host_stat.st_mode);
	store_half (buf + 8, host_stat.st_nlink);
	store_half (buf + 10, host_stat.st_uid);
	store_half (buf + 12, host_stat.st_gid);
	store_half (buf + 14, host_stat.st_rdev);
	store_word (buf + 16, host_stat.st_size);
	store_word (buf + 20, host_stat.st_atime);
	store_word (buf + 28, host_stat.st_mtime);
	store_word (buf + 36, host_stat.st_ctime);
      }
      break;

#ifdef TARGET_SYS_chown
    case TARGET_SYS_chown:
      RETVAL = chown (MEMPTR (PARM1), PARM (4, 2), PARM (6, 2));
      break;
#endif
    case TARGET_SYS_chmod:
      RETVAL = chmod (MEMPTR (PARM1), PARM (4, 2));
      break;
#ifdef TARGET_SYS_time
    case TARGET_SYS_time:
      RETVAL = time ((time_t *)MEMPTR (PARM1));
      break;
#endif
#ifdef TARGET_SYS_times
    case TARGET_SYS_times:
      {
	struct tms tms;
	RETVAL = times (&tms);
	store_word (PARM1, tms.tms_utime);
	store_word (PARM1 + 4, tms.tms_stime);
	store_word (PARM1 + 8, tms.tms_cutime);
	store_word (PARM1 + 12, tms.tms_cstime);
	break;
      }
#endif
#ifdef TARGET_SYS_gettimeofday
    case TARGET_SYS_gettimeofday:
      {
	struct timeval t;
	struct timezone tz;
	RETVAL = gettimeofday (&t, &tz);
	store_word (PARM1, t.tv_sec);
	store_word (PARM1 + 4, t.tv_usec);
	store_word (PARM (4, 4), tz.tz_minuteswest);
	store_word (PARM (4, 4) + 4, tz.tz_dsttime);
	break;
      }
#endif
#ifdef TARGET_SYS_utime
    case TARGET_SYS_utime:
      /* Cast the second argument to void *, to avoid type mismatch
	 if a prototype is present.  */
      RETVAL = utime (MEMPTR (PARM1), (void *) MEMPTR (PARM (4, 4)));
      break;
#endif
    default:
      abort ();
    }
  RETERR = errno;
  errno = save_errno;
}

/* nop */
void OP_F6 (insn, extension)
     unsigned long insn, extension;
{
}

/* breakpoint */
void
OP_FF (insn, extension)
     unsigned long insn, extension;
{
  State.exception = SIGTRAP;
  PC -= 1;
}

/* Simulator instruction semantics for i960base.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifdef DEFINE_LABELS

  /* The labels have the case they have because the enum of insn types
     is all uppercase and in the non-stdc case the insn symbol is built
     into the enum name.  */

  static struct {
    int index;
    void *label;
  } labels[] = {
    { I960BASE_INSN_X_INVALID, && case_sem_INSN_X_INVALID },
    { I960BASE_INSN_X_AFTER, && case_sem_INSN_X_AFTER },
    { I960BASE_INSN_X_BEFORE, && case_sem_INSN_X_BEFORE },
    { I960BASE_INSN_X_CTI_CHAIN, && case_sem_INSN_X_CTI_CHAIN },
    { I960BASE_INSN_X_CHAIN, && case_sem_INSN_X_CHAIN },
    { I960BASE_INSN_X_BEGIN, && case_sem_INSN_X_BEGIN },
    { I960BASE_INSN_MULO, && case_sem_INSN_MULO },
    { I960BASE_INSN_MULO1, && case_sem_INSN_MULO1 },
    { I960BASE_INSN_MULO2, && case_sem_INSN_MULO2 },
    { I960BASE_INSN_MULO3, && case_sem_INSN_MULO3 },
    { I960BASE_INSN_REMO, && case_sem_INSN_REMO },
    { I960BASE_INSN_REMO1, && case_sem_INSN_REMO1 },
    { I960BASE_INSN_REMO2, && case_sem_INSN_REMO2 },
    { I960BASE_INSN_REMO3, && case_sem_INSN_REMO3 },
    { I960BASE_INSN_DIVO, && case_sem_INSN_DIVO },
    { I960BASE_INSN_DIVO1, && case_sem_INSN_DIVO1 },
    { I960BASE_INSN_DIVO2, && case_sem_INSN_DIVO2 },
    { I960BASE_INSN_DIVO3, && case_sem_INSN_DIVO3 },
    { I960BASE_INSN_REMI, && case_sem_INSN_REMI },
    { I960BASE_INSN_REMI1, && case_sem_INSN_REMI1 },
    { I960BASE_INSN_REMI2, && case_sem_INSN_REMI2 },
    { I960BASE_INSN_REMI3, && case_sem_INSN_REMI3 },
    { I960BASE_INSN_DIVI, && case_sem_INSN_DIVI },
    { I960BASE_INSN_DIVI1, && case_sem_INSN_DIVI1 },
    { I960BASE_INSN_DIVI2, && case_sem_INSN_DIVI2 },
    { I960BASE_INSN_DIVI3, && case_sem_INSN_DIVI3 },
    { I960BASE_INSN_ADDO, && case_sem_INSN_ADDO },
    { I960BASE_INSN_ADDO1, && case_sem_INSN_ADDO1 },
    { I960BASE_INSN_ADDO2, && case_sem_INSN_ADDO2 },
    { I960BASE_INSN_ADDO3, && case_sem_INSN_ADDO3 },
    { I960BASE_INSN_SUBO, && case_sem_INSN_SUBO },
    { I960BASE_INSN_SUBO1, && case_sem_INSN_SUBO1 },
    { I960BASE_INSN_SUBO2, && case_sem_INSN_SUBO2 },
    { I960BASE_INSN_SUBO3, && case_sem_INSN_SUBO3 },
    { I960BASE_INSN_NOTBIT, && case_sem_INSN_NOTBIT },
    { I960BASE_INSN_NOTBIT1, && case_sem_INSN_NOTBIT1 },
    { I960BASE_INSN_NOTBIT2, && case_sem_INSN_NOTBIT2 },
    { I960BASE_INSN_NOTBIT3, && case_sem_INSN_NOTBIT3 },
    { I960BASE_INSN_AND, && case_sem_INSN_AND },
    { I960BASE_INSN_AND1, && case_sem_INSN_AND1 },
    { I960BASE_INSN_AND2, && case_sem_INSN_AND2 },
    { I960BASE_INSN_AND3, && case_sem_INSN_AND3 },
    { I960BASE_INSN_ANDNOT, && case_sem_INSN_ANDNOT },
    { I960BASE_INSN_ANDNOT1, && case_sem_INSN_ANDNOT1 },
    { I960BASE_INSN_ANDNOT2, && case_sem_INSN_ANDNOT2 },
    { I960BASE_INSN_ANDNOT3, && case_sem_INSN_ANDNOT3 },
    { I960BASE_INSN_SETBIT, && case_sem_INSN_SETBIT },
    { I960BASE_INSN_SETBIT1, && case_sem_INSN_SETBIT1 },
    { I960BASE_INSN_SETBIT2, && case_sem_INSN_SETBIT2 },
    { I960BASE_INSN_SETBIT3, && case_sem_INSN_SETBIT3 },
    { I960BASE_INSN_NOTAND, && case_sem_INSN_NOTAND },
    { I960BASE_INSN_NOTAND1, && case_sem_INSN_NOTAND1 },
    { I960BASE_INSN_NOTAND2, && case_sem_INSN_NOTAND2 },
    { I960BASE_INSN_NOTAND3, && case_sem_INSN_NOTAND3 },
    { I960BASE_INSN_XOR, && case_sem_INSN_XOR },
    { I960BASE_INSN_XOR1, && case_sem_INSN_XOR1 },
    { I960BASE_INSN_XOR2, && case_sem_INSN_XOR2 },
    { I960BASE_INSN_XOR3, && case_sem_INSN_XOR3 },
    { I960BASE_INSN_OR, && case_sem_INSN_OR },
    { I960BASE_INSN_OR1, && case_sem_INSN_OR1 },
    { I960BASE_INSN_OR2, && case_sem_INSN_OR2 },
    { I960BASE_INSN_OR3, && case_sem_INSN_OR3 },
    { I960BASE_INSN_NOR, && case_sem_INSN_NOR },
    { I960BASE_INSN_NOR1, && case_sem_INSN_NOR1 },
    { I960BASE_INSN_NOR2, && case_sem_INSN_NOR2 },
    { I960BASE_INSN_NOR3, && case_sem_INSN_NOR3 },
    { I960BASE_INSN_XNOR, && case_sem_INSN_XNOR },
    { I960BASE_INSN_XNOR1, && case_sem_INSN_XNOR1 },
    { I960BASE_INSN_XNOR2, && case_sem_INSN_XNOR2 },
    { I960BASE_INSN_XNOR3, && case_sem_INSN_XNOR3 },
    { I960BASE_INSN_NOT, && case_sem_INSN_NOT },
    { I960BASE_INSN_NOT1, && case_sem_INSN_NOT1 },
    { I960BASE_INSN_NOT2, && case_sem_INSN_NOT2 },
    { I960BASE_INSN_NOT3, && case_sem_INSN_NOT3 },
    { I960BASE_INSN_ORNOT, && case_sem_INSN_ORNOT },
    { I960BASE_INSN_ORNOT1, && case_sem_INSN_ORNOT1 },
    { I960BASE_INSN_ORNOT2, && case_sem_INSN_ORNOT2 },
    { I960BASE_INSN_ORNOT3, && case_sem_INSN_ORNOT3 },
    { I960BASE_INSN_CLRBIT, && case_sem_INSN_CLRBIT },
    { I960BASE_INSN_CLRBIT1, && case_sem_INSN_CLRBIT1 },
    { I960BASE_INSN_CLRBIT2, && case_sem_INSN_CLRBIT2 },
    { I960BASE_INSN_CLRBIT3, && case_sem_INSN_CLRBIT3 },
    { I960BASE_INSN_SHLO, && case_sem_INSN_SHLO },
    { I960BASE_INSN_SHLO1, && case_sem_INSN_SHLO1 },
    { I960BASE_INSN_SHLO2, && case_sem_INSN_SHLO2 },
    { I960BASE_INSN_SHLO3, && case_sem_INSN_SHLO3 },
    { I960BASE_INSN_SHRO, && case_sem_INSN_SHRO },
    { I960BASE_INSN_SHRO1, && case_sem_INSN_SHRO1 },
    { I960BASE_INSN_SHRO2, && case_sem_INSN_SHRO2 },
    { I960BASE_INSN_SHRO3, && case_sem_INSN_SHRO3 },
    { I960BASE_INSN_SHLI, && case_sem_INSN_SHLI },
    { I960BASE_INSN_SHLI1, && case_sem_INSN_SHLI1 },
    { I960BASE_INSN_SHLI2, && case_sem_INSN_SHLI2 },
    { I960BASE_INSN_SHLI3, && case_sem_INSN_SHLI3 },
    { I960BASE_INSN_SHRI, && case_sem_INSN_SHRI },
    { I960BASE_INSN_SHRI1, && case_sem_INSN_SHRI1 },
    { I960BASE_INSN_SHRI2, && case_sem_INSN_SHRI2 },
    { I960BASE_INSN_SHRI3, && case_sem_INSN_SHRI3 },
    { I960BASE_INSN_EMUL, && case_sem_INSN_EMUL },
    { I960BASE_INSN_EMUL1, && case_sem_INSN_EMUL1 },
    { I960BASE_INSN_EMUL2, && case_sem_INSN_EMUL2 },
    { I960BASE_INSN_EMUL3, && case_sem_INSN_EMUL3 },
    { I960BASE_INSN_MOV, && case_sem_INSN_MOV },
    { I960BASE_INSN_MOV1, && case_sem_INSN_MOV1 },
    { I960BASE_INSN_MOVL, && case_sem_INSN_MOVL },
    { I960BASE_INSN_MOVL1, && case_sem_INSN_MOVL1 },
    { I960BASE_INSN_MOVT, && case_sem_INSN_MOVT },
    { I960BASE_INSN_MOVT1, && case_sem_INSN_MOVT1 },
    { I960BASE_INSN_MOVQ, && case_sem_INSN_MOVQ },
    { I960BASE_INSN_MOVQ1, && case_sem_INSN_MOVQ1 },
    { I960BASE_INSN_MODPC, && case_sem_INSN_MODPC },
    { I960BASE_INSN_MODAC, && case_sem_INSN_MODAC },
    { I960BASE_INSN_LDA_OFFSET, && case_sem_INSN_LDA_OFFSET },
    { I960BASE_INSN_LDA_INDIRECT_OFFSET, && case_sem_INSN_LDA_INDIRECT_OFFSET },
    { I960BASE_INSN_LDA_INDIRECT, && case_sem_INSN_LDA_INDIRECT },
    { I960BASE_INSN_LDA_INDIRECT_INDEX, && case_sem_INSN_LDA_INDIRECT_INDEX },
    { I960BASE_INSN_LDA_DISP, && case_sem_INSN_LDA_DISP },
    { I960BASE_INSN_LDA_INDIRECT_DISP, && case_sem_INSN_LDA_INDIRECT_DISP },
    { I960BASE_INSN_LDA_INDEX_DISP, && case_sem_INSN_LDA_INDEX_DISP },
    { I960BASE_INSN_LDA_INDIRECT_INDEX_DISP, && case_sem_INSN_LDA_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LD_OFFSET, && case_sem_INSN_LD_OFFSET },
    { I960BASE_INSN_LD_INDIRECT_OFFSET, && case_sem_INSN_LD_INDIRECT_OFFSET },
    { I960BASE_INSN_LD_INDIRECT, && case_sem_INSN_LD_INDIRECT },
    { I960BASE_INSN_LD_INDIRECT_INDEX, && case_sem_INSN_LD_INDIRECT_INDEX },
    { I960BASE_INSN_LD_DISP, && case_sem_INSN_LD_DISP },
    { I960BASE_INSN_LD_INDIRECT_DISP, && case_sem_INSN_LD_INDIRECT_DISP },
    { I960BASE_INSN_LD_INDEX_DISP, && case_sem_INSN_LD_INDEX_DISP },
    { I960BASE_INSN_LD_INDIRECT_INDEX_DISP, && case_sem_INSN_LD_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDOB_OFFSET, && case_sem_INSN_LDOB_OFFSET },
    { I960BASE_INSN_LDOB_INDIRECT_OFFSET, && case_sem_INSN_LDOB_INDIRECT_OFFSET },
    { I960BASE_INSN_LDOB_INDIRECT, && case_sem_INSN_LDOB_INDIRECT },
    { I960BASE_INSN_LDOB_INDIRECT_INDEX, && case_sem_INSN_LDOB_INDIRECT_INDEX },
    { I960BASE_INSN_LDOB_DISP, && case_sem_INSN_LDOB_DISP },
    { I960BASE_INSN_LDOB_INDIRECT_DISP, && case_sem_INSN_LDOB_INDIRECT_DISP },
    { I960BASE_INSN_LDOB_INDEX_DISP, && case_sem_INSN_LDOB_INDEX_DISP },
    { I960BASE_INSN_LDOB_INDIRECT_INDEX_DISP, && case_sem_INSN_LDOB_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDOS_OFFSET, && case_sem_INSN_LDOS_OFFSET },
    { I960BASE_INSN_LDOS_INDIRECT_OFFSET, && case_sem_INSN_LDOS_INDIRECT_OFFSET },
    { I960BASE_INSN_LDOS_INDIRECT, && case_sem_INSN_LDOS_INDIRECT },
    { I960BASE_INSN_LDOS_INDIRECT_INDEX, && case_sem_INSN_LDOS_INDIRECT_INDEX },
    { I960BASE_INSN_LDOS_DISP, && case_sem_INSN_LDOS_DISP },
    { I960BASE_INSN_LDOS_INDIRECT_DISP, && case_sem_INSN_LDOS_INDIRECT_DISP },
    { I960BASE_INSN_LDOS_INDEX_DISP, && case_sem_INSN_LDOS_INDEX_DISP },
    { I960BASE_INSN_LDOS_INDIRECT_INDEX_DISP, && case_sem_INSN_LDOS_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDIB_OFFSET, && case_sem_INSN_LDIB_OFFSET },
    { I960BASE_INSN_LDIB_INDIRECT_OFFSET, && case_sem_INSN_LDIB_INDIRECT_OFFSET },
    { I960BASE_INSN_LDIB_INDIRECT, && case_sem_INSN_LDIB_INDIRECT },
    { I960BASE_INSN_LDIB_INDIRECT_INDEX, && case_sem_INSN_LDIB_INDIRECT_INDEX },
    { I960BASE_INSN_LDIB_DISP, && case_sem_INSN_LDIB_DISP },
    { I960BASE_INSN_LDIB_INDIRECT_DISP, && case_sem_INSN_LDIB_INDIRECT_DISP },
    { I960BASE_INSN_LDIB_INDEX_DISP, && case_sem_INSN_LDIB_INDEX_DISP },
    { I960BASE_INSN_LDIB_INDIRECT_INDEX_DISP, && case_sem_INSN_LDIB_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDIS_OFFSET, && case_sem_INSN_LDIS_OFFSET },
    { I960BASE_INSN_LDIS_INDIRECT_OFFSET, && case_sem_INSN_LDIS_INDIRECT_OFFSET },
    { I960BASE_INSN_LDIS_INDIRECT, && case_sem_INSN_LDIS_INDIRECT },
    { I960BASE_INSN_LDIS_INDIRECT_INDEX, && case_sem_INSN_LDIS_INDIRECT_INDEX },
    { I960BASE_INSN_LDIS_DISP, && case_sem_INSN_LDIS_DISP },
    { I960BASE_INSN_LDIS_INDIRECT_DISP, && case_sem_INSN_LDIS_INDIRECT_DISP },
    { I960BASE_INSN_LDIS_INDEX_DISP, && case_sem_INSN_LDIS_INDEX_DISP },
    { I960BASE_INSN_LDIS_INDIRECT_INDEX_DISP, && case_sem_INSN_LDIS_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDL_OFFSET, && case_sem_INSN_LDL_OFFSET },
    { I960BASE_INSN_LDL_INDIRECT_OFFSET, && case_sem_INSN_LDL_INDIRECT_OFFSET },
    { I960BASE_INSN_LDL_INDIRECT, && case_sem_INSN_LDL_INDIRECT },
    { I960BASE_INSN_LDL_INDIRECT_INDEX, && case_sem_INSN_LDL_INDIRECT_INDEX },
    { I960BASE_INSN_LDL_DISP, && case_sem_INSN_LDL_DISP },
    { I960BASE_INSN_LDL_INDIRECT_DISP, && case_sem_INSN_LDL_INDIRECT_DISP },
    { I960BASE_INSN_LDL_INDEX_DISP, && case_sem_INSN_LDL_INDEX_DISP },
    { I960BASE_INSN_LDL_INDIRECT_INDEX_DISP, && case_sem_INSN_LDL_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDT_OFFSET, && case_sem_INSN_LDT_OFFSET },
    { I960BASE_INSN_LDT_INDIRECT_OFFSET, && case_sem_INSN_LDT_INDIRECT_OFFSET },
    { I960BASE_INSN_LDT_INDIRECT, && case_sem_INSN_LDT_INDIRECT },
    { I960BASE_INSN_LDT_INDIRECT_INDEX, && case_sem_INSN_LDT_INDIRECT_INDEX },
    { I960BASE_INSN_LDT_DISP, && case_sem_INSN_LDT_DISP },
    { I960BASE_INSN_LDT_INDIRECT_DISP, && case_sem_INSN_LDT_INDIRECT_DISP },
    { I960BASE_INSN_LDT_INDEX_DISP, && case_sem_INSN_LDT_INDEX_DISP },
    { I960BASE_INSN_LDT_INDIRECT_INDEX_DISP, && case_sem_INSN_LDT_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_LDQ_OFFSET, && case_sem_INSN_LDQ_OFFSET },
    { I960BASE_INSN_LDQ_INDIRECT_OFFSET, && case_sem_INSN_LDQ_INDIRECT_OFFSET },
    { I960BASE_INSN_LDQ_INDIRECT, && case_sem_INSN_LDQ_INDIRECT },
    { I960BASE_INSN_LDQ_INDIRECT_INDEX, && case_sem_INSN_LDQ_INDIRECT_INDEX },
    { I960BASE_INSN_LDQ_DISP, && case_sem_INSN_LDQ_DISP },
    { I960BASE_INSN_LDQ_INDIRECT_DISP, && case_sem_INSN_LDQ_INDIRECT_DISP },
    { I960BASE_INSN_LDQ_INDEX_DISP, && case_sem_INSN_LDQ_INDEX_DISP },
    { I960BASE_INSN_LDQ_INDIRECT_INDEX_DISP, && case_sem_INSN_LDQ_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_ST_OFFSET, && case_sem_INSN_ST_OFFSET },
    { I960BASE_INSN_ST_INDIRECT_OFFSET, && case_sem_INSN_ST_INDIRECT_OFFSET },
    { I960BASE_INSN_ST_INDIRECT, && case_sem_INSN_ST_INDIRECT },
    { I960BASE_INSN_ST_INDIRECT_INDEX, && case_sem_INSN_ST_INDIRECT_INDEX },
    { I960BASE_INSN_ST_DISP, && case_sem_INSN_ST_DISP },
    { I960BASE_INSN_ST_INDIRECT_DISP, && case_sem_INSN_ST_INDIRECT_DISP },
    { I960BASE_INSN_ST_INDEX_DISP, && case_sem_INSN_ST_INDEX_DISP },
    { I960BASE_INSN_ST_INDIRECT_INDEX_DISP, && case_sem_INSN_ST_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_STOB_OFFSET, && case_sem_INSN_STOB_OFFSET },
    { I960BASE_INSN_STOB_INDIRECT_OFFSET, && case_sem_INSN_STOB_INDIRECT_OFFSET },
    { I960BASE_INSN_STOB_INDIRECT, && case_sem_INSN_STOB_INDIRECT },
    { I960BASE_INSN_STOB_INDIRECT_INDEX, && case_sem_INSN_STOB_INDIRECT_INDEX },
    { I960BASE_INSN_STOB_DISP, && case_sem_INSN_STOB_DISP },
    { I960BASE_INSN_STOB_INDIRECT_DISP, && case_sem_INSN_STOB_INDIRECT_DISP },
    { I960BASE_INSN_STOB_INDEX_DISP, && case_sem_INSN_STOB_INDEX_DISP },
    { I960BASE_INSN_STOB_INDIRECT_INDEX_DISP, && case_sem_INSN_STOB_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_STOS_OFFSET, && case_sem_INSN_STOS_OFFSET },
    { I960BASE_INSN_STOS_INDIRECT_OFFSET, && case_sem_INSN_STOS_INDIRECT_OFFSET },
    { I960BASE_INSN_STOS_INDIRECT, && case_sem_INSN_STOS_INDIRECT },
    { I960BASE_INSN_STOS_INDIRECT_INDEX, && case_sem_INSN_STOS_INDIRECT_INDEX },
    { I960BASE_INSN_STOS_DISP, && case_sem_INSN_STOS_DISP },
    { I960BASE_INSN_STOS_INDIRECT_DISP, && case_sem_INSN_STOS_INDIRECT_DISP },
    { I960BASE_INSN_STOS_INDEX_DISP, && case_sem_INSN_STOS_INDEX_DISP },
    { I960BASE_INSN_STOS_INDIRECT_INDEX_DISP, && case_sem_INSN_STOS_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_STL_OFFSET, && case_sem_INSN_STL_OFFSET },
    { I960BASE_INSN_STL_INDIRECT_OFFSET, && case_sem_INSN_STL_INDIRECT_OFFSET },
    { I960BASE_INSN_STL_INDIRECT, && case_sem_INSN_STL_INDIRECT },
    { I960BASE_INSN_STL_INDIRECT_INDEX, && case_sem_INSN_STL_INDIRECT_INDEX },
    { I960BASE_INSN_STL_DISP, && case_sem_INSN_STL_DISP },
    { I960BASE_INSN_STL_INDIRECT_DISP, && case_sem_INSN_STL_INDIRECT_DISP },
    { I960BASE_INSN_STL_INDEX_DISP, && case_sem_INSN_STL_INDEX_DISP },
    { I960BASE_INSN_STL_INDIRECT_INDEX_DISP, && case_sem_INSN_STL_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_STT_OFFSET, && case_sem_INSN_STT_OFFSET },
    { I960BASE_INSN_STT_INDIRECT_OFFSET, && case_sem_INSN_STT_INDIRECT_OFFSET },
    { I960BASE_INSN_STT_INDIRECT, && case_sem_INSN_STT_INDIRECT },
    { I960BASE_INSN_STT_INDIRECT_INDEX, && case_sem_INSN_STT_INDIRECT_INDEX },
    { I960BASE_INSN_STT_DISP, && case_sem_INSN_STT_DISP },
    { I960BASE_INSN_STT_INDIRECT_DISP, && case_sem_INSN_STT_INDIRECT_DISP },
    { I960BASE_INSN_STT_INDEX_DISP, && case_sem_INSN_STT_INDEX_DISP },
    { I960BASE_INSN_STT_INDIRECT_INDEX_DISP, && case_sem_INSN_STT_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_STQ_OFFSET, && case_sem_INSN_STQ_OFFSET },
    { I960BASE_INSN_STQ_INDIRECT_OFFSET, && case_sem_INSN_STQ_INDIRECT_OFFSET },
    { I960BASE_INSN_STQ_INDIRECT, && case_sem_INSN_STQ_INDIRECT },
    { I960BASE_INSN_STQ_INDIRECT_INDEX, && case_sem_INSN_STQ_INDIRECT_INDEX },
    { I960BASE_INSN_STQ_DISP, && case_sem_INSN_STQ_DISP },
    { I960BASE_INSN_STQ_INDIRECT_DISP, && case_sem_INSN_STQ_INDIRECT_DISP },
    { I960BASE_INSN_STQ_INDEX_DISP, && case_sem_INSN_STQ_INDEX_DISP },
    { I960BASE_INSN_STQ_INDIRECT_INDEX_DISP, && case_sem_INSN_STQ_INDIRECT_INDEX_DISP },
    { I960BASE_INSN_CMPOBE_REG, && case_sem_INSN_CMPOBE_REG },
    { I960BASE_INSN_CMPOBE_LIT, && case_sem_INSN_CMPOBE_LIT },
    { I960BASE_INSN_CMPOBNE_REG, && case_sem_INSN_CMPOBNE_REG },
    { I960BASE_INSN_CMPOBNE_LIT, && case_sem_INSN_CMPOBNE_LIT },
    { I960BASE_INSN_CMPOBL_REG, && case_sem_INSN_CMPOBL_REG },
    { I960BASE_INSN_CMPOBL_LIT, && case_sem_INSN_CMPOBL_LIT },
    { I960BASE_INSN_CMPOBLE_REG, && case_sem_INSN_CMPOBLE_REG },
    { I960BASE_INSN_CMPOBLE_LIT, && case_sem_INSN_CMPOBLE_LIT },
    { I960BASE_INSN_CMPOBG_REG, && case_sem_INSN_CMPOBG_REG },
    { I960BASE_INSN_CMPOBG_LIT, && case_sem_INSN_CMPOBG_LIT },
    { I960BASE_INSN_CMPOBGE_REG, && case_sem_INSN_CMPOBGE_REG },
    { I960BASE_INSN_CMPOBGE_LIT, && case_sem_INSN_CMPOBGE_LIT },
    { I960BASE_INSN_CMPIBE_REG, && case_sem_INSN_CMPIBE_REG },
    { I960BASE_INSN_CMPIBE_LIT, && case_sem_INSN_CMPIBE_LIT },
    { I960BASE_INSN_CMPIBNE_REG, && case_sem_INSN_CMPIBNE_REG },
    { I960BASE_INSN_CMPIBNE_LIT, && case_sem_INSN_CMPIBNE_LIT },
    { I960BASE_INSN_CMPIBL_REG, && case_sem_INSN_CMPIBL_REG },
    { I960BASE_INSN_CMPIBL_LIT, && case_sem_INSN_CMPIBL_LIT },
    { I960BASE_INSN_CMPIBLE_REG, && case_sem_INSN_CMPIBLE_REG },
    { I960BASE_INSN_CMPIBLE_LIT, && case_sem_INSN_CMPIBLE_LIT },
    { I960BASE_INSN_CMPIBG_REG, && case_sem_INSN_CMPIBG_REG },
    { I960BASE_INSN_CMPIBG_LIT, && case_sem_INSN_CMPIBG_LIT },
    { I960BASE_INSN_CMPIBGE_REG, && case_sem_INSN_CMPIBGE_REG },
    { I960BASE_INSN_CMPIBGE_LIT, && case_sem_INSN_CMPIBGE_LIT },
    { I960BASE_INSN_BBC_REG, && case_sem_INSN_BBC_REG },
    { I960BASE_INSN_BBC_LIT, && case_sem_INSN_BBC_LIT },
    { I960BASE_INSN_BBS_REG, && case_sem_INSN_BBS_REG },
    { I960BASE_INSN_BBS_LIT, && case_sem_INSN_BBS_LIT },
    { I960BASE_INSN_CMPI, && case_sem_INSN_CMPI },
    { I960BASE_INSN_CMPI1, && case_sem_INSN_CMPI1 },
    { I960BASE_INSN_CMPI2, && case_sem_INSN_CMPI2 },
    { I960BASE_INSN_CMPI3, && case_sem_INSN_CMPI3 },
    { I960BASE_INSN_CMPO, && case_sem_INSN_CMPO },
    { I960BASE_INSN_CMPO1, && case_sem_INSN_CMPO1 },
    { I960BASE_INSN_CMPO2, && case_sem_INSN_CMPO2 },
    { I960BASE_INSN_CMPO3, && case_sem_INSN_CMPO3 },
    { I960BASE_INSN_TESTNO_REG, && case_sem_INSN_TESTNO_REG },
    { I960BASE_INSN_TESTG_REG, && case_sem_INSN_TESTG_REG },
    { I960BASE_INSN_TESTE_REG, && case_sem_INSN_TESTE_REG },
    { I960BASE_INSN_TESTGE_REG, && case_sem_INSN_TESTGE_REG },
    { I960BASE_INSN_TESTL_REG, && case_sem_INSN_TESTL_REG },
    { I960BASE_INSN_TESTNE_REG, && case_sem_INSN_TESTNE_REG },
    { I960BASE_INSN_TESTLE_REG, && case_sem_INSN_TESTLE_REG },
    { I960BASE_INSN_TESTO_REG, && case_sem_INSN_TESTO_REG },
    { I960BASE_INSN_BNO, && case_sem_INSN_BNO },
    { I960BASE_INSN_BG, && case_sem_INSN_BG },
    { I960BASE_INSN_BE, && case_sem_INSN_BE },
    { I960BASE_INSN_BGE, && case_sem_INSN_BGE },
    { I960BASE_INSN_BL, && case_sem_INSN_BL },
    { I960BASE_INSN_BNE, && case_sem_INSN_BNE },
    { I960BASE_INSN_BLE, && case_sem_INSN_BLE },
    { I960BASE_INSN_BO, && case_sem_INSN_BO },
    { I960BASE_INSN_B, && case_sem_INSN_B },
    { I960BASE_INSN_BX_INDIRECT_OFFSET, && case_sem_INSN_BX_INDIRECT_OFFSET },
    { I960BASE_INSN_BX_INDIRECT, && case_sem_INSN_BX_INDIRECT },
    { I960BASE_INSN_BX_INDIRECT_INDEX, && case_sem_INSN_BX_INDIRECT_INDEX },
    { I960BASE_INSN_BX_DISP, && case_sem_INSN_BX_DISP },
    { I960BASE_INSN_BX_INDIRECT_DISP, && case_sem_INSN_BX_INDIRECT_DISP },
    { I960BASE_INSN_CALLX_DISP, && case_sem_INSN_CALLX_DISP },
    { I960BASE_INSN_CALLX_INDIRECT, && case_sem_INSN_CALLX_INDIRECT },
    { I960BASE_INSN_CALLX_INDIRECT_OFFSET, && case_sem_INSN_CALLX_INDIRECT_OFFSET },
    { I960BASE_INSN_RET, && case_sem_INSN_RET },
    { I960BASE_INSN_CALLS, && case_sem_INSN_CALLS },
    { I960BASE_INSN_FMARK, && case_sem_INSN_FMARK },
    { I960BASE_INSN_FLUSHREG, && case_sem_INSN_FLUSHREG },
    { 0, 0 }
  };
  int i;

  for (i = 0; labels[i].label != 0; ++i)
    {
#if FAST_P
      CPU_IDESC (current_cpu) [labels[i].index].sem_fast_lab = labels[i].label;
#else
      CPU_IDESC (current_cpu) [labels[i].index].sem_full_lab = labels[i].label;
#endif
    }

#undef DEFINE_LABELS
#endif /* DEFINE_LABELS */

#ifdef DEFINE_SWITCH

/* If hyper-fast [well not unnecessarily slow] execution is selected, turn
   off frills like tracing and profiling.  */
/* FIXME: A better way would be to have TRACE_RESULT check for something
   that can cause it to be optimized out.  Another way would be to emit
   special handlers into the instruction "stream".  */

#if FAST_P
#undef TRACE_RESULT
#define TRACE_RESULT(cpu, abuf, name, type, val)
#endif

#undef GET_ATTR
#define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)

{

#if WITH_SCACHE_PBB

/* Branch to next handler without going around main loop.  */
#define NEXT(vpc) goto * SEM_ARGBUF (vpc) -> semantic.sem_case
SWITCH (sem, SEM_ARGBUF (vpc) -> semantic.sem_case)

#else /* ! WITH_SCACHE_PBB */

#define NEXT(vpc) BREAK (sem)
#ifdef __GNUC__
#if FAST_P
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_fast_lab)
#else
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_full_lab)
#endif
#else
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->num)
#endif

#endif /* ! WITH_SCACHE_PBB */

    {

  CASE (sem, INSN_X_INVALID) : /* --invalid-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
    /* Update the recorded pc in the cpu state struct.
       Only necessary for WITH_SCACHE case, but to avoid the
       conditional compilation ....  */
    SET_H_PC (pc);
    /* Virtual insns have zero size.  Overwrite vpc with address of next insn
       using the default-insn-bitsize spec.  When executing insns in parallel
       we may want to queue the fault and continue execution.  */
    vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
    vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_AFTER) : /* --after-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
    i960base_pbb_after (current_cpu, sem_arg);
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_BEFORE) : /* --before-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
    i960base_pbb_before (current_cpu, sem_arg);
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_CTI_CHAIN) : /* --cti-chain-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
#ifdef DEFINE_SWITCH
    vpc = i960base_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = i960base_pbb_cti_chain (current_cpu, sem_arg,
			       CPU_PBB_BR_TYPE (current_cpu),
			       CPU_PBB_BR_NPC (current_cpu));
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_CHAIN) : /* --chain-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
    vpc = i960base_pbb_chain (current_cpu, sem_arg);
#ifdef DEFINE_SWITCH
    BREAK (sem);
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_BEGIN) : /* --begin-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
#ifdef DEFINE_SWITCH
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = i960base_pbb_begin (current_cpu, FAST_P);
#else
    vpc = i960base_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULO) : /* mulo $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULO1) : /* mulo $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULO2) : /* mulo $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULO3) : /* mulo $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMO) : /* remo $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMO1) : /* remo $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMO2) : /* remo $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMO3) : /* remo $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVO) : /* divo $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVO1) : /* divo $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVO2) : /* divo $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVO3) : /* divo $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMI) : /* remi $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMI1) : /* remi $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMI2) : /* remi $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_REMI3) : /* remi $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVI) : /* divi $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVI1) : /* divi $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVI2) : /* divi $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVI3) : /* divi $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDO) : /* addo $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDO1) : /* addo $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDO2) : /* addo $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDO3) : /* addo $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUBO) : /* subo $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUBO1) : /* subo $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUBO2) : /* subo $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUBO3) : /* subo $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTBIT) : /* notbit $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, * FLD (i_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTBIT1) : /* notbit $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, FLD (f_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTBIT2) : /* notbit $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, * FLD (i_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTBIT3) : /* notbit $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, FLD (f_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_AND) : /* and $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_AND1) : /* and $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_AND2) : /* and $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_AND3) : /* and $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDNOT) : /* andnot $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDNOT1) : /* andnot $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDNOT2) : /* andnot $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDNOT3) : /* andnot $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SETBIT) : /* setbit $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, * FLD (i_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SETBIT1) : /* setbit $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, FLD (f_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SETBIT2) : /* setbit $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, * FLD (i_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SETBIT3) : /* setbit $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, FLD (f_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTAND) : /* notand $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTAND1) : /* notand $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTAND2) : /* notand $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOTAND3) : /* notand $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XOR) : /* xor $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XOR1) : /* xor $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XOR2) : /* xor $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XOR3) : /* xor $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_OR) : /* or $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_OR1) : /* or $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_OR2) : /* or $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_OR3) : /* or $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOR) : /* nor $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOR1) : /* nor $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOR2) : /* nor $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOR3) : /* nor $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XNOR) : /* xnor $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (* FLD (i_src1), * FLD (i_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XNOR1) : /* xnor $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (FLD (f_src1), * FLD (i_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XNOR2) : /* xnor $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (* FLD (i_src1), FLD (f_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XNOR3) : /* xnor $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (FLD (f_src1), FLD (f_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOT) : /* not $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (* FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOT1) : /* not $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOT2) : /* not $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (* FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOT3) : /* not $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORNOT) : /* ornot $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORNOT1) : /* ornot $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORNOT2) : /* ornot $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORNOT3) : /* ornot $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CLRBIT) : /* clrbit $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, * FLD (i_src1))), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CLRBIT1) : /* clrbit $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, FLD (f_src1))), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CLRBIT2) : /* clrbit $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, * FLD (i_src1))), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CLRBIT3) : /* clrbit $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, FLD (f_src1))), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLO) : /* shlo $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLO1) : /* shlo $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLO2) : /* shlo $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLO3) : /* shlo $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRO) : /* shro $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SRLSI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRO1) : /* shro $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SRLSI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRO2) : /* shro $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SRLSI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRO3) : /* shro $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SRLSI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLI) : /* shli $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLI1) : /* shli $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLI2) : /* shli $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHLI3) : /* shli $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRI) : /* shri $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (SRASI (* FLD (i_src2), 31)) : (SRASI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRI1) : /* shri $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (SRASI (* FLD (i_src2), 31)) : (SRASI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRI2) : /* shri $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (SRASI (FLD (f_src2), 31)) : (SRASI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SHRI3) : /* shri $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (SRASI (FLD (f_src2), 31)) : (SRASI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EMUL) : /* emul $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (* FLD (i_src1)), ZEXTSIDI (* FLD (i_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EMUL1) : /* emul $lit1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (FLD (f_src1)), ZEXTSIDI (* FLD (i_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EMUL2) : /* emul $src1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (* FLD (i_src1)), ZEXTSIDI (FLD (f_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EMUL3) : /* emul $lit1, $lit2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (FLD (f_src1)), ZEXTSIDI (FLD (f_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOV) : /* mov $src1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOV1) : /* mov $lit1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVL) : /* movl $src1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movq.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  SI tmp_sregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_sregno = FLD (f_src1);
  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (1))]);
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVL1) : /* movl $lit1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVT) : /* movt $src1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movq.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  SI tmp_sregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_sregno = FLD (f_src1);
  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (1))]);
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (2))]);
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVT1) : /* movt $lit1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movq.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVQ) : /* movq $src1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movq.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  SI tmp_sregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_sregno = FLD (f_src1);
  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (1))]);
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (2))]);
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (3))]);
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVQ1) : /* movq $lit1, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movq.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MODPC) : /* modpc $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_src2);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MODAC) : /* modac $src1, $src2, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_src2);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_OFFSET) : /* lda $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = FLD (f_offset);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_INDIRECT_OFFSET) : /* lda $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (FLD (f_offset), * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_INDIRECT) : /* lda ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_abase);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_INDIRECT_INDEX) : /* lda ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_DISP) : /* lda $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = FLD (f_optdisp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_INDIRECT_DISP) : /* lda $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = ADDSI (FLD (f_optdisp), * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_INDEX_DISP) : /* lda $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDA_INDIRECT_INDEX_DISP) : /* lda $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_OFFSET) : /* ld $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_INDIRECT_OFFSET) : /* ld $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_INDIRECT) : /* ld ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_INDIRECT_INDEX) : /* ld ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_DISP) : /* ld $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_INDIRECT_DISP) : /* ld $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_INDEX_DISP) : /* ld $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD_INDIRECT_INDEX_DISP) : /* ld $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_OFFSET) : /* ldob $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_INDIRECT_OFFSET) : /* ldob $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_INDIRECT) : /* ldob ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_INDIRECT_INDEX) : /* ldob ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_DISP) : /* ldob $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_INDIRECT_DISP) : /* ldob $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_INDEX_DISP) : /* ldob $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOB_INDIRECT_INDEX_DISP) : /* ldob $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_OFFSET) : /* ldos $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_INDIRECT_OFFSET) : /* ldos $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_INDIRECT) : /* ldos ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_INDIRECT_INDEX) : /* ldos ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_DISP) : /* ldos $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_INDIRECT_DISP) : /* ldos $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_INDEX_DISP) : /* ldos $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDOS_INDIRECT_INDEX_DISP) : /* ldos $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_OFFSET) : /* ldib $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_INDIRECT_OFFSET) : /* ldib $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_INDIRECT) : /* ldib ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_INDIRECT_INDEX) : /* ldib ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_DISP) : /* ldib $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_INDIRECT_DISP) : /* ldib $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_INDEX_DISP) : /* ldib $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIB_INDIRECT_INDEX_DISP) : /* ldib $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_OFFSET) : /* ldis $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_INDIRECT_OFFSET) : /* ldis $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_INDIRECT) : /* ldis ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_INDIRECT_INDEX) : /* ldis ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_DISP) : /* ldis $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_INDIRECT_DISP) : /* ldis $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_INDEX_DISP) : /* ldis $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDIS_INDIRECT_INDEX_DISP) : /* ldis $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_OFFSET) : /* ldl $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_offset);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_INDIRECT_OFFSET) : /* ldl $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_offset), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_INDIRECT) : /* ldl ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = * FLD (i_abase);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_INDIRECT_INDEX) : /* ldl ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_DISP) : /* ldl $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_optdisp);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_INDIRECT_DISP) : /* ldl $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_INDEX_DISP) : /* ldl $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDL_INDIRECT_INDEX_DISP) : /* ldl $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_OFFSET) : /* ldt $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_offset);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_INDIRECT_OFFSET) : /* ldt $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_offset), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_INDIRECT) : /* ldt ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = * FLD (i_abase);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_INDIRECT_INDEX) : /* ldt ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_DISP) : /* ldt $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_optdisp);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_INDIRECT_DISP) : /* ldt $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_INDEX_DISP) : /* ldt $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDT_INDIRECT_INDEX_DISP) : /* ldt $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_OFFSET) : /* ldq $offset, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_offset);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_INDIRECT_OFFSET) : /* ldq $offset($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_offset), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_INDIRECT) : /* ldq ($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = * FLD (i_abase);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_INDIRECT_INDEX) : /* ldq ($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_DISP) : /* ldq $optdisp, $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_optdisp);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_INDIRECT_DISP) : /* ldq $optdisp($abase), $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_INDEX_DISP) : /* ldq $optdisp[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDQ_INDIRECT_INDEX_DISP) : /* ldq $optdisp($abase)[$index*S$scale], $dst */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_OFFSET) : /* st $st_src, $offset */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_INDIRECT_OFFSET) : /* st $st_src, $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_INDIRECT) : /* st $st_src, ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_INDIRECT_INDEX) : /* st $st_src, ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_DISP) : /* st $st_src, $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_INDIRECT_DISP) : /* st $st_src, $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_INDEX_DISP) : /* st $st_src, $optdisp[$index*S$scale */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST_INDIRECT_INDEX_DISP) : /* st $st_src, $optdisp($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_OFFSET) : /* stob $st_src, $offset */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_INDIRECT_OFFSET) : /* stob $st_src, $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_INDIRECT) : /* stob $st_src, ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_INDIRECT_INDEX) : /* stob $st_src, ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_DISP) : /* stob $st_src, $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_INDIRECT_DISP) : /* stob $st_src, $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_INDEX_DISP) : /* stob $st_src, $optdisp[$index*S$scale */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOB_INDIRECT_INDEX_DISP) : /* stob $st_src, $optdisp($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_OFFSET) : /* stos $st_src, $offset */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_INDIRECT_OFFSET) : /* stos $st_src, $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_INDIRECT) : /* stos $st_src, ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_INDIRECT_INDEX) : /* stos $st_src, ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_DISP) : /* stos $st_src, $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_INDIRECT_DISP) : /* stos $st_src, $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_INDEX_DISP) : /* stos $st_src, $optdisp[$index*S$scale */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STOS_INDIRECT_INDEX_DISP) : /* stos $st_src, $optdisp($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_OFFSET) : /* stl $st_src, $offset */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_INDIRECT_OFFSET) : /* stl $st_src, $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_INDIRECT) : /* stl $st_src, ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_INDIRECT_INDEX) : /* stl $st_src, ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_DISP) : /* stl $st_src, $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_INDIRECT_DISP) : /* stl $st_src, $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_INDEX_DISP) : /* stl $st_src, $optdisp[$index*S$scale */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STL_INDIRECT_INDEX_DISP) : /* stl $st_src, $optdisp($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_OFFSET) : /* stt $st_src, $offset */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_INDIRECT_OFFSET) : /* stt $st_src, $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_INDIRECT) : /* stt $st_src, ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_INDIRECT_INDEX) : /* stt $st_src, ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_DISP) : /* stt $st_src, $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_INDIRECT_DISP) : /* stt $st_src, $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_INDEX_DISP) : /* stt $st_src, $optdisp[$index*S$scale */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STT_INDIRECT_INDEX_DISP) : /* stt $st_src, $optdisp($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_OFFSET) : /* stq $st_src, $offset */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_INDIRECT_OFFSET) : /* stq $st_src, $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_INDIRECT) : /* stq $st_src, ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_INDIRECT_INDEX) : /* stq $st_src, ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_DISP) : /* stq $st_src, $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_INDIRECT_DISP) : /* stq $st_src, $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_INDEX_DISP) : /* stq $st_src, $optdisp[$index*S$scale */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STQ_INDIRECT_INDEX_DISP) : /* stq $st_src, $optdisp($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBE_REG) : /* cmpobe $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBE_LIT) : /* cmpobe $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBNE_REG) : /* cmpobne $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBNE_LIT) : /* cmpobne $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBL_REG) : /* cmpobl $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBL_LIT) : /* cmpobl $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBLE_REG) : /* cmpoble $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LEUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBLE_LIT) : /* cmpoble $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LEUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBG_REG) : /* cmpobg $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBG_LIT) : /* cmpobg $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBGE_REG) : /* cmpobge $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GEUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPOBGE_LIT) : /* cmpobge $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GEUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBE_REG) : /* cmpibe $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBE_LIT) : /* cmpibe $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBNE_REG) : /* cmpibne $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBNE_LIT) : /* cmpibne $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBL_REG) : /* cmpibl $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBL_LIT) : /* cmpibl $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBLE_REG) : /* cmpible $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBLE_LIT) : /* cmpible $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBG_REG) : /* cmpibg $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBG_LIT) : /* cmpibg $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBGE_REG) : /* cmpibge $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPIBGE_LIT) : /* cmpibge $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BBC_REG) : /* bbc $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (ANDSI (SLLSI (1, * FLD (i_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BBC_LIT) : /* bbc $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (ANDSI (SLLSI (1, FLD (f_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BBS_REG) : /* bbs $br_src1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (SLLSI (1, * FLD (i_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BBS_LIT) : /* bbs $br_lit1, $br_src2, $br_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (SLLSI (1, FLD (f_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPI) : /* cmpi $src1, $src2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (* FLD (i_src1), * FLD (i_src2))) ? (4) : (EQSI (* FLD (i_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPI1) : /* cmpi $lit1, $src2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (FLD (f_src1), * FLD (i_src2))) ? (4) : (EQSI (FLD (f_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPI2) : /* cmpi $src1, $lit2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (* FLD (i_src1), FLD (f_src2))) ? (4) : (EQSI (* FLD (i_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPI3) : /* cmpi $lit1, $lit2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (FLD (f_src1), FLD (f_src2))) ? (4) : (EQSI (FLD (f_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPO) : /* cmpo $src1, $src2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (* FLD (i_src1), * FLD (i_src2))) ? (4) : (EQSI (* FLD (i_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPO1) : /* cmpo $lit1, $src2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (FLD (f_src1), * FLD (i_src2))) ? (4) : (EQSI (FLD (f_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPO2) : /* cmpo $src1, $lit2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (* FLD (i_src1), FLD (f_src2))) ? (4) : (EQSI (* FLD (i_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPO3) : /* cmpo $lit1, $lit2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul3.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (FLD (f_src1), FLD (f_src2))) ? (4) : (EQSI (FLD (f_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTNO_REG) : /* testno $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EQSI (CPU (h_cc), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTG_REG) : /* testg $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 1), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTE_REG) : /* teste $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 2), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTGE_REG) : /* testge $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 3), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTL_REG) : /* testl $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 4), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTNE_REG) : /* testne $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 5), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTLE_REG) : /* testle $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 6), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_TESTO_REG) : /* testo $br_src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 7), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNO) : /* bno $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (CPU (h_cc), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BG) : /* bg $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 1), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BE) : /* be $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 2), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGE) : /* bge $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 3), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BL) : /* bl $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 4), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNE) : /* bne $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 5), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLE) : /* ble $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 6), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BO) : /* bo $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 7), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_B) : /* b $ctrl_disp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bno.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BX_INDIRECT_OFFSET) : /* bx $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ADDSI (FLD (f_offset), * FLD (i_abase));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BX_INDIRECT) : /* bx ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = * FLD (i_abase);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BX_INDIRECT_INDEX) : /* bx ($abase)[$index*S$scale] */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BX_DISP) : /* bx $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = FLD (f_optdisp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BX_INDIRECT_DISP) : /* bx $optdisp($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ADDSI (FLD (f_optdisp), * FLD (i_abase));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLX_DISP) : /* callx $optdisp */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  tmp_temp = ANDSI (ADDSI (CPU (h_gr[((UINT) 1)]), 63), INVSI (63));
  {
    SI opval = ADDSI (pc, 8);
    CPU (h_gr[((UINT) 2)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0), CPU (h_gr[((UINT) 0)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4), CPU (h_gr[((UINT) 1)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8), CPU (h_gr[((UINT) 2)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12), CPU (h_gr[((UINT) 3)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16), CPU (h_gr[((UINT) 4)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20), CPU (h_gr[((UINT) 5)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24), CPU (h_gr[((UINT) 6)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28), CPU (h_gr[((UINT) 7)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32), CPU (h_gr[((UINT) 8)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36), CPU (h_gr[((UINT) 9)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40), CPU (h_gr[((UINT) 10)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44), CPU (h_gr[((UINT) 11)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48), CPU (h_gr[((UINT) 12)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52), CPU (h_gr[((UINT) 13)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56), CPU (h_gr[((UINT) 14)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60), CPU (h_gr[((UINT) 15)]));
  {
    USI opval = FLD (f_optdisp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 1)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 2)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 3)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 4)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 5)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 6)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 7)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 8)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 9)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 10)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 11)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 12)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 13)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 14)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 15)]) = 0xdeadbeef;
  {
    SI opval = CPU (h_gr[((UINT) 31)]);
    CPU (h_gr[((UINT) 0)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = tmp_temp;
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
  {
    SI opval = ADDSI (tmp_temp, 64);
    CPU (h_gr[((UINT) 1)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLX_INDIRECT) : /* callx ($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  tmp_temp = ANDSI (ADDSI (CPU (h_gr[((UINT) 1)]), 63), INVSI (63));
  {
    SI opval = ADDSI (pc, 4);
    CPU (h_gr[((UINT) 2)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0), CPU (h_gr[((UINT) 0)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4), CPU (h_gr[((UINT) 1)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8), CPU (h_gr[((UINT) 2)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12), CPU (h_gr[((UINT) 3)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16), CPU (h_gr[((UINT) 4)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20), CPU (h_gr[((UINT) 5)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24), CPU (h_gr[((UINT) 6)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28), CPU (h_gr[((UINT) 7)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32), CPU (h_gr[((UINT) 8)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36), CPU (h_gr[((UINT) 9)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40), CPU (h_gr[((UINT) 10)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44), CPU (h_gr[((UINT) 11)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48), CPU (h_gr[((UINT) 12)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52), CPU (h_gr[((UINT) 13)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56), CPU (h_gr[((UINT) 14)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60), CPU (h_gr[((UINT) 15)]));
  {
    USI opval = * FLD (i_abase);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 1)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 2)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 3)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 4)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 5)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 6)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 7)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 8)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 9)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 10)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 11)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 12)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 13)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 14)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 15)]) = 0xdeadbeef;
  {
    SI opval = CPU (h_gr[((UINT) 31)]);
    CPU (h_gr[((UINT) 0)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = tmp_temp;
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
  {
    SI opval = ADDSI (tmp_temp, 64);
    CPU (h_gr[((UINT) 1)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLX_INDIRECT_OFFSET) : /* callx $offset($abase) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  tmp_temp = ANDSI (ADDSI (CPU (h_gr[((UINT) 1)]), 63), INVSI (63));
  {
    SI opval = ADDSI (pc, 4);
    CPU (h_gr[((UINT) 2)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0), CPU (h_gr[((UINT) 0)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4), CPU (h_gr[((UINT) 1)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8), CPU (h_gr[((UINT) 2)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12), CPU (h_gr[((UINT) 3)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16), CPU (h_gr[((UINT) 4)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20), CPU (h_gr[((UINT) 5)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24), CPU (h_gr[((UINT) 6)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28), CPU (h_gr[((UINT) 7)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32), CPU (h_gr[((UINT) 8)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36), CPU (h_gr[((UINT) 9)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40), CPU (h_gr[((UINT) 10)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44), CPU (h_gr[((UINT) 11)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48), CPU (h_gr[((UINT) 12)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52), CPU (h_gr[((UINT) 13)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56), CPU (h_gr[((UINT) 14)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60), CPU (h_gr[((UINT) 15)]));
  {
    USI opval = ADDSI (FLD (f_offset), * FLD (i_abase));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 1)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 2)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 3)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 4)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 5)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 6)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 7)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 8)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 9)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 10)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 11)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 12)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 13)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 14)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 15)]) = 0xdeadbeef;
  {
    SI opval = CPU (h_gr[((UINT) 31)]);
    CPU (h_gr[((UINT) 0)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = tmp_temp;
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
  {
    SI opval = ADDSI (tmp_temp, 64);
    CPU (h_gr[((UINT) 1)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_RET) : /* ret */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    SI opval = CPU (h_gr[((UINT) 0)]);
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0));
CPU (h_gr[((UINT) 1)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4));
CPU (h_gr[((UINT) 2)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8));
CPU (h_gr[((UINT) 3)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12));
CPU (h_gr[((UINT) 4)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16));
CPU (h_gr[((UINT) 5)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20));
CPU (h_gr[((UINT) 6)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24));
CPU (h_gr[((UINT) 7)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28));
CPU (h_gr[((UINT) 8)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32));
CPU (h_gr[((UINT) 9)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36));
CPU (h_gr[((UINT) 10)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40));
CPU (h_gr[((UINT) 11)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44));
CPU (h_gr[((UINT) 12)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48));
CPU (h_gr[((UINT) 13)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52));
CPU (h_gr[((UINT) 14)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56));
CPU (h_gr[((UINT) 15)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60));
  {
    USI opval = CPU (h_gr[((UINT) 2)]);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLS) : /* calls $src1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_emul2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = i960_trap (current_cpu, pc, * FLD (i_src1));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_FMARK) : /* fmark */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = i960_breakpoint (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_FLUSHREG) : /* flushreg */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);


    }
  ENDSWITCH (sem) /* End of semantic switch.  */

  /* At this point `vpc' contains the next insn to execute.  */
}

#undef DEFINE_SWITCH
#endif /* DEFINE_SWITCH */

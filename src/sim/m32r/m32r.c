/* m32r simulator support code
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define WANT_CPU
#define WANT_CPU_M32R

#include "sim-main.h"

/* The contents of BUF are in target byte order.  */

void
m32r_fetch_register (sd, rn, buf)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);

  if (rn < 16)
    SETTWI (buf, GET_H_GR (rn));
  else if (rn < 21)
    SETTWI (buf, GET_H_CR (rn - 16));
  else switch (rn) {
    case PC_REGNUM:
      SETTWI (buf, GET_H_PC ());
      break;
    case ACCL_REGNUM:
      SETTWI (buf, GETLODI (GET_H_ACCUM ()));
      break;
    case ACCH_REGNUM:
      SETTWI (buf, GETHIDI (GET_H_ACCUM ()));
      break;
#if 0
    case 23: *reg = STATE_CPU_CPU (sd, 0)->h_cond;		break;
    case 24: *reg = STATE_CPU_CPU (sd, 0)->h_sm;		break;
    case 25: *reg = STATE_CPU_CPU (sd, 0)->h_bsm;		break;
    case 26: *reg = STATE_CPU_CPU (sd, 0)->h_ie;		break;
    case 27: *reg = STATE_CPU_CPU (sd, 0)->h_bie;		break;
    case 28: *reg = STATE_CPU_CPU (sd, 0)->h_bcarry;		break; /* rename: bc */
    case 29: memcpy (buf, &STATE_CPU_CPU (sd, 0)->h_bpc, sizeof(WI));	break; /* duplicate */
#endif
    default: abort ();
  }
}
 
/* The contents of BUF are in target byte order.  */

void
m32r_store_register (sd, rn, buf)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);

  if (rn < 16)
    SET_H_GR (rn, GETTWI (buf));
  else if (rn < 21)
    SET_H_CR (rn - 16, GETTWI (buf));
  else switch (rn) {
    case PC_REGNUM:
      SET_H_PC (GETTWI (buf));
      break;
    case ACCL_REGNUM:
      SETLODI (CPU (h_accum), GETTWI (buf));
      break;
    case ACCH_REGNUM:
      SETHIDI (CPU (h_accum), GETTWI (buf));
      break;
#if 0
    case 23: STATE_CPU_CPU (sd, 0)->h_cond   = *reg;			break;
    case 24: STATE_CPU_CPU (sd, 0)->h_sm     = *reg;			break;
    case 25: STATE_CPU_CPU (sd, 0)->h_bsm    = *reg;			break;
    case 26: STATE_CPU_CPU (sd, 0)->h_ie     = *reg;			break;
    case 27: STATE_CPU_CPU (sd, 0)->h_bie    = *reg;			break;
    case 28: STATE_CPU_CPU (sd, 0)->h_bcarry = *reg;			break; /* rename: bc */
    case 29: memcpy (&STATE_CPU_CPU (sd, 0)->h_bpc, buf, sizeof(DI));	break; /* duplicate */
#endif
  }
}

#if WITH_PROFILE_MODEL_P

void
m32r_model_mark_get_h_gr (SIM_CPU *cpu, ARGBUF *abuf)
{
  if ((CPU_CGEN_PROFILE (cpu)->h_gr & abuf->h_gr_get) != 0)
    {
      PROFILE_MODEL_LOAD_STALL_COUNT (CPU_PROFILE_DATA (cpu)) += 2;
      if (TRACE_INSN_P (cpu))
	cgen_trace_printf (cpu, " ; Load stall of 2 cycles.");
    }
}

void
m32r_model_mark_set_h_gr (SIM_CPU *cpu, ARGBUF *abuf)
{
}

void
m32r_model_mark_busy_reg (SIM_CPU *cpu, ARGBUF *abuf)
{
  CPU_CGEN_PROFILE (cpu)->h_gr = abuf->h_gr_set;
}

void
m32r_model_mark_unbusy_reg (SIM_CPU *cpu, ARGBUF *abuf)
{
  CPU_CGEN_PROFILE (cpu)->h_gr = 0;
}

#endif /* WITH_PROFILE_MODEL_P */

USI
m32r_h_cr_get (SIM_CPU *current_cpu, UINT cr)
{
  switch (cr)
    {
    case H_CR_PSW : /* psw */
      return ((CPU (h_bsm) << 15)
	      | (CPU (h_bie) << 14)
	      | (CPU (h_bcond) << 8)
	      | (CPU (h_sm) << 7)
	      | (CPU (h_ie) << 6)
	      | (CPU (h_cond) << 0));
    case H_CR_CBR : /* condition bit */
      return CPU (h_cond);
    case H_CR_SPI : /* interrupt stack pointer */
      if (! CPU (h_sm))
	return CPU (h_gr[H_GR_SP]);
      else
	return CPU (h_cr[H_CR_SPI]);
    case H_CR_SPU : /* user stack pointer */
      if (CPU (h_sm))
	return CPU (h_gr[H_GR_SP]);
      else
	return CPU (h_cr[H_CR_SPU]);
    case H_CR_BPC : /* backup pc */
      return CPU (h_bpc);
    case 4 : /* unused */
    case 5 : /* unused */
      return CPU (h_cr[cr]);
    default :
      return 0;
    }
}

void
m32r_h_cr_set (SIM_CPU *current_cpu, UINT cr, USI newval)
{
  switch (cr)
    {
    case H_CR_PSW : /* psw */
      {
	int old_sm = CPU (h_sm);
	CPU (h_bsm) = (newval & (1 << 15)) != 0;
	CPU (h_bie) = (newval & (1 << 14)) != 0;
	CPU (h_bcond) = (newval & (1 << 8)) != 0;
	CPU (h_sm) = (newval & (1 << 7)) != 0;
	CPU (h_ie) = (newval & (1 << 6)) != 0;
	CPU (h_cond) = (newval & (1 << 0)) != 0;
	/* When switching stack modes, update the registers.  */
	if (old_sm != CPU (h_sm))
	  {
	    if (old_sm)
	      {
		/* Switching user -> system.  */
		CPU (h_cr[H_CR_SPU]) = CPU (h_gr[H_GR_SP]);
		CPU (h_gr[H_GR_SP]) = CPU (h_cr[H_CR_SPI]);
	      }
	    else
	      {
		/* Switching system -> user.  */
		CPU (h_cr[H_CR_SPI]) = CPU (h_gr[H_GR_SP]);
		CPU (h_gr[H_GR_SP]) = CPU (h_cr[H_CR_SPU]);
	      }
	  }
	break;
      }
    case H_CR_CBR : /* condition bit */
      CPU (h_cond) = (newval & 1) != 0;
      break;
    case H_CR_SPI : /* interrupt stack pointer */
      if (! CPU (h_sm))
	CPU (h_gr[H_GR_SP]) = newval;
      else
	CPU (h_cr[H_CR_SPI]) = newval;
      break;
    case H_CR_SPU : /* user stack pointer */
      if (CPU (h_sm))
	CPU (h_gr[H_GR_SP]) = newval;
      else
	CPU (h_cr[H_CR_SPU]) = newval;
      break;
    case H_CR_BPC : /* backup pc */
      CPU (h_bpc) = newval;
      /* drop through */
    case 4 : /* unused */
    case 5 : /* unused */
      CPU (h_cr[cr]) = newval;
      break;
    default :
      /* ignore */
      break;
    }
}

/* Misc. support for CPU family m32r.

This file is machine generated with CGEN.

Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.

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

#define WANT_CPU
#define WANT_CPU_M32R

#include "sim-main.h"

/* Get the value of h-pc.  */

USI
m32r_h_pc_get (SIM_CPU *current_cpu)
{
  return CPU (h_pc);
}

/* Set a value for h-pc.  */

void
m32r_h_pc_set (SIM_CPU *current_cpu, USI newval)
{
  CPU (h_pc) = newval;
}

/* Get the value of h-gr.  */

SI
m32r_h_gr_get (SIM_CPU *current_cpu, UINT regno)
{
  return CPU (h_gr[regno]);
}

/* Set a value for h-gr.  */

void
m32r_h_gr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  CPU (h_gr[regno]) = newval;
}

/* Get the value of h-accum.  */

DI
m32r_h_accum_get (SIM_CPU *current_cpu)
{
  return CPU (h_accum);
}

/* Set a value for h-accum.  */

void
m32r_h_accum_set (SIM_CPU *current_cpu, DI newval)
{
  CPU (h_accum) = newval;
}

/* Get the value of h-cond.  */

UBI
m32r_h_cond_get (SIM_CPU *current_cpu)
{
  return CPU (h_cond);
}

/* Set a value for h-cond.  */

void
m32r_h_cond_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_cond) = newval;
}

/* Get the value of h-sm.  */

UBI
m32r_h_sm_get (SIM_CPU *current_cpu)
{
  return CPU (h_sm);
}

/* Set a value for h-sm.  */

void
m32r_h_sm_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_sm) = newval;
}

/* Get the value of h-bsm.  */

UBI
m32r_h_bsm_get (SIM_CPU *current_cpu)
{
  return CPU (h_bsm);
}

/* Set a value for h-bsm.  */

void
m32r_h_bsm_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_bsm) = newval;
}

/* Get the value of h-ie.  */

UBI
m32r_h_ie_get (SIM_CPU *current_cpu)
{
  return CPU (h_ie);
}

/* Set a value for h-ie.  */

void
m32r_h_ie_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_ie) = newval;
}

/* Get the value of h-bie.  */

UBI
m32r_h_bie_get (SIM_CPU *current_cpu)
{
  return CPU (h_bie);
}

/* Set a value for h-bie.  */

void
m32r_h_bie_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_bie) = newval;
}

/* Get the value of h-bcond.  */

UBI
m32r_h_bcond_get (SIM_CPU *current_cpu)
{
  return CPU (h_bcond);
}

/* Set a value for h-bcond.  */

void
m32r_h_bcond_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_bcond) = newval;
}

/* Get the value of h-bpc.  */

SI
m32r_h_bpc_get (SIM_CPU *current_cpu)
{
  return CPU (h_bpc);
}

/* Set a value for h-bpc.  */

void
m32r_h_bpc_set (SIM_CPU *current_cpu, SI newval)
{
  CPU (h_bpc) = newval;
}

/* Get the value of h-lock.  */

UBI
m32r_h_lock_get (SIM_CPU *current_cpu)
{
  return CPU (h_lock);
}

/* Set a value for h-lock.  */

void
m32r_h_lock_set (SIM_CPU *current_cpu, UBI newval)
{
  CPU (h_lock) = newval;
}


/* collection of junk waiting time to sort out
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef M32R_SIM_H
#define M32R_SIM_H

#define PC_REGNUM	21
#define ACCL_REGNUM	22
#define ACCH_REGNUM	23

/* This is invoked by the nop pattern in the .cpu file.  */
#if WITH_PROFILE_INSN_P
#define PROFILE_COUNT_FILLNOPS(cpu, addr) \
do { \
  if (CPU_PROFILE_FLAGS (cpu) [PROFILE_INSN_IDX] \
      && (addr & 3) != 0) \
    ++ CPU_M32R_MISC_PROFILE (cpu).fillnop_count; \
} while (0)
#else
#define PROFILE_COUNT_FILLNOPS(cpu, addr)
#endif

#define GETTWI GETTSI
#define SETTWI SETTSI

/* Result of semantic function is one of
   - next address, branch only
   - NEW_PC_SKIP, sc/snc insn
   - NEW_PC_2, 2 byte non-branch non-sc/snc insn
   - NEW_PC_4, 4 byte non-branch insn
   The special values have bit 1 set so it's cheap to distinguish them.  */
#define NEW_PC_BASE 0xffff0001
#define NEW_PC_SKIP NEW_PC_BASE
#define NEW_PC_2 (NEW_PC_BASE + 2)
#define NEW_PC_4 (NEW_PC_BASE + 4)
#define NEW_PC_BRANCH_P(addr) (((addr) & 1) == 0)


/* This macro is emitted by the generator to record branch addresses.  */
#define BRANCH_NEW_PC(var, addr) \
do { var = (addr); } while (0)

/* Support for the MSPR register.
   This is needed in order for overlays to work correctly with the scache.  */

/* Address of the MSPR register (Cache Purge Control Register).  */
#define MSPR_ADDR 0xfffffff7

/* Serial device addresses.  */
#define UART_INCHAR_ADDR	0xff102013
#define UART_OUTCHAR_ADDR	0xff10200f
#define UART_STATUS_ADDR	0xff102006
#define UART_INPUT_EMPTY	0x4
#define UART_OUTPUT_EMPTY	0x1

/* Start address and length of all device support.  */
#define M32R_DEVICE_ADDR	0xff000000
#define M32R_DEVICE_LEN		0x00ffffff

/* sim_core_attach device argument.  */
extern device m32r_devices;

/* FIXME: Temporary, until device support ready.  */
struct _device { int foo; };

#endif /* M32R_SIM_H */

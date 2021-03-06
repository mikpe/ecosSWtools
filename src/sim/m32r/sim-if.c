/* Main simulator entry points for the M32R.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

#include "sim-main.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"
#include "targ-vals.h"

static SIM_RC alloc_cpu (SIM_DESC, struct _bfd *, char **);
static void free_state (SIM_DESC);
static void print_m32r_misc_cpu (SIM_CPU *cpu, int verbose);

/* Records simulator descriptor so utilities like m32r_dump_regs can be
   called from gdb.  */
SIM_DESC current_state;

/* Scan the args and bfd to see what kind of cpus are in use and allocate
   space for them.  */

static SIM_RC
alloc_cpu (SIM_DESC sd, struct _bfd *abfd, char **argv)
{
  /* Compute the size of the SIM_CPU struct.
     For now its the max of all the possible sizes.  */
  int size = 0;
  const MACH *mach;

  for (mach = &machs[0]; MACH_NAME (mach) != NULL; ++mach)
    {
      int mach_size = IMP_PROPS_SIM_CPU_SIZE (MACH_IMP_PROPS (mach));
      size = mach_size > size ? mach_size : size;
    }
  if (size == 0)
    abort ();

  /* `sizeof (SIM_CPU)' is the size of the generic part, and `size' is the
     size of the cpu-specific part.  */
  STATE_CPU (sd, 0) = zalloc (sizeof (SIM_CPU) + size);

  return SIM_RC_OK;
}

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_CPU (sd, 0))
    zfree (STATE_CPU (sd, 0));
  sim_state_free (sd);
}

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (kind, callback, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback *callback;
     struct _bfd *abfd;
     char **argv;
{
  SIM_DESC sd = sim_state_alloc (kind, callback);

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (alloc_cpu (sd, abfd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

#if 0 /* FIXME: pc is in mach-specific struct */
  /* FIXME: watchpoints code shouldn't need this */
  {
    SIM_CPU *current_cpu = STATE_CPU (sd, 0);
    STATE_WATCHPOINTS (sd)->pc = &(PC);
    STATE_WATCHPOINTS (sd)->sizeof_pc = sizeof (PC);
  }
#endif

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

#if 0 /* FIXME: 'twould be nice if we could do this */
  /* These options override any module options.
     Obviously ambiguity should be avoided, however the caller may wish to
     augment the meaning of an option.  */
  if (extra_options != NULL)
    sim_add_option_table (sd, extra_options);
#endif

  /* Allocate core managed memory */
  sim_do_commandf (sd, "memory region 0,0x%lx", M32R_DEFAULT_MEM_SIZE);

  /* Allocate a handler for the MSPR register.  */
  sim_core_attach (sd, NULL,
		   0 /*level*/,
		   access_read_write,
		   0 /*space ???*/,
		   M32R_DEVICE_ADDR, M32R_DEVICE_LEN /*nr_bytes*/,
		   0 /*modulo*/,
		   &m32r_devices,
		   NULL /*buffer*/);

  /* getopt will print the error message so we just have to exit if this fails.
     FIXME: Hmmm...  in the case of gdb we need getopt to call
     print_filtered.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      free_state (sd);
      return 0;
    }

  /* check for/establish the a reference program image */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL),
			   abfd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      free_state (sd);
      return 0;
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      free_state (sd);
      return 0;
    }

  /* Initialize various cgen things not done by common framework.  */
  cgen_init (sd);

  {
    int i;

    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	/* Only needed for profiling, but the structure member is small.  */
	memset (& CPU_M32R_MISC_PROFILE (STATE_CPU (sd, i)), 0,
		sizeof (CPU_M32R_MISC_PROFILE (STATE_CPU (sd, i))));
	/* Hook in callback for reporting these stats */
	PROFILE_INFO_CPU_CALLBACK (CPU_PROFILE_DATA (STATE_CPU (sd, i)))
	  = print_m32r_misc_cpu;
      }
  }

  /* Store in a global so things like sparc32_dump_regs can be invoked
     from the gdb command line.  */
  current_state = sd;

  return sd;
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  sim_module_uninstall (sd);
}

SIM_RC
sim_create_inferior (sd, abfd, argv, envp)
     SIM_DESC sd;
     struct _bfd *abfd;
     char **argv;
     char **envp;
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  SIM_ADDR addr;

  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  h_pc_set (current_cpu, addr);

#if 0
  STATE_ARGV (sd) = sim_copy_argv (argv);
  STATE_ENVP (sd) = sim_copy_argv (envp);
#endif

  return SIM_RC_OK;
}

int
sim_stop (SIM_DESC sd)
{
  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_m32r :
      return m32r_engine_stop (sd);
    default :
      abort ();
    }
}

void
sim_resume (sd, step, siggnal)
     SIM_DESC sd;
     int step, siggnal;
{
  sim_module_resume (sd);

  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_m32r :
      m32r_engine_run (sd, step, siggnal);
      break;
    default :
      abort ();
    }

  sim_module_suspend (sd);
}

/* PROFILE_CPU_CALLBACK */

static void
print_m32r_misc_cpu (SIM_CPU *cpu, int verbose)
{
  SIM_DESC sd = CPU_STATE (cpu);
  char buf[20];

  if (CPU_PROFILE_FLAGS (cpu) [PROFILE_INSN_IDX])
    {
      sim_io_printf (sd, "Miscellaneous Statistics\n\n");
      sim_io_printf (sd, "  %-*s %s\n\n",
		     PROFILE_LABEL_WIDTH, "Fill nops:",
		     sim_add_commas (buf, sizeof (buf),
				     CPU_M32R_MISC_PROFILE (cpu).fillnop_count));
    }
}

/* The contents of BUF are in target byte order.  */

int
sim_fetch_register (sd, rn, buf, length)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
     int length;
{
  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_m32r :
      m32r_fetch_register (sd, rn, buf);
      break;
    default :
      abort ();
    }
  return -1;
}
 
/* The contents of BUF are in target byte order.  */

int
sim_store_register (sd, rn, buf, length)
     SIM_DESC sd;
     int rn;
     unsigned char *buf;
     int length;
{
  switch (STATE_ARCHITECTURE (sd)->mach)
    {
    case bfd_mach_m32r :
      m32r_store_register (sd, rn, buf);
      break;
    default :
      abort ();
    }
  return -1;
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char *cmd;
{ 
  if (sim_args_command (sd, cmd) != SIM_RC_OK)
    sim_io_eprintf (sd, "Unknown command `%s'\n", cmd);
}

/* The semantic code invokes this for illegal (unrecognized) instructions.  */

void
sim_engine_illegal_insn (current_cpu, pc)
     SIM_CPU *current_cpu;
     PCADDR pc;
{
  sim_engine_halt (CPU_STATE (current_cpu), current_cpu, NULL, pc,
		   sim_stopped, SIM_SIGILL);
}

/* Utility fns to access registers, without knowing the current mach.
   FIXME: Machine generate?  */

USI
h_pc_get (SIM_CPU *current_cpu)
{
  switch (STATE_ARCHITECTURE (CPU_STATE (current_cpu))->mach)
    {
    case bfd_mach_m32r :
      return m32r_h_pc_get (current_cpu);
    default :
      abort ();
    }
}

void
h_pc_set (SIM_CPU *current_cpu, USI newval)
{
  switch (STATE_ARCHITECTURE (CPU_STATE (current_cpu))->mach)
    {
    case bfd_mach_m32r :
      m32r_h_pc_set (current_cpu, newval);
      break;
    default :
      abort ();
    }
}

SI
h_gr_get (SIM_CPU *current_cpu, UINT regno)
{
  switch (STATE_ARCHITECTURE (CPU_STATE (current_cpu))->mach)
    {
    case bfd_mach_m32r :
      return m32r_h_gr_get (current_cpu, regno);
    default :
      abort ();
    }
}

void
h_gr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  switch (STATE_ARCHITECTURE (CPU_STATE (current_cpu))->mach)
    {
    case bfd_mach_m32r :
      m32r_h_gr_set (current_cpu, regno, newval);
      break;
    default :
      abort ();
    }
}

/* Read/write functions for system call interface.  */

static int
syscall_read_mem (host_callback *cb, struct cb_syscall *sc,
		  unsigned long taddr, char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes);
}

static int
syscall_write_mem (host_callback *cb, struct cb_syscall *sc,
		   unsigned long taddr, const char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
}

/* Trap support.
   The result is the pc address to continue at.  */

USI
do_trap (SIM_CPU *current_cpu, int num)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  host_callback *cb = STATE_CALLBACK (sd);

#ifdef SIM_HAVE_BREAKPOINTS
  /* Check for breakpoints "owned" by the simulator first, regardless
     of --environment.  */
  if (num == 1)
    {
      /* First try sim-break.c.  If it's a breakpoint the simulator "owns"
	 it doesn't return.  Otherwise it returns and let's us try.  */
      sim_handle_breakpoint (sd, current_cpu, h_pc_get (current_cpu));
      /* Fall through.  */
    }
#endif

  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    {
      /* The new pc is the trap vector entry.
	 We assume there's a branch there to some handler.  */
      USI new_pc = num * 4;
      return new_pc;
    }

  switch (num)
    {
    case 0 :
      /* Trap 0 is used for system calls.  */
      {
	CB_SYSCALL s;

	CB_SYSCALL_INIT (&s);
	s.func = h_gr_get (current_cpu, 0);
	s.arg1 = h_gr_get (current_cpu, 1);
	s.arg2 = h_gr_get (current_cpu, 2);
	s.arg3 = h_gr_get (current_cpu, 3);

	if (s.func == TARGET_SYS_exit)
	  {
	    sim_engine_halt (sd, current_cpu, NULL, h_pc_get (current_cpu),
			     sim_exited, s.arg1);
	  }

	s.p1 = (PTR) sd;
	s.p2 = (PTR) current_cpu;
	s.read_mem = syscall_read_mem;
	s.write_mem = syscall_write_mem;
	cb_syscall (STATE_CALLBACK (sd), &s);
	h_gr_set (current_cpu, 2, s.errcode);
	h_gr_set (current_cpu, 0, s.result);
	h_gr_set (current_cpu, 1, s.result2);
	break;
      }

    case 1:	/* breakpoint trap */
      sim_engine_halt (sd, current_cpu, NULL, NULL_CIA,
		       sim_stopped, SIM_SIGTRAP);
      break;

    default :
      /* Unless environment operating, ignore other traps.  */
      break;
    }

  /* Fake an "rte" insn.  */
  return (h_pc_get (current_cpu) & -4) + 4;
}

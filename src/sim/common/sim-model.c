/* Model support.
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

#include "sim-main.h"
#include "libiberty.h"
#include "sim-options.h"
#include "sim-io.h"
#include "sim-assert.h"

static void model_set (SIM_DESC, sim_cpu *, const MODEL *);

static DECLARE_OPTION_HANDLER (model_option_handler);

#define OPTION_MODEL (OPTION_START + 0)

static const OPTION model_options[] = {
  { {"model", required_argument, NULL, OPTION_MODEL},
      '\0', "MODEL", "Specify model to simulate",
      model_option_handler },
  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL }
};

static SIM_RC
model_option_handler (SIM_DESC sd, sim_cpu *cpu, int opt,
		      char *arg, int is_command)
{
  int n;

  switch (opt)
    {
    case OPTION_MODEL :
      {
	const MODEL *model = sim_model_lookup (arg);
	if (! model)
	  {
	    sim_io_eprintf (sd, "unknown model `%s'", arg);
	    return SIM_RC_FAIL;
	  }
	sim_model_set (sd, cpu, model);
	break;
      }
    }

  return SIM_RC_OK;
}

SIM_RC
sim_model_install (SIM_DESC sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  sim_add_option_table (sd, NULL, model_options);

  return SIM_RC_OK;
}

/* Subroutine of sim_model_set to set the model for one cpu.  */

static void
model_set (SIM_DESC sd, sim_cpu *cpu, const MODEL *model)
{
  CPU_MACH (cpu) = MODEL_MACH (model);
  CPU_MODEL (cpu) = model;
  (* MACH_INIT_CPU (MODEL_MACH (model))) (cpu);
}

/* Set the current model of CPU to MODEL.
   If CPU is NULL, all cpus are set to MODEL.  */

void
sim_model_set (SIM_DESC sd, sim_cpu *cpu, const MODEL *model)
{
  if (! cpu)
    {
      int c;

      for (c = 0; c < MAX_NR_PROCESSORS; ++c)
	if (STATE_CPU (sd, c))
	  model_set (sd, STATE_CPU (sd, c), model);
    }
  else
    {
      model_set (sd, cpu, model);
    }
}

/* Look up model named NAME.
   Result is pointer to MODEL entry or NULL if not found.  */

const MODEL *
sim_model_lookup (const char *name)
{
  const MACH **machp;
  const MODEL *model;

  for (machp = & sim_machs[0]; *machp != NULL; ++machp)
    {
      for (model = MACH_MODELS (*machp); MODEL_NAME (model) != NULL; ++model)
	{
	  if (strcmp (MODEL_NAME (model), name) == 0)
	    return model;
	}
    }
  return NULL;
}

/* Look up machine named NAME.
   Result is pointer to MACH entry or NULL if not found.  */

const MACH *
sim_mach_lookup (const char *name)
{
  const MACH **machp;

  for (machp = & sim_machs[0]; *machp != NULL; ++machp)
    {
      if (strcmp (MACH_NAME (*machp), name) == 0)
	return *machp;
    }
  return NULL;
}

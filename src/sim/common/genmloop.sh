# This shell script emits a C file. -*- C -*-
# Generate the main loop of the simulator.
# Syntax: genmloop.sh /bin/sh [options] cpu mainloop.in
# Options: [-mono|-multi] -scache -fast -parallel
#
# -scache: use the scache
# -fast: include support for fast execution in addition to full featured mode
# -parallel: cpu can execute multiple instructions parallely
#
# FIXME: "multi" support is wip.

# TODO
# - move this C code to mainloop.in
# - keep genmloop.sh
# - build exec.in from .cpu file
# - have each cpu provide handwritten cycle.in
# - integrate with common/sim-engine.[ch]
# - for sparc, have two main loops, outer one handles delay slot when npc != 0
#   - inner loop does not handle delay slots, pc = pc + 4

type=mono
#scache=
#fast=
#parallel=

shell=$1 ; shift

while true
do
	case $1 in
	-mono) type=mono ;;
	-multi) type=multi ;;
	-no-scache) ;;
	-scache) scache=yes ;;
	-no-fast) ;;
	-fast) fast=yes ;;
	-no-parallel) ;;
	-parallel) parallel=yes ;;
	*) break ;;
	esac
	shift
done

cpu=$1
file=$2

cat <<EOF
/* This file is is generated by the genmloop script.  DO NOT EDIT! */

/* Main loop for CGEN-based simulators.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of the GNU simulators.

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

/* We want the scache version of SEM_ARG.
   This is used by the switch() version of the semantic code.  */
EOF

if [ x$scache = xyes ] ; then
	echo "#define SCACHE_P"
else
	echo '/*#define SCACHE_P*/'
	echo '#undef WITH_SCACHE'
	echo '#define WITH_SCACHE 0'
fi

cat <<EOF

#define WANT_CPU
#define WANT_CPU_@CPU@

#include "sim-main.h"
#include "bfd.h"
#include "cgen-mem.h"
#include "cgen-ops.h"
#include "cpu-opc.h"
#include "cpu-sim.h"
#include "sim-assert.h"

/* Tell sim_main_loop to use the cache if it's active.
   Collecting profile data and tracing slow us down so we don't do them in
   "fast mode".
   There are 2 possibilities on 2 axes:
   - use or don't use the cache
   - run normally (full featured) or run fast
   Supporting all four possibilities in one executable is a bit much but
   supporting full/fast seems reasonable.
   If the cache is configured in it is always used.
   ??? Need to see whether it speeds up profiling significantly or not.
   Speeding up tracing doesn't seem worth it.
   ??? Sometimes supporting more than one set of semantic functions will make
   the simulator too large - this should be configurable.
*/

#if WITH_SCACHE
#define RUN_FAST_P(cpu) (STATE_RUN_FAST_P (CPU_STATE (cpu)))
#else
#define RUN_FAST_P(cpu) 0
#endif

#ifndef SIM_PRE_EXEC_HOOK
#define SIM_PRE_EXEC_HOOK(state)
#endif

#ifndef SIM_POST_EXEC_HOOK
#define SIM_POST_EXEC_HOOK(state)
#endif

#if 0 /* FIXME:experiment */
/* "sc" is local to the calling function.
   It is done this way to keep the internals of the implementation out of
   the description file.  */
#define EXTRACT(cpu, pc, insn, sc, num, fast_p) \
@cpu@_extract (cpu, pc, insn, sc + num, fast_p)

#define EXECUTE(cpu, sc, num, fast_p) \
@cpu@_execute (cpu, sc + num, fast_p)
#endif

#define GET_ATTR(cpu, num, attr) \
CGEN_INSN_ATTR (sc[num].argbuf.opcode, CGEN_INSN_##attr)

EOF

${SHELL} $file support

cat <<EOF

static volatile int keep_running;
/* Want to measure simulator speed even in fast mode.  */
static unsigned long insn_count;
static SIM_ELAPSED_TIME start_time;

/* Forward decls of cpu-specific functions.  */
static void engine_resume (SIM_DESC, int, int);
static void engine_resume_full (SIM_DESC);
${scache+static void engine_resume_fast (SIM_DESC);}

int
@cpu@_engine_stop (SIM_DESC sd)
{
  keep_running = 0;
  return 1;
}

void
@cpu@_engine_run (SIM_DESC sd, int step, int siggnal)
{
#if WITH_SCACHE
  if (USING_SCACHE_P (sd))
    scache_flush (sd);
#endif
  engine_resume (sd, step, siggnal);
}

static void
engine_resume (SIM_DESC sd, int step, int siggnal)
{
  sim_cpu *current_cpu = STATE_CPU (sd, 0);
  /* These are volatile to survive setjmp.  */
  volatile sim_cpu *cpu = current_cpu;
  volatile sim_engine *engine = STATE_ENGINE (sd);
  jmp_buf buf;
  int jmpval;

  keep_running = ! step;
  start_time = sim_elapsed_time_get ();
  /* FIXME: Having this global can slow things down a teensy bit.
     After things are working see about moving engine_resume_{full,fast}
     back into this function.  */
  insn_count = 0;

  engine->jmpbuf = &buf;
  if (setjmp (buf))
    {
      /* Account for the last insn executed.  */
      ++insn_count;

      engine->jmpbuf = NULL;
      TRACE_INSN_FINI ((sim_cpu *) cpu);
      PROFILE_EXEC_TIME (CPU_PROFILE_DATA (cpu))
	+= sim_elapsed_time_since (start_time);
      PROFILE_TOTAL_INSN_COUNT (CPU_PROFILE_DATA (cpu))
	+= insn_count;

      return;
    }

  /* ??? Restart support to be added in time.  */

  /* The computed goto switch can be used, and while the number of blocks
     may swamp the relatively few that this function contains, when running
     with the scache we put the actual semantic code in their own
     functions.  */

EOF

if [ x$fast = xyes ] ; then
	cat <<EOF
  if (step
      || !RUN_FAST_P (current_cpu))
    engine_resume_full (sd);
  else
    engine_resume_fast (sd);
EOF
else
	cat <<EOF
  engine_resume_full (sd);
EOF
fi

cat <<EOF

  /* If the loop exits, either we single-stepped or engine_stop was called.
     In either case we need to call engine_halt: to properly exit this
     function we must go through the setjmp executed above.  */
  if (step)
    sim_engine_halt (sd, current_cpu, NULL, NULL_CIA, sim_stopped, SIM_SIGTRAP);
  sim_engine_halt (sd, current_cpu, NULL, NULL_CIA, sim_stopped, SIM_SIGINT);
}

EOF

##########################################################################

if [ x$scache = xyes ] ; then
	cat <<EOF

static void
engine_resume_full (SIM_DESC sd)
{
#define FAST_P 0
  /* current_{state,cpu} exist for the generated code to use.  */
  SIM_DESC current_state = sd;
  sim_cpu *current_cpu = STATE_CPU (sd, 0);
${parallel+  int icount = 0;}

EOF

# Any initialization code before looping starts.
# Note that this code may declare some locals.
${SHELL} $file init

if [ x$parallel = xyes ] ; then
cat << EOF

#if defined (HAVE_PARALLEL_EXEC) && defined (__GNUC__)
  {
    static read_init_p = 0;
    if (! read_init_p)
      {
/* ??? Later maybe paste read.c in when building mainloop.c.  */
#define DEFINE_LABELS
#include "readx.c"
	read_init_p = 1;
      }
  }
#endif

EOF
fi

cat <<EOF

  do
    {
      /* FIXME: Later check every insn for events and such.  */

      SIM_PRE_EXEC_HOOK (current_cpu);

      {
	unsigned int hash;
	SCACHE *sc;
	PCADDR pc = PC;

	/* First step: look up current insn in hash table.  */
	hash = SCACHE_HASH_PC (sd, pc);
	sc = CPU_SCACHE_CACHE (current_cpu) + hash;

	/* If the entry isn't the one we want (cache miss),
	   fetch and decode the instruction.  */
	if (sc->argbuf.addr != pc)
	  {
	    insn_t insn;

	    PROFILE_COUNT_SCACHE_MISS (current_cpu);

/* begin full-extract-scache */
EOF

${SHELL} $file full-extract-scache

cat <<EOF
/* end full-extract-scache */
	  }
	else
	  {
	    PROFILE_COUNT_SCACHE_HIT (current_cpu);
	    /* Make core access statistics come out right.
	       The size is a guess, but it's currently not used either.  */
	    PROFILE_COUNT_CORE (current_cpu, pc, 2, exec_map);
	  }

/* begin full-exec-scache */
EOF

${SHELL} $file full-exec-scache

cat <<EOF
/* end full-exec-scache */
      }

      SIM_POST_EXEC_HOOK (current_cpu);

      ++insn_count;
    }
  while (keep_running);
#undef FAST_P
}
EOF

##########################################################################

else # ! WITH_SCACHE
	cat <<EOF

static void
engine_resume_full (SIM_DESC sd)
{
#define FAST_P 0
  SIM_DESC current_state = sd;
  sim_cpu *current_cpu = STATE_CPU (sd, 0);
  SCACHE cache[MAX_LIW_INSNS];
  SCACHE *sc = &cache[0];
${parallel+  int icount = 0;}

EOF

# Any initialization code before looping starts.
# Note that this code may declare some locals.
${SHELL} $file init

if [ x$parallel = xyes ] ; then
cat << EOF

#if defined (HAVE_PARALLEL_EXEC) && defined (__GNUC__)
  {
    static read_init_p = 0;
    if (! read_init_p)
      {
/* ??? Later maybe paste read.c in when building mainloop.c.  */
#define DEFINE_LABELS
#include "readx.c"
	read_init_p = 1;
      }
  }
#endif

EOF
fi

cat <<EOF

  do
    {
      /* FIXME: Later check every insn for events and such.  */

      SIM_PRE_EXEC_HOOK (current_cpu);

      {
/* begin full-{extract,exec}-noscache */
EOF

${SHELL} $file full-extract-noscache
echo ""
${SHELL} $file full-exec-noscache

cat <<EOF
/* end full-{extract,exec}-noscache */
      }

      SIM_POST_EXEC_HOOK (current_cpu);

      ++insn_count;
    }
  while (keep_running);
#undef FAST_P
}

EOF
fi # ! WITH_SCACHE

##########################################################################

if [ x$fast = xyes ] ; then
    if [ x$scache = xyes ] ; then
	cat <<EOF

static void
engine_resume_fast (SIM_DESC sd)
{
#define FAST_P 1
  SIM_DESC current_state = sd;
  sim_cpu *current_cpu = STATE_CPU (sd, 0);
${parallel+  int icount = 0;}

EOF

# Any initialization code before looping starts.
# Note that this code may declare some locals.
${SHELL} $file init

if [ x$parallel = xyes ] ; then
cat << EOF

#if defined (HAVE_PARALLEL_EXEC) && defined (__GNUC__)
  {
    static read_init_p = 0;
    if (! read_init_p)
      {
/* ??? Later maybe paste read.c in when building mainloop.c.  */
#define DEFINE_LABELS
#include "readx.c"
	read_init_p = 1;
      }
  }
#endif

EOF
fi

cat <<EOF

#if defined (WITH_SEM_SWITCH_FAST) && defined (__GNUC__)
  {
    static decode_init_p = 0;
    if (! decode_init_p)
      {
/* ??? Later maybe paste sem-switch.c in when building mainloop.c.  */
#define DEFINE_LABELS
#include "sem-switch.c"
	decode_init_p = 1;
      }
  }
#endif

  do
    {
      {
	unsigned int hash;
	SCACHE *sc;
	PCADDR pc = PC;

	/* First step: look up current insn in hash table.  */
	hash = SCACHE_HASH_PC (sd, pc);
	sc = CPU_SCACHE_CACHE (current_cpu) + hash;

	/* If the entry isn't the one we want (cache miss),
	   fetch and decode the instruction.  */
	if (sc->argbuf.addr != pc)
	  {
	    insn_t insn;

/* begin fast-extract-scache */
EOF

${SHELL} $file fast-extract-scache

cat <<EOF
/* end fast-extract-scache */
	  }

/* begin fast-exec-scache */
EOF

${SHELL} $file fast-exec-scache

cat <<EOF
/* end fast-exec-scache */

      }

      ++insn_count;
    }
  while (keep_running);
#undef FAST_P
}

EOF

##########################################################################

else # ! WITH_SCACHE
	cat <<EOF

static void
engine_resume_fast (SIM_DESC sd)
{
#define FAST_P 1
  SIM_DESC current_state = sd;
  sim_cpu *current_cpu = STATE_CPU (sd, 0);
  SCACHE cache[MAX_LIW_INSNS];
  SCACHE *sc = &cache[0];
${parallel+  int icount = 0;}

EOF

# Any initialization code before looping starts.
# Note that this code may declare some locals.
${SHELL} $file init

if [ x$parallel = xyes ] ; then
cat << EOF

#if defined (HAVE_PARALLEL_EXEC) && defined (__GNUC__)
  {
    static read_init_p = 0;
    if (! read_init_p)
      {
/* ??? Later maybe paste read.c in when building mainloop.c.  */
#define DEFINE_LABELS
#include "readx.c"
	read_init_p = 1;
      }
  }
#endif

EOF
fi

cat <<EOF

  do
    {
/* begin fast-{extract,exec}-noscache */
EOF

${SHELL} $file fast-extract-noscache
echo ""
${SHELL} $file fast-exec-noscache

cat <<EOF
/* end fast-{extract,exec}-noscache */

      ++insn_count;
    }
  while (keep_running);
#undef FAST_P
}

EOF

    fi # ! WITH_SCACHE
fi # -fast

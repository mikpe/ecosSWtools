/* Move registers around to reduce number of move instructions needed.
   Copyright (C) 1987, 88, 89, 92-97, 1998 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


/* This module looks for cases where matching constraints would force
   an instruction to need a reload, and this reload would be a register
   to register move.  It then attempts to change the registers used by the
   instruction to avoid the move instruction.  */

#include "config.h"
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/* Must precede rtl.h for FFS.  */
#include <stdio.h>

#include "rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "output.h"
#include "reload.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "expr.h"
#include "insn-flags.h"

static int optimize_reg_copy_1	PROTO((rtx, rtx, rtx));
static void optimize_reg_copy_2	PROTO((rtx, rtx, rtx));
static void optimize_reg_copy_3	PROTO((rtx, rtx, rtx));

struct match {
  int with[MAX_RECOG_OPERANDS];
  enum { READ, WRITE, READWRITE } use[MAX_RECOG_OPERANDS];
  int commutative[MAX_RECOG_OPERANDS];
  int early_clobber[MAX_RECOG_OPERANDS];
};

static int find_matches PROTO((rtx, struct match *));
static int fixup_match_1 PROTO((rtx, rtx, rtx, rtx, rtx, int, int, int, FILE *))
;
static int reg_is_remote_constant_p PROTO((rtx, rtx, rtx));
static int stable_but_for_p PROTO((rtx, rtx, rtx));
static int loop_depth;

#ifdef AUTO_INC_DEC

/* INC_INSN is an instruction that adds INCREMENT to REG.
   Try to fold INC_INSN as a post/pre in/decrement into INSN.
   Iff INC_INSN_SET is nonzero, inc_insn has a destination different from src.
   Return nonzero for success.  */
static int
try_auto_increment (insn, inc_insn, inc_insn_set, reg, increment, pre)
     rtx reg, insn, inc_insn ,inc_insn_set;
     HOST_WIDE_INT increment;
     int pre;
{
  enum rtx_code inc_code;

  rtx pset = single_set (insn);
  if (pset)
    {
      /* Can't use the size of SET_SRC, we might have something like
	 (sign_extend:SI (mem:QI ...  */
      rtx use = find_use_as_address (pset, reg, 0);
      if (use != 0 && use != (rtx) 1)
	{
	  int size = GET_MODE_SIZE (GET_MODE (use));
	  if (0
#ifdef HAVE_POST_INCREMENT
	      || (pre == 0 && (inc_code = POST_INC, increment == size))
#endif
#ifdef HAVE_PRE_INCREMENT
	      || (pre == 1 && (inc_code = PRE_INC, increment == size))
#endif
#ifdef HAVE_POST_DECREMENT
	      || (pre == 0 && (inc_code = POST_DEC, increment == -size))
#endif
#ifdef HAVE_PRE_DECREMENT
	      || (pre == 1 && (inc_code = PRE_DEC, increment == -size))
#endif
	  )
	    {
	      if (inc_insn_set)
		validate_change
		  (inc_insn, 
		   &SET_SRC (inc_insn_set),
		   XEXP (SET_SRC (inc_insn_set), 0), 1);
	      validate_change (insn, &XEXP (use, 0),
			       gen_rtx_fmt_e (inc_code, Pmode, reg), 1);
	      if (apply_change_group ())
		{
		  REG_NOTES (insn)
		    = gen_rtx_EXPR_LIST (REG_INC,
					 reg, REG_NOTES (insn));
		  if (! inc_insn_set)
		    {
		      PUT_CODE (inc_insn, NOTE);
		      NOTE_LINE_NUMBER (inc_insn) = NOTE_INSN_DELETED;
		      NOTE_SOURCE_FILE (inc_insn) = 0;
		    }
		  return 1;
		}
	    }
	}
    }
  return 0;
}
#endif  /* AUTO_INC_DEC */

static int *regno_src_regno;

/* Indicate how good a choice REG (which appears as a source) is to replace
   a destination register with.  The higher the returned value, the better
   the choice.  The main objective is to avoid using a register that is
   a candidate for tying to a hard register, since the output might in
   turn be a candidate to be tied to a different hard register.  */
int
replacement_quality(reg)
     rtx reg;
{
  int src_regno;

  /* Bad if this isn't a register at all.  */
  if (GET_CODE (reg) != REG)
    return 0;

  /* If this register is not meant to get a hard register,
     it is a poor choice.  */
  if (REG_LIVE_LENGTH (REGNO (reg)) < 0)
    return 0;

  src_regno = regno_src_regno[REGNO (reg)];

  /* If it was not copied from another register, it is fine.  */
  if (src_regno < 0)
    return 3;

  /* Copied from a hard register?  */
  if (src_regno < FIRST_PSEUDO_REGISTER)
    return 1;

  /* Copied from a pseudo register - not as bad as from a hard register,
     yet still cumbersome, since the register live length will be lengthened
     when the registers get tied.  */
  return 2;
}

/* INSN is a copy from SRC to DEST, both registers, and SRC does not die
   in INSN.

   Search forward to see if SRC dies before either it or DEST is modified,
   but don't scan past the end of a basic block.  If so, we can replace SRC
   with DEST and let SRC die in INSN. 

   This will reduce the number of registers live in that range and may enable
   DEST to be tied to SRC, thus often saving one register in addition to a
   register-register copy.  */

static int
optimize_reg_copy_1 (insn, dest, src)
     rtx insn;
     rtx dest;
     rtx src;
{
  rtx p, q;
  rtx note;
  rtx dest_death = 0;
  int sregno = REGNO (src);
  int dregno = REGNO (dest);

  /* We don't want to mess with hard regs if register classes are small. */
  if (sregno == dregno
      || (SMALL_REGISTER_CLASSES
	  && (sregno < FIRST_PSEUDO_REGISTER
	      || dregno < FIRST_PSEUDO_REGISTER))
      /* We don't see all updates to SP if they are in an auto-inc memory
	 reference, so we must disallow this optimization on them.  */
      || sregno == STACK_POINTER_REGNUM || dregno == STACK_POINTER_REGNUM)
    return 0;

  for (p = NEXT_INSN (insn); p; p = NEXT_INSN (p))
    {
      if (GET_CODE (p) == CODE_LABEL || GET_CODE (p) == JUMP_INSN
	  || (GET_CODE (p) == NOTE
	      && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
		  || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
	break;

      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).
	 A CALL_INSN might be the last insn of a basic block, if it is inside
	 an EH region.  There is no easy way to tell, so we just always break
	 when we see a CALL_INSN if flag_exceptions is nonzero.  */
      if (flag_exceptions && GET_CODE (p) == CALL_INSN)
	break;

      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
	continue;

      if (reg_set_p (src, p) || reg_set_p (dest, p)
	  /* Don't change a USE of a register.  */
	  || (GET_CODE (PATTERN (p)) == USE
	      && reg_overlap_mentioned_p (src, XEXP (PATTERN (p), 0))))
	break;

      /* See if all of SRC dies in P.  This test is slightly more
	 conservative than it needs to be.  */
      if ((note = find_regno_note (p, REG_DEAD, sregno)) != 0
	  && GET_MODE (XEXP (note, 0)) == GET_MODE (src))
	{
	  int failed = 0;
	  int length = 0;
	  int d_length = 0;
	  int n_calls = 0;
	  int d_n_calls = 0;

	  /* We can do the optimization.  Scan forward from INSN again,
	     replacing regs as we go.  Set FAILED if a replacement can't
	     be done.  In that case, we can't move the death note for SRC.
	     This should be rare.  */

	  /* Set to stop at next insn.  */
	  for (q = next_real_insn (insn);
	       q != next_real_insn (p);
	       q = next_real_insn (q))
	    {
	      if (reg_overlap_mentioned_p (src, PATTERN (q)))
		{
		  /* If SRC is a hard register, we might miss some
		     overlapping registers with validate_replace_rtx,
		     so we would have to undo it.  We can't if DEST is
		     present in the insn, so fail in that combination
		     of cases.  */
		  if (sregno < FIRST_PSEUDO_REGISTER
		      && reg_mentioned_p (dest, PATTERN (q)))
		    failed = 1;

		  /* Replace all uses and make sure that the register
		     isn't still present.  */
		  else if (validate_replace_rtx (src, dest, q)
			   && (sregno >= FIRST_PSEUDO_REGISTER
			       || ! reg_overlap_mentioned_p (src,
							     PATTERN (q))))
		    {
		      /* We assume that a register is used exactly once per
			 insn in the updates below.  If this is not correct,
			 no great harm is done.  */
		      if (sregno >= FIRST_PSEUDO_REGISTER)
			REG_N_REFS (sregno) -= loop_depth;
		      if (dregno >= FIRST_PSEUDO_REGISTER)
			REG_N_REFS (dregno) += loop_depth;
		    }
		  else
		    {
		      validate_replace_rtx (dest, src, q);
		      failed = 1;
		    }
		}

	      /* Count the insns and CALL_INSNs passed.  If we passed the
		 death note of DEST, show increased live length.  */
	      length++;
	      if (dest_death)
		d_length++;

	      /* If the insn in which SRC dies is a CALL_INSN, don't count it
		 as a call that has been crossed.  Otherwise, count it.  */
	      if (q != p && GET_CODE (q) == CALL_INSN)
		{
		  n_calls++;
		  if (dest_death)
		    d_n_calls++;
		}

	      /* If DEST dies here, remove the death note and save it for
		 later.  Make sure ALL of DEST dies here; again, this is
		 overly conservative.  */
	      if (dest_death == 0
		  && (dest_death = find_regno_note (q, REG_DEAD, dregno)) != 0)
		{
		  if (GET_MODE (XEXP (dest_death, 0)) != GET_MODE (dest))
		    failed = 1, dest_death = 0;
		  else
		    remove_note (q, dest_death);
		}
	    }

	  if (! failed)
	    {
	      if (sregno >= FIRST_PSEUDO_REGISTER)
		{
		  if (REG_LIVE_LENGTH (sregno) >= 0)
		    {
		      REG_LIVE_LENGTH (sregno) -= length;
		      /* REG_LIVE_LENGTH is only an approximation after
			 combine if sched is not run, so make sure that we
			 still have a reasonable value.  */
		      if (REG_LIVE_LENGTH (sregno) < 2)
			REG_LIVE_LENGTH (sregno) = 2;
		    }

		  REG_N_CALLS_CROSSED (sregno) -= n_calls;
		}

	      if (dregno >= FIRST_PSEUDO_REGISTER)
		{
		  if (REG_LIVE_LENGTH (dregno) >= 0)
		    REG_LIVE_LENGTH (dregno) += d_length;

		  REG_N_CALLS_CROSSED (dregno) += d_n_calls;
		}

	      /* Move death note of SRC from P to INSN.  */
	      remove_note (p, note);
	      XEXP (note, 1) = REG_NOTES (insn);
	      REG_NOTES (insn) = note;
	    }

	  /* Put death note of DEST on P if we saw it die.  */
	  if (dest_death)
	    {
	      XEXP (dest_death, 1) = REG_NOTES (p);
	      REG_NOTES (p) = dest_death;
	    }

	  return ! failed;
	}

      /* If SRC is a hard register which is set or killed in some other
	 way, we can't do this optimization.  */
      else if (sregno < FIRST_PSEUDO_REGISTER
	       && dead_or_set_p (p, src))
	break;
    }
  return 0;
}

/* INSN is a copy of SRC to DEST, in which SRC dies.  See if we now have
   a sequence of insns that modify DEST followed by an insn that sets
   SRC to DEST in which DEST dies, with no prior modification of DEST.
   (There is no need to check if the insns in between actually modify
   DEST.  We should not have cases where DEST is not modified, but
   the optimization is safe if no such modification is detected.)
   In that case, we can replace all uses of DEST, starting with INSN and
   ending with the set of SRC to DEST, with SRC.  We do not do this
   optimization if a CALL_INSN is crossed unless SRC already crosses a
   call or if DEST dies before the copy back to SRC.

   It is assumed that DEST and SRC are pseudos; it is too complicated to do
   this for hard registers since the substitutions we may make might fail.  */

static void
optimize_reg_copy_2 (insn, dest, src)
     rtx insn;
     rtx dest;
     rtx src;
{
  rtx p, q;
  rtx set;
  int sregno = REGNO (src);
  int dregno = REGNO (dest);

  for (p = NEXT_INSN (insn); p; p = NEXT_INSN (p))
    {
      if (GET_CODE (p) == CODE_LABEL || GET_CODE (p) == JUMP_INSN
	  || (GET_CODE (p) == NOTE
	      && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
		  || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
	break;

      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).
	 A CALL_INSN might be the last insn of a basic block, if it is inside
	 an EH region.  There is no easy way to tell, so we just always break
	 when we see a CALL_INSN if flag_exceptions is nonzero.  */
      if (flag_exceptions && GET_CODE (p) == CALL_INSN)
	break;

      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
	continue;

      set = single_set (p);
      if (set && SET_SRC (set) == dest && SET_DEST (set) == src
	  && find_reg_note (p, REG_DEAD, dest))
	{
	  /* We can do the optimization.  Scan forward from INSN again,
	     replacing regs as we go.  */

	  /* Set to stop at next insn.  */
	  for (q = insn; q != NEXT_INSN (p); q = NEXT_INSN (q))
	    if (GET_RTX_CLASS (GET_CODE (q)) == 'i')
	      {
		if (reg_mentioned_p (dest, PATTERN (q)))
		  {
		    PATTERN (q) = replace_rtx (PATTERN (q), dest, src);

		    /* We assume that a register is used exactly once per
		       insn in the updates below.  If this is not correct,
		       no great harm is done.  */
		    REG_N_REFS (dregno) -= loop_depth;
		    REG_N_REFS (sregno) += loop_depth;
		  }


	      if (GET_CODE (q) == CALL_INSN)
		{
		  REG_N_CALLS_CROSSED (dregno)--;
		  REG_N_CALLS_CROSSED (sregno)++;
		}
	      }

	  remove_note (p, find_reg_note (p, REG_DEAD, dest));
	  REG_N_DEATHS (dregno)--;
	  remove_note (insn, find_reg_note (insn, REG_DEAD, src));
	  REG_N_DEATHS (sregno)--;
	  return;
	}

      if (reg_set_p (src, p)
	  || find_reg_note (p, REG_DEAD, dest)
	  || (GET_CODE (p) == CALL_INSN && REG_N_CALLS_CROSSED (sregno) == 0))
	break;
    }
}
/* INSN is a ZERO_EXTEND or SIGN_EXTEND of SRC to DEST.
   Look if SRC dies there, and if it is only set once, by loading
   it from memory.  If so, try to encorporate the zero/sign extension
   into the memory read, change SRC to the mode of DEST, and alter
   the remaining accesses to use the appropriate SUBREG.  This allows
   SRC and DEST to be tied later.  */
static void
optimize_reg_copy_3 (insn, dest, src)
     rtx insn;
     rtx dest;
     rtx src;
{
  rtx src_reg = XEXP (src, 0);
  int src_no = REGNO (src_reg);
  int dst_no = REGNO (dest);
  rtx p, set, subreg;
  enum machine_mode old_mode;

  if (src_no < FIRST_PSEUDO_REGISTER
      || dst_no < FIRST_PSEUDO_REGISTER
      || ! find_reg_note (insn, REG_DEAD, src_reg)
      || REG_N_SETS (src_no) != 1)
    return;
  for (p = PREV_INSN (insn); ! reg_set_p (src_reg, p); p = PREV_INSN (p))
    {
      if (GET_CODE (p) == CODE_LABEL || GET_CODE (p) == JUMP_INSN
	  || (GET_CODE (p) == NOTE
	      && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
		  || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
	return;

      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).
	 A CALL_INSN might be the last insn of a basic block, if it is inside
	 an EH region.  There is no easy way to tell, so we just always break
	 when we see a CALL_INSN if flag_exceptions is nonzero.  */
      if (flag_exceptions && GET_CODE (p) == CALL_INSN)
	return;

      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
	continue;
    }
  if (! (set = single_set (p))
      || GET_CODE (SET_SRC (set)) != MEM
      || SET_DEST (set) != src_reg)
    return;
  old_mode = GET_MODE (src_reg);
  PUT_MODE (src_reg, GET_MODE (src));
  XEXP (src, 0) = SET_SRC (set);
  if (! validate_change (p, &SET_SRC (set), src, 0))
    {
      PUT_MODE (src_reg, old_mode);
      XEXP (src, 0) = src_reg;
      return;
    }
  subreg = gen_rtx_SUBREG (old_mode, src_reg, 0);
  while (p = NEXT_INSN (p), p != insn)
    {
      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
	continue;
      validate_replace_rtx (src_reg, subreg, p);
    }
  validate_replace_rtx (src, src_reg, insn);
}

/* Return whether REG is set in only one location, and is set to a
   constant, but is set in a different basic block from INSN (an
   instructions which uses REG).  In this case REG is equivalent to a
   constant, and we don't want to break that equivalence, because that
   may increase register pressure and make reload harder.  If REG is
   set in the same basic block as INSN, we don't worry about it,
   because we'll probably need a register anyhow (??? but what if REG
   is used in a different basic block as well as this one?).  FIRST is
   the first insn in the function.  */

static int
reg_is_remote_constant_p (reg, insn, first)
     rtx reg;
     rtx insn;
     rtx first;
{
  register rtx p;

  if (REG_N_SETS (REGNO (reg)) != 1)
    return 0;

  /* Look for the set.  */
  for (p = LOG_LINKS (insn); p; p = XEXP (p, 1))
    {
      rtx s;

      if (REG_NOTE_KIND (p) != 0)
	continue;
      s = single_set (XEXP (p, 0));
      if (s != 0
	  && GET_CODE (SET_DEST (s)) == REG
	  && REGNO (SET_DEST (s)) == REGNO (reg))
	{
	  /* The register is set in the same basic block.  */
	  return 0;
	}
    }

  for (p = first; p && p != insn; p = NEXT_INSN (p))
    {
      rtx s;

      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
	continue;
      s = single_set (p);
      if (s != 0
	  && GET_CODE (SET_DEST (s)) == REG
	  && REGNO (SET_DEST (s)) == REGNO (reg))
	{
	  /* This is the instruction which sets REG.  If there is a
             REG_EQUAL note, then REG is equivalent to a constant.  */
	  if (find_reg_note (p, REG_EQUAL, NULL_RTX))
	    return 1;
	  return 0;
	}
    }

  return 0;
}

/* cse disrupts preincrement / postdecrement squences when it finds a
   hard register as ultimate source, like the frame pointer.  */
int
fixup_match_2 (insn, dst, src, offset, regmove_dump_file)
     rtx insn, dst, src, offset;
     FILE *regmove_dump_file;
{
  rtx p, dst_death = 0;
  int length, num_calls = 0;

  /* If SRC dies in INSN, we'd have to move the death note.  This is
     considered to be very unlikely, so we just skip the optimization
     in this case.  */
  if (find_regno_note (insn, REG_DEAD, REGNO (src)))
    return 0;

  /* Scan backward to find the first instruction that sets DST.  */

  for (length = 0, p = PREV_INSN (insn); p; p = PREV_INSN (p))
    {
      rtx pset;

      if (GET_CODE (p) == CODE_LABEL
          || GET_CODE (p) == JUMP_INSN
          || (GET_CODE (p) == NOTE
              && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
                  || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
        break;

      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).
	 A CALL_INSN might be the last insn of a basic block, if it is inside
	 an EH region.  There is no easy way to tell, so we just always break
	 when we see a CALL_INSN if flag_exceptions is nonzero.  */
      if (flag_exceptions && GET_CODE (p) == CALL_INSN)
	break;

      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
        continue;

      if (find_regno_note (p, REG_DEAD, REGNO (dst)))
	dst_death = p;
      if (! dst_death)
	length++;

      pset = single_set (p);
      if (pset && SET_DEST (pset) == dst
	  && GET_CODE (SET_SRC (pset)) == PLUS
	  && XEXP (SET_SRC (pset), 0) == src
	  && GET_CODE (XEXP (SET_SRC (pset), 1)) == CONST_INT)
        {
	  HOST_WIDE_INT newconst
	    = INTVAL (offset) - INTVAL (XEXP (SET_SRC (pset), 1));
	  if (validate_change (insn, &PATTERN (insn),
			       gen_addsi3 (dst, dst, GEN_INT (newconst)), 0))
	    {
	      /* Remove the death note for DST from DST_DEATH.  */
	      if (dst_death)
		{
		  remove_death (REGNO (dst), dst_death);
		  REG_LIVE_LENGTH (REGNO (dst)) += length;
		  REG_N_CALLS_CROSSED (REGNO (dst)) += num_calls;
		}

	      REG_N_REFS (REGNO (dst)) += loop_depth;
	      REG_N_REFS (REGNO (src)) -= loop_depth;

	      if (regmove_dump_file)
		fprintf (regmove_dump_file,
			 "Fixed operand of insn %d.\n",
			  INSN_UID (insn));

#ifdef AUTO_INC_DEC
	      for (p = PREV_INSN (insn); p; p = PREV_INSN (p))
		{
		  if (GET_CODE (p) == CODE_LABEL
		      || GET_CODE (p) == JUMP_INSN
		      || (GET_CODE (p) == NOTE
			  && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
			      || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
		    break;
		  if (reg_overlap_mentioned_p (dst, PATTERN (p)))
		    {
		      if (try_auto_increment (p, insn, 0, dst, newconst, 0))
			return 1;
		      break;
		    }
		}
	      for (p = NEXT_INSN (insn); p; p = NEXT_INSN (p))
		{
		  if (GET_CODE (p) == CODE_LABEL
		      || GET_CODE (p) == JUMP_INSN
		      || (GET_CODE (p) == NOTE
			  && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
			      || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
		    break;
		  if (reg_overlap_mentioned_p (dst, PATTERN (p)))
		    {
		      try_auto_increment (p, insn, 0, dst, newconst, 1);
		      break;
		    }
		}
#endif
	      return 1;
	    }
        }

      if (reg_set_p (dst, PATTERN (p)))
        break;

      /* If we have passed a call instruction, and the
         pseudo-reg SRC is not already live across a call,
         then don't perform the optimization.  */
      /* reg_set_p is overly conservative for CALL_INSNS, thinks that all
	 hard regs are clobbered.  Thus, we only use it for src for
	 non-call insns.  */
      if (GET_CODE (p) == CALL_INSN)
        {
	  if (! dst_death)
	    num_calls++;

          if (REG_N_CALLS_CROSSED (REGNO (src)) == 0)
            break;

	  if (call_used_regs [REGNO (dst)]
	      || find_reg_fusage (p, CLOBBER, dst))
	    break;
        }
      else if (reg_set_p (src, PATTERN (p)))
        break;
    }

  return 0;
}

void
regmove_optimize (f, nregs, regmove_dump_file)
     rtx f;
     int nregs;
     FILE *regmove_dump_file;
{
  rtx insn;
  struct match match;
  int pass;
  int maxregnum = max_reg_num (), i;

  regno_src_regno = (int *)alloca (sizeof *regno_src_regno * maxregnum);
  for (i = maxregnum; --i >= 0; ) regno_src_regno[i] = -1;

  /* A forward/backward pass.  Replace output operands with input operands.  */

  loop_depth = 1;

  for (pass = 0; pass <= 2; pass++)
    {
      if (! flag_regmove && pass >= flag_expensive_optimizations)
	return;

      if (regmove_dump_file)
	fprintf (regmove_dump_file, "Starting %s pass...\n",
		 pass ? "backward" : "forward");

      for (insn = pass ? get_last_insn () : f; insn;
	   insn = pass ? PREV_INSN (insn) : NEXT_INSN (insn))
	{
	  rtx set;
	  int insn_code_number;
	  int operand_number, match_number;

	  if (GET_CODE (insn) == NOTE)
	    {
	      if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_BEG)
		loop_depth++;
	      else if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_END)
		loop_depth--;
	    }

	  set = single_set (insn);
	  if (! set)
	    continue;

	  if (flag_expensive_optimizations && ! pass
	      && (GET_CODE (SET_SRC (set)) == SIGN_EXTEND
		  || GET_CODE (SET_SRC (set)) == ZERO_EXTEND)
	      && GET_CODE (XEXP (SET_SRC (set), 0)) == REG
	      && GET_CODE (SET_DEST(set)) == REG)
	    optimize_reg_copy_3 (insn, SET_DEST (set), SET_SRC (set));

	  if (flag_expensive_optimizations && ! pass
	      && GET_CODE (SET_SRC (set)) == REG
	      && GET_CODE (SET_DEST(set)) == REG)
	    {
	      /* If this is a register-register copy where SRC is not dead,
		 see if we can optimize it.  If this optimization succeeds,
		 it will become a copy where SRC is dead.  */
	      if ((find_reg_note (insn, REG_DEAD, SET_SRC (set))
		   || optimize_reg_copy_1 (insn, SET_DEST (set), SET_SRC (set)))
		  && REGNO (SET_DEST (set)) >= FIRST_PSEUDO_REGISTER)
		{
		  /* Similarly for a pseudo-pseudo copy when SRC is dead.  */
		  if (REGNO (SET_SRC (set)) >= FIRST_PSEUDO_REGISTER)
		    optimize_reg_copy_2 (insn, SET_DEST (set), SET_SRC (set));
		  if (regno_src_regno[REGNO (SET_DEST (set))] < 0
		      && SET_SRC (set) != SET_DEST (set))
		    {
		      int srcregno = REGNO (SET_SRC(set));
		      if (regno_src_regno[srcregno] >= 0)
			srcregno = regno_src_regno[srcregno];
		      regno_src_regno[REGNO (SET_DEST (set))] = srcregno;
		    }
		}
	    }
#ifdef REGISTER_CONSTRAINTS
	  insn_code_number
	    = find_matches (insn, &match);

	  if (insn_code_number < 0)
	    continue;

	  /* Now scan through the operands looking for a source operand
	     which is supposed to match the destination operand.
	     Then scan forward for an instruction which uses the dest
	     operand.
	     If it dies there, then replace the dest in both operands with
	     the source operand.  */

	  for (operand_number = 0;
	       operand_number < insn_n_operands[insn_code_number];
	       operand_number++)
	    {
	      rtx src, dst, src_subreg;
	      enum reg_class src_class, dst_class;

	      match_number = match.with[operand_number];

	      /* Nothing to do if the two operands aren't supposed to match.  */
	      if (match_number < 0)
		continue;

	      src = recog_operand[operand_number];
	      dst = recog_operand[match_number];

	      if (GET_CODE (src) != REG)
		continue;

	      src_subreg = src;
	      if (GET_CODE (dst) == SUBREG
		  && GET_MODE_SIZE (GET_MODE (dst))
		     >= GET_MODE_SIZE (GET_MODE (SUBREG_REG (dst))))
		{
		  src_subreg
		    = gen_rtx_SUBREG (GET_MODE (SUBREG_REG (dst)),
				      src, SUBREG_WORD (dst));
		  dst = SUBREG_REG (dst);
		}
	      if (GET_CODE (dst) != REG
		  || REGNO (dst) < FIRST_PSEUDO_REGISTER)
		continue;

	      if (REGNO (src) < FIRST_PSEUDO_REGISTER)
		{
		  if (match.commutative[operand_number] < operand_number)
		    regno_src_regno[REGNO (dst)] = REGNO (src);
		  continue;
		}

	      if (REG_LIVE_LENGTH (REGNO (src)) < 0)
		continue;

	      /* operand_number/src must be a read-only operand, and
		 match_operand/dst must be a write-only operand.  */
	      if (match.use[operand_number] != READ
		  || match.use[match_number] != WRITE)
		continue;

	      if (match.early_clobber[match_number]
		  && count_occurrences (PATTERN (insn), src) > 1)
		continue;

	      /* Make sure match_operand is the destination.  */
	      if (recog_operand[match_number] != SET_DEST (set))
		continue;

	      /* If the operands already match, then there is nothing to do.  */
	      /* But in the commutative case, we might find a better match.  */
	      if (operands_match_p (src, dst)
		  || (match.commutative[operand_number] >= 0
		      && operands_match_p (recog_operand[match.commutative
							 [operand_number]], dst)
		      && (replacement_quality (recog_operand[match.commutative
							     [operand_number]])
			  >= replacement_quality (src))))
		continue;

	      src_class = reg_preferred_class (REGNO (src));
	      dst_class = reg_preferred_class (REGNO (dst));
	      if (src_class != dst_class
		  && (! reg_class_subset_p (src_class, dst_class)
		      || CLASS_LIKELY_SPILLED_P (src_class))
		  && (! reg_class_subset_p (dst_class, src_class)
		      || CLASS_LIKELY_SPILLED_P (dst_class)))
		continue;
	  
	      if (fixup_match_1 (insn, set, src, src_subreg, dst, pass,
				 operand_number, match_number,
				 regmove_dump_file))
		break;
	    }
	}
    }

  /* A backward pass.  Replace input operands with output operands.  */

  if (regmove_dump_file)
    fprintf (regmove_dump_file, "Starting backward pass...\n");

  loop_depth = 1;

  for (insn = get_last_insn (); insn; insn = PREV_INSN (insn))
    {
      if (GET_CODE (insn) == NOTE)
	{
	  if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_END)
	    loop_depth++;
	  else if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_BEG)
	    loop_depth--;
	}
      if (GET_RTX_CLASS (GET_CODE (insn)) == 'i')
	{
	  int insn_code_number = find_matches (insn, &match);
	  int operand_number, match_number;
	  
	  if (insn_code_number < 0)
	    continue;

	  /* Now scan through the operands looking for a destination operand
	     which is supposed to match a source operand.
	     Then scan backward for an instruction which sets the source
	     operand.  If safe, then replace the source operand with the
	     dest operand in both instructions.  */

	  for (operand_number = 0;
	       operand_number < insn_n_operands[insn_code_number];
	       operand_number++)
	    {
	      rtx set, p, src, dst;
	      rtx src_note, dst_note;
	      int success = 0;
	      int num_calls = 0;
	      enum reg_class src_class, dst_class;
	      int length;

	      match_number = match.with[operand_number];

	      /* Nothing to do if the two operands aren't supposed to match.  */
	      if (match_number < 0)
		continue;

	      dst = recog_operand[match_number];
	      src = recog_operand[operand_number];

	      if (GET_CODE (src) != REG)
		continue;

	      if (GET_CODE (dst) != REG
		  || REGNO (dst) < FIRST_PSEUDO_REGISTER
		  || REG_LIVE_LENGTH (REGNO (dst)) < 0)
		continue;

	      /* If the operands already match, then there is nothing to do.  */
	      if (operands_match_p (src, dst)
		  || (match.commutative[operand_number] >= 0
		      && operands_match_p (recog_operand[match.commutative[operand_number]], dst)))
		continue;

	      set = single_set (insn);
	      if (! set)
		continue;

	      /* match_number/dst must be a write-only operand, and
		 operand_operand/src must be a read-only operand.  */
	      if (match.use[operand_number] != READ
		  || match.use[match_number] != WRITE)
		continue;

	      if (match.early_clobber[match_number]
		  && count_occurrences (PATTERN (insn), src) > 1)
		continue;

	      /* Make sure match_number is the destination.  */
	      if (recog_operand[match_number] != SET_DEST (set))
		continue;

	      if (REGNO (src) < FIRST_PSEUDO_REGISTER)
		{
		  if (GET_CODE (SET_SRC (set)) == PLUS
		      && GET_CODE (XEXP (SET_SRC (set), 1)) == CONST_INT
		      && XEXP (SET_SRC (set), 0) == src
		      && fixup_match_2 (insn, dst, src,
					XEXP (SET_SRC (set), 1),
					regmove_dump_file))
		    break;
		  continue;
		}
	      src_class = reg_preferred_class (REGNO (src));
	      dst_class = reg_preferred_class (REGNO (dst));
	      if (src_class != dst_class
		  && (! reg_class_subset_p (src_class, dst_class)
		      || CLASS_LIKELY_SPILLED_P (src_class))
		  && (! reg_class_subset_p (dst_class, src_class)
		      || CLASS_LIKELY_SPILLED_P (dst_class)))
		continue;
	  
	      if (! (src_note = find_reg_note (insn, REG_DEAD, src)))
		continue;

	      /* Can not modify an earlier insn to set dst if this insn
		 uses an old value in the source.  */
	      if (reg_overlap_mentioned_p (dst, SET_SRC (set)))
		continue;

	      if (regmove_dump_file)
		fprintf (regmove_dump_file,
			 "Could fix operand %d of insn %d matching operand %d.\n",
			 operand_number, INSN_UID (insn), match_number);

	      /* If src is set once in a different basic block,
		 and is set equal to a constant, then do not use
		 it for this optimization, as this would make it
		 no longer equivalent to a constant.  */
	      if (reg_is_remote_constant_p (src, insn, f))
		continue;

	      /* Scan backward to find the first instruction that uses
		 the input operand.  If the operand is set here, then
		 replace it in both instructions with match_number.  */

	      for (length = 0, p = PREV_INSN (insn); p; p = PREV_INSN (p))
		{
		  rtx pset;

		  if (GET_CODE (p) == CODE_LABEL
		      || GET_CODE (p) == JUMP_INSN
		      || (GET_CODE (p) == NOTE
			  && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
			      || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
		    break;

		  /* ??? We can't scan past the end of a basic block without
		     updating the register lifetime info
		     (REG_DEAD/basic_block_live_at_start).
		     A CALL_INSN might be the last insn of a basic block, if
		     it is inside an EH region.  There is no easy way to tell,
		     so we just always break when we see a CALL_INSN if
		     flag_exceptions is nonzero.  */
		  if (flag_exceptions && GET_CODE (p) == CALL_INSN)
		    break;

		  if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
		    continue;

		  length++;

		  /* ??? See if all of SRC is set in P.  This test is much
		     more conservative than it needs to be.  */
		  pset = single_set (p);
		  if (pset && SET_DEST (pset) == src)
		    {
		      /* We use validate_replace_rtx, in case there
			 are multiple identical source operands.  All of
			 them have to be changed at the same time.  */
		      if (validate_replace_rtx (src, dst, insn))
			{
			  if (validate_change (p, &SET_DEST (pset),
					       dst, 0))
			    success = 1;
			  else
			    {
			      /* Change all source operands back.
				 This modifies the dst as a side-effect.  */
			      validate_replace_rtx (dst, src, insn);
			      /* Now make sure the dst is right.  */
			      validate_change (insn,
					       recog_operand_loc[match_number],
					       dst, 0);
			    }
			}
		      break;
		    }

		  if (reg_overlap_mentioned_p (src, PATTERN (p))
		      || reg_overlap_mentioned_p (dst, PATTERN (p)))
		    break;

		  /* If we have passed a call instruction, and the
		     pseudo-reg DST is not already live across a call,
		     then don't perform the optimization.  */
		  if (GET_CODE (p) == CALL_INSN)
		    {
		      num_calls++;

		      if (REG_N_CALLS_CROSSED (REGNO (dst)) == 0)
			break;
		    }
		}

	      if (success)
		{
		  int dstno, srcno;

		  /* Remove the death note for SRC from INSN.  */
		  remove_note (insn, src_note);
		  /* Move the death note for SRC to P if it is used
		     there.  */
		  if (reg_overlap_mentioned_p (src, PATTERN (p)))
		    {
		      XEXP (src_note, 1) = REG_NOTES (p);
		      REG_NOTES (p) = src_note;
		    }
		  /* If there is a REG_DEAD note for DST on P, then remove
		     it, because DST is now set there.  */
		  if ((dst_note = find_reg_note (p, REG_DEAD, dst)))
		    remove_note (p, dst_note);

		  dstno = REGNO (dst);
		  srcno = REGNO (src);

		  REG_N_SETS (dstno)++;
		  REG_N_SETS (srcno)--;

		  REG_N_CALLS_CROSSED (dstno) += num_calls;
		  REG_N_CALLS_CROSSED (srcno) -= num_calls;

		  REG_LIVE_LENGTH (dstno) += length;
		  if (REG_LIVE_LENGTH (srcno) >= 0)
		    {
		      REG_LIVE_LENGTH (srcno) -= length;
		      /* REG_LIVE_LENGTH is only an approximation after
			 combine if sched is not run, so make sure that we
			 still have a reasonable value.  */
		      if (REG_LIVE_LENGTH (srcno) < 2)
			REG_LIVE_LENGTH (srcno) = 2;
		    }

		  /* We assume that a register is used exactly once per
		     insn in the updates above.  If this is not correct,
		     no great harm is done.  */

		  REG_N_REFS (dstno) += 2 * loop_depth;
		  REG_N_REFS (srcno) -= 2 * loop_depth;

                  /* If that was the only time src was set,
                     and src was not live at the start of the
                     function, we know that we have no more
                     references to src; clear REG_N_REFS so it
                     won't make reload do any work.  */
                  if (REG_N_SETS (REGNO (src)) == 0
                      && ! regno_uninitialized (REGNO (src)))
                    REG_N_REFS (REGNO (src)) = 0;

		  if (regmove_dump_file)
		    fprintf (regmove_dump_file,
			     "Fixed operand %d of insn %d matching operand %d.\n",
			     operand_number, INSN_UID (insn), match_number);

		  break;
		}
	    }
	}
    }
#endif /* REGISTER_CONSTRAINTS */
}


static int
find_matches (insn, matchp)
     rtx insn;
     struct match *matchp;
{
  int likely_spilled[MAX_RECOG_OPERANDS];
  int operand_number;
  int insn_code_number = recog_memoized (insn);
  int any_matches = 0;

  if (insn_code_number < 0)
    return -1;

  insn_extract (insn);
  if (! constrain_operands (insn_code_number, 0))
    return -1;

  /* Must initialize this before main loop, because the code for
     the commutative case may set matches for operands other than
     the current one.  */
  for (operand_number = insn_n_operands[insn_code_number];
       --operand_number >= 0; )
    matchp->with[operand_number] = matchp->commutative[operand_number] = -1;

  for (operand_number = 0; operand_number < insn_n_operands[insn_code_number];
       operand_number++)
    {
      char *p, c;
      int i = 0;

      p = insn_operand_constraint[insn_code_number][operand_number];

      likely_spilled[operand_number] = 0;
      matchp->use[operand_number] = READ;
      matchp->early_clobber[operand_number] = 0;
      if (*p == '=')
	matchp->use[operand_number] = WRITE;
      else if (*p == '+')
	matchp->use[operand_number] = READWRITE;

      for (;*p && i < which_alternative; p++)
	if (*p == ',')
	  i++;

      while ((c = *p++) != '\0' && c != ',')
	switch (c)
	  {
	  case '=':
	    break;
	  case '+':
	    break;
	  case '&':
	    matchp->early_clobber[operand_number] = 1;
	    break;
	  case '%':
	    matchp->commutative[operand_number] = operand_number + 1;
	    matchp->commutative[operand_number + 1] = operand_number;
	    break;
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9':
	    c -= '0';
	    if (c < operand_number && likely_spilled[c])
	      break;
	    matchp->with[operand_number] = c;
	    any_matches = 1;
	    if (matchp->commutative[operand_number] >= 0)
	      matchp->with[matchp->commutative[operand_number]] = c;
	    break;
	  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'h':
	  case 'j': case 'k': case 'l': case 'p': case 'q': case 't': case 'u':
	  case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B':
	  case 'C': case 'D': case 'W': case 'Y': case 'Z':
	    if (CLASS_LIKELY_SPILLED_P (REG_CLASS_FROM_LETTER (c)))
	      likely_spilled[operand_number] = 1;
	    break;
	  }
    }
  return any_matches ? insn_code_number : -1;
}

/* Try to replace output operand DST in SET, with input operand SRC.  SET is
   the only set in INSN.  INSN has just been recgnized and constrained.
   SRC is operand number OPERAND_NUMBER in INSN.
   DST is operand number MATCH_NUMBER in INSN.
   If BACKWARD is nonzero, we have been called in a backward pass.
   Return nonzero for success.  */
static int
fixup_match_1 (insn, set, src, src_subreg, dst, backward, operand_number,
	       match_number, regmove_dump_file)
     rtx insn, set, src, src_subreg, dst;
     int backward, operand_number, match_number;
     FILE *regmove_dump_file;
{
  rtx p;
  rtx post_inc = 0, post_inc_set = 0, search_end = 0;
  int success = 0;
  int num_calls = 0, s_num_calls = 0;
  enum rtx_code code = NOTE;
  HOST_WIDE_INT insn_const, newconst;
  rtx overlap = 0; /* need to move insn ? */
  rtx src_note = find_reg_note (insn, REG_DEAD, src), dst_note;
  int length, s_length, true_loop_depth;

  if (! src_note)
    {
      /* Look for (set (regX) (op regA constX))
		  (set (regY) (op regA constY))
	 and change that to
		  (set (regA) (op regA constX)).
		  (set (regY) (op regA constY-constX)).
	 This works for add and shift operations, if
	 regA is dead after or set by the second insn.  */

      code = GET_CODE (SET_SRC (set));
      if ((code == PLUS || code == LSHIFTRT
	   || code == ASHIFT || code == ASHIFTRT)
	  && XEXP (SET_SRC (set), 0) == src
	  && GET_CODE (XEXP (SET_SRC (set), 1)) == CONST_INT)
	insn_const = INTVAL (XEXP (SET_SRC (set), 1));
      else if (! stable_but_for_p (SET_SRC (set), src, dst))
	return 0;
      else
	/* We might find a src_note while scanning.  */
	code = NOTE;
    }

  if (regmove_dump_file)
    fprintf (regmove_dump_file,
	     "Could fix operand %d of insn %d matching operand %d.\n",
	     operand_number, INSN_UID (insn), match_number);

  /* If SRC is equivalent to a constant set in a different basic block,
     then do not use it for this optimization.  We want the equivalence
     so that if we have to reload this register, we can reload the
     constant, rather than extending the lifespan of the register.  */
  if (reg_is_remote_constant_p (src, insn, get_insns ()))
    return 0;

  /* Scan forward to find the next instruction that
     uses the output operand.  If the operand dies here,
     then replace it in both instructions with
     operand_number.  */

  for (length = s_length = 0, p = NEXT_INSN (insn); p; p = NEXT_INSN (p))
    {
      if (GET_CODE (p) == CODE_LABEL || GET_CODE (p) == JUMP_INSN
	  || (GET_CODE (p) == NOTE
	      && (NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_BEG
		  || NOTE_LINE_NUMBER (p) == NOTE_INSN_LOOP_END)))
	break;

      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).
	 A CALL_INSN might be the last insn of a basic block, if it is
	 inside an EH region.  There is no easy way to tell, so we just
	 always break when we see a CALL_INSN if flag_exceptions is nonzero.  */
      if (flag_exceptions && GET_CODE (p) == CALL_INSN)
	break;

      if (GET_RTX_CLASS (GET_CODE (p)) != 'i')
	continue;

      length++;
      if (src_note)
	s_length++;

      if (reg_set_p (src, p) || reg_set_p (dst, p)
	  || (GET_CODE (PATTERN (p)) == USE
	      && reg_overlap_mentioned_p (src, XEXP (PATTERN (p), 0))))
	break;

      /* See if all of DST dies in P.  This test is
	 slightly more conservative than it needs to be.  */
      if ((dst_note = find_regno_note (p, REG_DEAD, REGNO (dst)))
	  && (GET_MODE (XEXP (dst_note, 0)) == GET_MODE (dst)))
	{
	  if (! src_note)
	    {
	      rtx q;
	      rtx set2;

	      /* If an optimization is done, the value of SRC while P
		 is executed will be changed.  Check that this is OK.  */
	      if (reg_overlap_mentioned_p (src, PATTERN (p)))
		break;
	      for (q = p; q; q = NEXT_INSN (q))
		{
		  if (GET_CODE (q) == CODE_LABEL || GET_CODE (q) == JUMP_INSN
		      || (GET_CODE (q) == NOTE
			  && (NOTE_LINE_NUMBER (q) == NOTE_INSN_LOOP_BEG
			      || NOTE_LINE_NUMBER (q) == NOTE_INSN_LOOP_END)))
		    {
		      q = 0;
		      break;
		    }

		  /* ??? We can't scan past the end of a basic block without
		     updating the register lifetime info
		     (REG_DEAD/basic_block_live_at_start).
		     A CALL_INSN might be the last insn of a basic block, if
		     it is inside an EH region.  There is no easy way to tell,
		     so we just always break when we see a CALL_INSN if
		     flag_exceptions is nonzero.  */
		  if (flag_exceptions && GET_CODE (p) == CALL_INSN)
		    {
		      q = 0;
		      break;
		    }

		  if (GET_RTX_CLASS (GET_CODE (q)) != 'i')
		    continue;
		  if (reg_overlap_mentioned_p (src, PATTERN (q))
		      || reg_set_p (src, q))
		    break;
		}
	      if (q)
		set2 = single_set (q);
	      if (! q || ! set2 || GET_CODE (SET_SRC (set2)) != code
		  || XEXP (SET_SRC (set2), 0) != src
		  || GET_CODE (XEXP (SET_SRC (set2), 1)) != CONST_INT
		  || (SET_DEST (set2) != src
		      && ! find_reg_note (q, REG_DEAD, src)))
		{
		  /* If this is a PLUS, we can still save a register by doing
		     src += insn_const;
		     P;
		     src -= insn_const; .
		     This also gives opportunities for subsequent
		     optimizations in the backward pass, so do it there.  */
		  if (code == PLUS && backward
#ifdef HAVE_cc0
		      /* We may not emit an insn directly
			 after P if the latter sets CC0.  */
		      && ! sets_cc0_p (PATTERN (p))
#endif
		      )

		    {
		      search_end = q;
		      q = insn;
		      set2 = set;
		      newconst = -insn_const;
		      code = MINUS;
		    }
		  else
		    break;
		}
	      else
		{
		  newconst = INTVAL (XEXP (SET_SRC (set2), 1)) - insn_const;
		  /* Reject out of range shifts.  */
		  if (code != PLUS
		      && (newconst < 0
			  || (newconst
			      >= GET_MODE_BITSIZE (GET_MODE (SET_SRC (set2))))))
		    break;
		  if (code == PLUS)
		    {
		      post_inc = q;
		      if (SET_DEST (set2) != src)
			post_inc_set = set2;
		    }
		}
	      /* We use 1 as last argument to validate_change so that all
		 changes are accepted or rejected together by apply_change_group
		 when it is called by validate_replace_rtx .  */
	      validate_change (q, &XEXP (SET_SRC (set2), 1),
			       GEN_INT (newconst), 1);
	    }
	  validate_change (insn, recog_operand_loc[match_number], src, 1);
	  if (validate_replace_rtx (dst, src_subreg, p))
	    success = 1;
	  break;
	}

      if (reg_overlap_mentioned_p (dst, PATTERN (p)))
	break;
      if (! src_note && reg_overlap_mentioned_p (src, PATTERN (p)))
	{
	  /* INSN was already checked to be movable when
	     we found no REG_DEAD note for src on it.  */
	  overlap = p;
	  src_note = find_reg_note (p, REG_DEAD, src);
	}

      /* If we have passed a call instruction, and the pseudo-reg SRC is not
	 already live across a call, then don't perform the optimization.  */
      if (GET_CODE (p) == CALL_INSN)
	{
	  if (REG_N_CALLS_CROSSED (REGNO (src)) == 0)
	    break;

	  num_calls++;

	  if (src_note)
	    s_num_calls++;

	}
    }

  if (! success)
    return 0;

  true_loop_depth = backward ? 2 - loop_depth : loop_depth;

  /* Remove the death note for DST from P.  */
  remove_note (p, dst_note);
  if (code == MINUS)
    {
      post_inc = emit_insn_after (copy_rtx (PATTERN (insn)), p);
#if defined (HAVE_PRE_INCREMENT) || defined (HAVE_PRE_DECREMENT)
      if (search_end
	  && try_auto_increment (search_end, post_inc, 0, src, newconst, 1))
	post_inc = 0;
#endif
      validate_change (insn, &XEXP (SET_SRC (set), 1), GEN_INT (insn_const), 0);
      REG_N_SETS (REGNO (src))++;
      REG_N_REFS (REGNO (src)) += true_loop_depth;
      REG_LIVE_LENGTH (REGNO (src))++;
    }
  if (overlap)
    {
      /* The lifetime of src and dest overlap,
	 but we can change this by moving insn.  */
      rtx pat = PATTERN (insn);
      if (src_note)
	remove_note (overlap, src_note);
#if defined (HAVE_POST_INCREMENT) || defined (HAVE_POST_DECREMENT)
      if (code == PLUS
	  && try_auto_increment (overlap, insn, 0, src, insn_const, 0))
	insn = overlap;
      else
#endif
	{
	  rtx notes = REG_NOTES (insn);

	  emit_insn_after_with_line_notes (pat, PREV_INSN (p), insn);
	  PUT_CODE (insn, NOTE);
	  NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
	  NOTE_SOURCE_FILE (insn) = 0;
	  /* emit_insn_after_with_line_notes has no
	     return value, so search for the new insn.  */
	  for (insn = p; PATTERN (insn) != pat; )
	    insn = PREV_INSN (insn);

	  REG_NOTES (insn) = notes;
	}
    }
  /* Sometimes we'd generate src = const; src += n;
     if so, replace the instruction that set src
     in the first place.  */

  if (! overlap && (code == PLUS || code == MINUS))
    {
      rtx note = find_reg_note (insn, REG_EQUAL, NULL_RTX);
      rtx q, set2;
      int num_calls2 = 0, s_length2 = 0;

      if (note && CONSTANT_P (XEXP (note, 0)))
	{
	  for (q = PREV_INSN (insn); q; q = PREV_INSN(q))
	    {
	      if (GET_CODE (q) == CODE_LABEL || GET_CODE (q) == JUMP_INSN
		  || (GET_CODE (q) == NOTE
		      && (NOTE_LINE_NUMBER (q) == NOTE_INSN_LOOP_BEG
			  || NOTE_LINE_NUMBER (q) == NOTE_INSN_LOOP_END)))
		{
		  q = 0;
		  break;
		}

	      /* ??? We can't scan past the end of a basic block without
		 updating the register lifetime info
		 (REG_DEAD/basic_block_live_at_start).
		 A CALL_INSN might be the last insn of a basic block, if
		 it is inside an EH region.  There is no easy way to tell,
		 so we just always break when we see a CALL_INSN if
		 flag_exceptions is nonzero.  */
	      if (flag_exceptions && GET_CODE (p) == CALL_INSN)
		{
		  q = 0;
		  break;
		}

	      if (GET_RTX_CLASS (GET_CODE (q)) != 'i')
		continue;
	      s_length2++;
	      if (reg_set_p (src, q))
		{
		  set2 = single_set (q);
		  break;
		}
	      if (reg_overlap_mentioned_p (src, PATTERN (q)))
		{
		  q = 0;
		  break;
		}
	      if (GET_CODE (p) == CALL_INSN)
		num_calls2++;
	    }
	  if (q && set2 && SET_DEST (set2) == src && CONSTANT_P (SET_SRC (set2))
	      && validate_change (insn, &SET_SRC (set), XEXP (note, 0), 0))
	    {
	      PUT_CODE (q, NOTE);
	      NOTE_LINE_NUMBER (q) = NOTE_INSN_DELETED;
	      NOTE_SOURCE_FILE (q) = 0;
	      REG_N_SETS (REGNO (src))--;
	      REG_N_CALLS_CROSSED (REGNO (src)) -= num_calls2;
	      REG_N_REFS (REGNO (src)) -= true_loop_depth;
	      REG_LIVE_LENGTH (REGNO (src)) -= s_length2;
	      insn_const = 0;
	    }
	}
    }

  /* Don't remove this seemingly useless if, it is needed to pair with the
     else in the next two conditionally included code blocks.  */
  if (0)
    {;}
#if defined (HAVE_PRE_INCREMENT) || defined (HAVE_PRE_DECREMENT)
  else if ((code == PLUS || code == MINUS) && insn_const
	   && try_auto_increment (p, insn, 0, src, insn_const, 1))
    insn = p;
#endif
#if defined (HAVE_POST_INCREMENT) || defined (HAVE_POST_DECREMENT)
  else if (post_inc
	   && try_auto_increment (p, post_inc, post_inc_set, src, newconst, 0))
    post_inc = 0;
#endif
#if defined (HAVE_PRE_INCREMENT) || defined (HAVE_PRE_DECREMENT)
  /* If post_inc still prevails, try to find an
     insn where it can be used as a pre-in/decrement.
     If code is MINUS, this was already tried.  */
  if (post_inc && code == PLUS
  /* Check that newconst is likely to be usable
     in a pre-in/decrement before starting the search.  */
      && (0
#if defined (HAVE_PRE_INCREMENT)
	  || (newconst > 0 && newconst <= MOVE_MAX)
#endif
#if defined (HAVE_PRE_DECREMENT)
	  || (newconst < 0 && newconst >= -MOVE_MAX)
#endif
	 ) && exact_log2 (newconst))
    {
      rtx q, inc_dest;

      inc_dest = post_inc_set ? SET_DEST (post_inc_set) : src;
      for (q = post_inc; q = NEXT_INSN (q); )
	{
	  if (GET_CODE (q) == CODE_LABEL || GET_CODE (q) == JUMP_INSN
	      || (GET_CODE (q) == NOTE
		  && (NOTE_LINE_NUMBER (q) == NOTE_INSN_LOOP_BEG
		      || NOTE_LINE_NUMBER (q) == NOTE_INSN_LOOP_END)))
	    break;

	  /* ??? We can't scan past the end of a basic block without updating
	     the register lifetime info (REG_DEAD/basic_block_live_at_start).
	     A CALL_INSN might be the last insn of a basic block, if it
	     is inside an EH region.  There is no easy way to tell so we
	     just always break when we see a CALL_INSN if flag_exceptions
	     is nonzero.  */
	  if (flag_exceptions && GET_CODE (p) == CALL_INSN)
	    break;

	  if (GET_RTX_CLASS (GET_CODE (q)) != 'i')
	    continue;
	  if (src != inc_dest && (reg_overlap_mentioned_p (src, PATTERN (q))
				  || reg_set_p (src, q)))
	    break;
	  if (reg_set_p (inc_dest, q))
	    break;
	  if (reg_overlap_mentioned_p (inc_dest, PATTERN (q)))
	    {
	      try_auto_increment (q, post_inc,
				  post_inc_set, inc_dest, newconst, 1);
	      break;
	    }
	}
    }
#endif /* defined (HAVE_PRE_INCREMENT) || defined (HAVE_PRE_DECREMENT) */
  /* Move the death note for DST to INSN if it is used
     there.  */
  if (reg_overlap_mentioned_p (dst, PATTERN (insn)))
    {
      XEXP (dst_note, 1) = REG_NOTES (insn);
      REG_NOTES (insn) = dst_note;
    }

  if (src_note)
    {
      /* Move the death note for SRC from INSN to P.  */
      if (! overlap)
	remove_note (insn, src_note);
      XEXP (src_note, 1) = REG_NOTES (p);
      REG_NOTES (p) = src_note;

      REG_N_CALLS_CROSSED (REGNO (src)) += s_num_calls;
    }

  REG_N_SETS (REGNO (src))++;
  REG_N_SETS (REGNO (dst))--;

  REG_N_CALLS_CROSSED (REGNO (dst)) -= num_calls;

  REG_LIVE_LENGTH (REGNO (src)) += s_length;
  if (REG_LIVE_LENGTH (REGNO (dst)) >= 0)
    {
      REG_LIVE_LENGTH (REGNO (dst)) -= length;
      /* REG_LIVE_LENGTH is only an approximation after
	 combine if sched is not run, so make sure that we
	 still have a reasonable value.  */
      if (REG_LIVE_LENGTH (REGNO (dst)) < 2)
	REG_LIVE_LENGTH (REGNO (dst)) = 2;
    }

  /* We assume that a register is used exactly once per
      insn in the updates above.  If this is not correct,
      no great harm is done.  */

  REG_N_REFS (REGNO (src)) += 2 * true_loop_depth;
  REG_N_REFS (REGNO (dst)) -= 2 * true_loop_depth;

  /* If that was the only time dst was set,
     and dst was not live at the start of the
     function, we know that we have no more
     references to dst; clear REG_N_REFS so it
     won't make reload do any work.  */
  if (REG_N_SETS (REGNO (dst)) == 0
      && ! regno_uninitialized (REGNO (dst)))
    REG_N_REFS (REGNO (dst)) = 0;

  if (regmove_dump_file)
    fprintf (regmove_dump_file,
	     "Fixed operand %d of insn %d matching operand %d.\n",
	     operand_number, INSN_UID (insn), match_number);
  return 1;
}


/* return nonzero if X is stable but for mentioning SRC or mentioning /
   changing DST .  If in doubt, presume it is unstable.  */
static int
stable_but_for_p (x, src, dst)
     rtx x, src, dst;
{
  RTX_CODE code = GET_CODE (x);
  switch (GET_RTX_CLASS (code))
    {
    case '<': case '1': case 'c': case '2': case 'b': case '3':
      {
	int i;
	char *fmt = GET_RTX_FORMAT (code);
	for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
	  if (fmt[i] == 'e' && ! stable_but_for_p (XEXP (x, i), src, dst))
	      return 0;
	return 1;
      }
    case 'o':
      if (x == src || x == dst)
	return 1;
      /* fall through */
    default:
      return ! rtx_unstable_p (x);
    }
}

/* Test if regmove seems profitable for this target.  */
int
regmove_profitable_p ()
{
#ifdef REGISTER_CONSTRAINTS
  struct match match;
  enum machine_mode mode;
  optab tstoptab = add_optab;
  do /* check add_optab and ashl_optab */
    for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT); mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	{
	  int icode = (int) tstoptab->handlers[(int) mode].insn_code;
	  rtx reg0, reg1, reg2, pat;
	  int i;
    
	  if (GET_MODE_BITSIZE (mode) < 32 || icode == CODE_FOR_nothing)
	    continue;
	  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
	    if (TEST_HARD_REG_BIT (reg_class_contents[GENERAL_REGS], i))
	      break;
	  if (i + 2 >= FIRST_PSEUDO_REGISTER)
	    break;
	  reg0 = gen_rtx_REG (insn_operand_mode[icode][0], i);
	  reg1 = gen_rtx_REG (insn_operand_mode[icode][1], i + 1);
	  reg2 = gen_rtx_REG (insn_operand_mode[icode][2], i + 2);
	  if (! (*insn_operand_predicate[icode][0]) (reg0, VOIDmode)
	      || ! (*insn_operand_predicate[icode][1]) (reg1, VOIDmode)
	      || ! (*insn_operand_predicate[icode][2]) (reg2, VOIDmode))
	    break;
	  pat = GEN_FCN (icode) (reg0, reg1, reg2);
	  if (! pat)
	    continue;
	  if (GET_CODE (pat) == SEQUENCE)
	    pat = XVECEXP (pat, 0,  XVECLEN (pat, 0) - 1);
	  else
	    pat = make_insn_raw (pat);
	  if (! single_set (pat)
	      || GET_CODE (SET_SRC (single_set (pat))) != tstoptab->code)
	    /* Unexpected complexity;  don't need to handle this unless
	       we find a machine where this occurs and regmove should
	       be enabled.  */
	    break;
	  if (find_matches (pat, &match) >= 0)
	    return 1;
	  break;
	}
  while (tstoptab != ashl_optab && (tstoptab = ashl_optab, 1));
#endif /* REGISTER_CONSTRAINTS */
  return 0;
}

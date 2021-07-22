/* Simulator for the MIPS architecture.

   This file is part of the MIPS sim

		THIS SOFTWARE IS NOT COPYRIGHTED

   Cygnus offers the following for use in the public domain.  Cygnus
   makes no warranty with regard to the software or it's performance
   and the user accepts the software "AS IS" with all faults.

   CYGNUS DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
   THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

   $Revision: 1.4.2.1 $
   $Date: 1998/06/16 14:50:26 $             

   */

#ifndef SIM_MAIN_C
#define SIM_MAIN_C

#include "sim-main.h"

#if !(WITH_IGEN)
#define SIM_MANIFESTS
#include "oengine.c"
#undef SIM_MANIFESTS
#endif


/*---------------------------------------------------------------------------*/
/*-- simulator engine -------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/* Description from page A-22 of the "MIPS IV Instruction Set" manual
   (revision 3.1) */
/* Translate a virtual address to a physical address and cache
   coherence algorithm describing the mechanism used to resolve the
   memory reference. Given the virtual address vAddr, and whether the
   reference is to Instructions ot Data (IorD), find the corresponding
   physical address (pAddr) and the cache coherence algorithm (CCA)
   used to resolve the reference. If the virtual address is in one of
   the unmapped address spaces the physical address and the CCA are
   determined directly by the virtual address. If the virtual address
   is in one of the mapped address spaces then the TLB is used to
   determine the physical address and access type; if the required
   translation is not present in the TLB or the desired access is not
   permitted the function fails and an exception is taken.

   NOTE: Normally (RAW == 0), when address translation fails, this
   function raises an exception and does not return. */

INLINE_SIM_MAIN (int)
address_translation (SIM_DESC sd,
		     sim_cpu *cpu,
		     address_word cia,
		     address_word vAddr,
		     int IorD,
		     int LorS,
		     address_word *pAddr,
		     int *CCA,
		     int raw)
{
  int res = -1; /* TRUE : Assume good return */

#ifdef DEBUG
  sim_io_printf(sd,"AddressTranslation(0x%s,%s,%s,...);\n",pr_addr(vAddr),(IorD ? "isDATA" : "isINSTRUCTION"),(LorS ? "iSTORE" : "isLOAD"));
#endif

  /* Check that the address is valid for this memory model */

  /* For a simple (flat) memory model, we simply pass virtual
     addressess through (mostly) unchanged. */
  vAddr &= 0xFFFFFFFF;


  *pAddr = vAddr; /* default for isTARGET */
  *CCA = Uncached; /* not used for isHOST */

  return(res);
}

/* Description from page A-23 of the "MIPS IV Instruction Set" manual
   (revision 3.1) */
/* Prefetch data from memory. Prefetch is an advisory instruction for
   which an implementation specific action is taken. The action taken
   may increase performance, but must not change the meaning of the
   program, or alter architecturally-visible state. */

INLINE_SIM_MAIN (void)
prefetch (SIM_DESC sd,
	  sim_cpu *cpu,
	  address_word cia,
	  int CCA,
	  address_word pAddr,
	  address_word vAddr,
	  int DATA,
	  int hint)
{
#ifdef DEBUG
  sim_io_printf(sd,"Prefetch(%d,0x%s,0x%s,%d,%d);\n",CCA,pr_addr(pAddr),pr_addr(vAddr),DATA,hint);
#endif /* DEBUG */

  /* For our simple memory model we do nothing */
  return;
}

/* Description from page A-22 of the "MIPS IV Instruction Set" manual
   (revision 3.1) */
/* Load a value from memory. Use the cache and main memory as
   specified in the Cache Coherence Algorithm (CCA) and the sort of
   access (IorD) to find the contents of AccessLength memory bytes
   starting at physical location pAddr. The data is returned in the
   fixed width naturally-aligned memory element (MemElem). The
   low-order two (or three) bits of the address and the AccessLength
   indicate which of the bytes within MemElem needs to be given to the
   processor. If the memory access type of the reference is uncached
   then only the referenced bytes are read from memory and valid
   within the memory element. If the access type is cached, and the
   data is not present in cache, an implementation specific size and
   alignment block of memory is read and loaded into the cache to
   satisfy a load reference. At a minimum, the block is the entire
   memory element. */
INLINE_SIM_MAIN (void)
load_memory (SIM_DESC SD,
	     sim_cpu *CPU,
	     address_word cia,
	     uword64* memvalp,
	     uword64* memval1p,
	     int CCA,
	     unsigned int AccessLength,
	     address_word pAddr,
	     address_word vAddr,
	     int IorD)
{
  uword64 value = 0;
  uword64 value1 = 0;

#ifdef DEBUG
  sim_io_printf(sd,"DBG: LoadMemory(%p,%p,%d,%d,0x%s,0x%s,%s)\n",memvalp,memval1p,CCA,AccessLength,pr_addr(pAddr),pr_addr(vAddr),(IorD ? "isDATA" : "isINSTRUCTION"));
#endif /* DEBUG */

#if defined(WARN_MEM)
  if (CCA != uncached)
    sim_io_eprintf(sd,"LoadMemory CCA (%d) is not uncached (currently all accesses treated as cached)\n",CCA);
#endif /* WARN_MEM */

#if !(WITH_IGEN)
  /* IGEN performs this test in ifetch16() / ifetch32() */
  /* If instruction fetch then we need to check that the two lo-order
     bits are zero, otherwise raise a InstructionFetch exception: */
  if ((IorD == isINSTRUCTION)
      && ((pAddr & 0x3) != 0)
      && (((pAddr & 0x1) != 0) || ((vAddr & 0x1) == 0)))
    SignalExceptionInstructionFetch ();
#endif

  if (((pAddr & LOADDRMASK) + AccessLength) > LOADDRMASK)
    {
      /* In reality this should be a Bus Error */
      sim_io_error (SD, "LOAD AccessLength of %d would extend over %d bit aligned boundary for physical address 0x%s\n",
		    AccessLength,
		    (LOADDRMASK + 1) << 3,
		    pr_addr (pAddr));
    }

#if defined(TRACE)
  dotrace (SD, CPU, tracefh,((IorD == isDATA) ? 0 : 2),(unsigned int)(pAddr&0xFFFFFFFF),(AccessLength + 1),"load%s",((IorD == isDATA) ? "" : " instruction"));
#endif /* TRACE */
  
  /* Read the specified number of bytes from memory.  Adjust for
     host/target byte ordering/ Align the least significant byte
     read. */

  switch (AccessLength)
    {
    case AccessLength_QUADWORD :
      {
	unsigned_16 val = sim_core_read_aligned_16 (CPU, NULL_CIA, read_map, pAddr);
	value1 = VH8_16 (val);
	value = VL8_16 (val);
	break;
      }
    case AccessLength_DOUBLEWORD :
      value = sim_core_read_aligned_8 (CPU, NULL_CIA,
				       read_map, pAddr);
      break;
    case AccessLength_SEPTIBYTE :
      value = sim_core_read_misaligned_7 (CPU, NULL_CIA,
					  read_map, pAddr);
      break;
    case AccessLength_SEXTIBYTE :
      value = sim_core_read_misaligned_6 (CPU, NULL_CIA,
					  read_map, pAddr);
      break;
    case AccessLength_QUINTIBYTE :
      value = sim_core_read_misaligned_5 (CPU, NULL_CIA,
					  read_map, pAddr);
      break;
    case AccessLength_WORD :
      value = sim_core_read_aligned_4 (CPU, NULL_CIA,
				       read_map, pAddr);
      break;
    case AccessLength_TRIPLEBYTE :
      value = sim_core_read_misaligned_3 (CPU, NULL_CIA,
					  read_map, pAddr);
      break;
    case AccessLength_HALFWORD :
      value = sim_core_read_aligned_2 (CPU, NULL_CIA,
				       read_map, pAddr);
      break;
    case AccessLength_BYTE :
      value = sim_core_read_aligned_1 (CPU, NULL_CIA,
				       read_map, pAddr);
      break;
    default:
      abort ();
    }
  
#ifdef DEBUG
  printf("DBG: LoadMemory() : (offset %d) : value = 0x%s%s\n",
	 (int)(pAddr & LOADDRMASK),pr_uword64(value1),pr_uword64(value));
#endif /* DEBUG */
  
  /* See also store_memory. Position data in correct byte lanes. */
  if (AccessLength <= LOADDRMASK)
    {
      if (BigEndianMem)
	/* for big endian target, byte (pAddr&LOADDRMASK == 0) is
	   shifted to the most significant byte position.  */
	value <<= (((LOADDRMASK - (pAddr & LOADDRMASK)) - AccessLength) * 8);
      else
	/* For little endian target, byte (pAddr&LOADDRMASK == 0)
	   is already in the correct postition. */
	value <<= ((pAddr & LOADDRMASK) * 8);
    }
  
#ifdef DEBUG
  printf("DBG: LoadMemory() : shifted value = 0x%s%s\n",
	 pr_uword64(value1),pr_uword64(value));
#endif /* DEBUG */
  
  *memvalp = value;
  if (memval1p) *memval1p = value1;
}


/* Description from page A-23 of the "MIPS IV Instruction Set" manual
   (revision 3.1) */
/* Store a value to memory. The specified data is stored into the
   physical location pAddr using the memory hierarchy (data caches and
   main memory) as specified by the Cache Coherence Algorithm
   (CCA). The MemElem contains the data for an aligned, fixed-width
   memory element (word for 32-bit processors, doubleword for 64-bit
   processors), though only the bytes that will actually be stored to
   memory need to be valid. The low-order two (or three) bits of pAddr
   and the AccessLength field indicates which of the bytes within the
   MemElem data should actually be stored; only these bytes in memory
   will be changed. */

INLINE_SIM_MAIN (void)
store_memory (SIM_DESC SD,
	      sim_cpu *CPU,
	      address_word cia,
	      int CCA,
	      unsigned int AccessLength,
	      uword64 MemElem,
	      uword64 MemElem1,   /* High order 64 bits */
	      address_word pAddr,
	      address_word vAddr)
{
#ifdef DEBUG
  sim_io_printf(sd,"DBG: StoreMemory(%d,%d,0x%s,0x%s,0x%s,0x%s)\n",CCA,AccessLength,pr_uword64(MemElem),pr_uword64(MemElem1),pr_addr(pAddr),pr_addr(vAddr));
#endif /* DEBUG */
  
#if defined(WARN_MEM)
  if (CCA != uncached)
    sim_io_eprintf(sd,"StoreMemory CCA (%d) is not uncached (currently all accesses treated as cached)\n",CCA);
#endif /* WARN_MEM */
  
  if (((pAddr & LOADDRMASK) + AccessLength) > LOADDRMASK)
    sim_io_error (SD, "STORE AccessLength of %d would extend over %d bit aligned boundary for physical address 0x%s\n",
		  AccessLength,
		  (LOADDRMASK + 1) << 3,
		  pr_addr(pAddr));
  
#if defined(TRACE)
  dotrace (SD, CPU, tracefh,1,(unsigned int)(pAddr&0xFFFFFFFF),(AccessLength + 1),"store");
#endif /* TRACE */
  
#ifdef DEBUG
  printf("DBG: StoreMemory: offset = %d MemElem = 0x%s%s\n",(unsigned int)(pAddr & LOADDRMASK),pr_uword64(MemElem1),pr_uword64(MemElem));
#endif /* DEBUG */
  
  /* See also load_memory. Position data in correct byte lanes. */
  if (AccessLength <= LOADDRMASK)
    {
      if (BigEndianMem)
	/* for big endian target, byte (pAddr&LOADDRMASK == 0) is
	   shifted to the most significant byte position.  */
	MemElem >>= (((LOADDRMASK - (pAddr & LOADDRMASK)) - AccessLength) * 8);
      else
	/* For little endian target, byte (pAddr&LOADDRMASK == 0)
	   is already in the correct postition. */
	MemElem >>= ((pAddr & LOADDRMASK) * 8);
    }
  
#ifdef DEBUG
  printf("DBG: StoreMemory: shift = %d MemElem = 0x%s%s\n",shift,pr_uword64(MemElem1),pr_uword64(MemElem));
#endif /* DEBUG */
  
  switch (AccessLength)
    {
    case AccessLength_QUADWORD :
      {
	unsigned_16 val = U16_8 (MemElem1, MemElem);
	sim_core_write_aligned_16 (CPU, NULL_CIA, write_map, pAddr, val);
	break;
      }
    case AccessLength_DOUBLEWORD :
      sim_core_write_aligned_8 (CPU, NULL_CIA,
				write_map, pAddr, MemElem);
      break;
    case AccessLength_SEPTIBYTE :
      sim_core_write_misaligned_7 (CPU, NULL_CIA,
				   write_map, pAddr, MemElem);
      break;
    case AccessLength_SEXTIBYTE :
      sim_core_write_misaligned_6 (CPU, NULL_CIA,
				   write_map, pAddr, MemElem);
      break;
    case AccessLength_QUINTIBYTE :
      sim_core_write_misaligned_5 (CPU, NULL_CIA,
				   write_map, pAddr, MemElem);
      break;
    case AccessLength_WORD :
      sim_core_write_aligned_4 (CPU, NULL_CIA,
				write_map, pAddr, MemElem);
      break;
    case AccessLength_TRIPLEBYTE :
      sim_core_write_misaligned_3 (CPU, NULL_CIA,
				   write_map, pAddr, MemElem);
      break;
    case AccessLength_HALFWORD :
      sim_core_write_aligned_2 (CPU, NULL_CIA,
				write_map, pAddr, MemElem);
      break;
    case AccessLength_BYTE :
      sim_core_write_aligned_1 (CPU, NULL_CIA,
				write_map, pAddr, MemElem);
      break;
    default:
      abort ();
    }	
  
  return;
}


INLINE_SIM_MAIN (unsigned32)
ifetch32 (SIM_DESC SD,
	  sim_cpu *CPU,
	  address_word cia,
	  address_word vaddr)
{
  /* Copy the action of the LW instruction */
  address_word mask = LOADDRMASK;
  address_word access = AccessLength_WORD;
  address_word reverseendian = (ReverseEndian ? (mask ^ access) : 0);
  address_word bigendiancpu = (BigEndianCPU ? (mask ^ access) : 0);
  unsigned int byte;
  address_word paddr;
  int uncached;
  unsigned64 memval;

  if ((vaddr & access) != 0)
    SignalExceptionInstructionFetch ();
  AddressTranslation (vaddr, isINSTRUCTION, isLOAD, &paddr, &uncached, isTARGET, isREAL);
  paddr = ((paddr & ~mask) | ((paddr & mask) ^ reverseendian));
  LoadMemory (&memval, NULL, uncached, access, paddr, vaddr, isINSTRUCTION, isREAL);
  byte = ((vaddr & mask) ^ bigendiancpu);
  return (memval >> (8 * byte));
}


INLINE_SIM_MAIN (unsigned16)
ifetch16 (SIM_DESC SD,
	  sim_cpu *CPU,
	  address_word cia,
	  address_word vaddr)
{
  /* Copy the action of the LH instruction */
  address_word mask = LOADDRMASK;
  address_word access = AccessLength_HALFWORD;
  address_word reverseendian = (ReverseEndian ? (mask ^ access) : 0);
  address_word bigendiancpu = (BigEndianCPU ? (mask ^ access) : 0);
  unsigned int byte;
  address_word paddr;
  int uncached;
  unsigned64 memval;

  if ((vaddr & access) != 0)
    SignalExceptionInstructionFetch ();
  AddressTranslation (vaddr, isINSTRUCTION, isLOAD, &paddr, &uncached, isTARGET, isREAL);
  paddr = ((paddr & ~mask) | ((paddr & mask) ^ reverseendian));
  LoadMemory (&memval, NULL, uncached, access, paddr, vaddr, isINSTRUCTION, isREAL);
  byte = ((vaddr & mask) ^ bigendiancpu);
  return (memval >> (8 * byte));
}



/* Description from page A-26 of the "MIPS IV Instruction Set" manual (revision 3.1) */
/* Order loads and stores to synchronise shared memory. Perform the
   action necessary to make the effects of groups of synchronizable
   loads and stores indicated by stype occur in the same order for all
   processors. */
INLINE_SIM_MAIN (void)
sync_operation (SIM_DESC sd,
		sim_cpu *cpu,
		address_word cia,
		int stype)
{
#ifdef DEBUG
  sim_io_printf(sd,"SyncOperation(%d) : TODO\n",stype);
#endif /* DEBUG */
  return;
}

INLINE_SIM_MAIN (void)
cache_op (SIM_DESC SD,
	  sim_cpu *CPU,
	  address_word cia,
	  int op,
	  address_word pAddr,
	  address_word vAddr,
	  unsigned int instruction)
{
#if 1 /* stop warning message being displayed (we should really just remove the code) */
  static int icache_warning = 1;
  static int dcache_warning = 1;
#else
  static int icache_warning = 0;
  static int dcache_warning = 0;
#endif

  /* If CP0 is not useable (User or Supervisor mode) and the CP0
     enable bit in the Status Register is clear - a coprocessor
     unusable exception is taken. */
#if 0
  sim_io_printf(SD,"TODO: Cache availability checking (PC = 0x%s)\n",pr_addr(cia));
#endif

  switch (op & 0x3) {
    case 0: /* instruction cache */
      switch (op >> 2) {
        case 0: /* Index Invalidate */
        case 1: /* Index Load Tag */
        case 2: /* Index Store Tag */
        case 4: /* Hit Invalidate */
        case 5: /* Fill */
        case 6: /* Hit Writeback */
          if (!icache_warning)
            {
              sim_io_eprintf(SD,"Instruction CACHE operation %d to be coded\n",(op >> 2));
              icache_warning = 1;
            }
          break;

        default:
          SignalException(ReservedInstruction,instruction);
          break;
      }
      break;

    case 1: /* data cache */
      switch (op >> 2) {
        case 0: /* Index Writeback Invalidate */
        case 1: /* Index Load Tag */
        case 2: /* Index Store Tag */
        case 3: /* Create Dirty */
        case 4: /* Hit Invalidate */
        case 5: /* Hit Writeback Invalidate */
        case 6: /* Hit Writeback */ 
          if (!dcache_warning)
            {
              sim_io_eprintf(SD,"Data CACHE operation %d to be coded\n",(op >> 2));
              dcache_warning = 1;
            }
          break;

        default:
          SignalException(ReservedInstruction,instruction);
          break;
      }
      break;

    default: /* unrecognised cache ID */
      SignalException(ReservedInstruction,instruction);
      break;
  }

  return;
}


INLINE_SIM_MAIN (void)
pending_tick (SIM_DESC SD,
	      sim_cpu *CPU,
	      address_word cia)
{
  if (PENDING_TRACE)							
    sim_io_printf (SD, "PENDING_DRAIN - pending_in = %d, pending_out = %d, pending_total = %d\n", PENDING_IN, PENDING_OUT, PENDING_TOTAL); 
  if (PENDING_OUT != PENDING_IN)					
    {									
      int loop;							
      int index = PENDING_OUT;					
      int total = PENDING_TOTAL;					
      if (PENDING_TOTAL == 0)						
	sim_engine_abort (SD, CPU, cia, "PENDING_DRAIN - Mis-match on pending update pointers\n"); 
      for (loop = 0; (loop < total); loop++)				
	{								
	  if (PENDING_SLOT_DEST[index] != NULL)			
	    {								
	      PENDING_SLOT_DELAY[index] -= 1;				
	      if (PENDING_SLOT_DELAY[index] == 0)			
		{							
		  if (PENDING_SLOT_BIT[index] >= 0)			
		    switch (PENDING_SLOT_SIZE[index])                 
		      {						
		      case 32:					
			if (PENDING_SLOT_VALUE[index])		
			  *(unsigned32*)PENDING_SLOT_DEST[index] |= 	
			    BIT32 (PENDING_SLOT_BIT[index]);		
			else						
			  *(unsigned32*)PENDING_SLOT_DEST[index] &= 	
			    BIT32 (PENDING_SLOT_BIT[index]);		
			break;					
		      case 64:					
			if (PENDING_SLOT_VALUE[index])		
			  *(unsigned64*)PENDING_SLOT_DEST[index] |= 	
			    BIT64 (PENDING_SLOT_BIT[index]);		
			else						
			  *(unsigned64*)PENDING_SLOT_DEST[index] &= 	
			    BIT64 (PENDING_SLOT_BIT[index]);		
			break;					
			break;					
		      }
		  else
		    switch (PENDING_SLOT_SIZE[index])                 
		      {						
		      case 32:					
			*(unsigned32*)PENDING_SLOT_DEST[index] = 	
			  PENDING_SLOT_VALUE[index];			
			break;					
		      case 64:					
			*(unsigned64*)PENDING_SLOT_DEST[index] = 	
			  PENDING_SLOT_VALUE[index];			
			break;					
		      }							
		}							
	      if (PENDING_OUT == index)				
		{							
		  PENDING_SLOT_DEST[index] = NULL;			
		  PENDING_OUT = (PENDING_OUT + 1) % PSLOTS;		
		  PENDING_TOTAL--;					
		}							
	    }								
	}								
      index = (index + 1) % PSLOTS;					
    }									
}


#endif

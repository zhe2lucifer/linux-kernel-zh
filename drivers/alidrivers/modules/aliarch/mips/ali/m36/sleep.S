/**
* Copyright (c) 2011,Ali Corp.
* All rights reserved.
*
* FileName     : sleep.S
* Verison      : 1.0
* Author       : Tony Zhang
* Date         : 2015-06-30
* Description  : Jump to STR Part II inside pmu ram and resume entry
*/

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/cacheops.h>
#include "mips.h"
/**********************************************************************************************************************************************/

#define STR_CPU_REG_BACKUP                                                 (0xB8054800)/*reserved 128 byte to store MIPS cpu general register.*/
#define STR_CPU_REG_BACKUP_2                                            (0xB8054600)
#define RETENTION_PMU_RAM_START                                 (0xB8055000)
#define SR_IEC                                                                                  (0x00000001)/*Interrupt Enable, current.*/
#define DCACHE_WRBACK                                                             (0x19)
#define CACHE_INV                                                                         (0x10)
#define CFG_IB_MASK                                                                   (0x7<<19)
#define CFG_IC_MASK                                                                    (0x7<<22)
#define CFG_DC_MASK                                                                  (0x7<<13)
#define CFG_DB_MASK                                                                  (0x7<<10)

#define MMFC0(rt,rd,sel) \
	.word 0x40000000 | (rt<<16) | (rd<<11) | sel
#define MMTC0(rt,rd,sel) \
	.word 0x40800000 | (rt<<16) | (rd<<11) | sel
/**********************************************************************************************************************************************/

/*
*fucntion name: ali_str_finish_suspend(void).
*used register: all.
*description: backup cpu running status before enter standby mode.
*/

LEAF(ali_str_finish_suspend)
	.set    noat
	/*Backup general registers.*/
	li        k0, STR_CPU_REG_BACKUP
	sw      t0, 1*4(k0)
	sw      t1, 2*4(k0)
	sw      t2, 3*4(k0)
	sw      t3, 4*4(k0)
	sw      t4, 5*4(k0)
	sw      t5, 6*4(k0)
	sw      t6, 7*4(k0)
	sw      t7, 8*4(k0)
	sw      t8, 9*4(k0)
	sw      t9, 10*4(k0)
	sw      a0, 11*4(k0)
	sw      a1, 12*4(k0)
	sw      a2, 13*4(k0)
	sw      a3, 14*4(k0)
	sw      s0, 15*4(k0)
	sw      s1, 16*4(k0)
	sw      s2, 17*4(k0)
	sw      s3, 18*4(k0)
	sw      s4, 19*4(k0)
	sw      s5, 20*4(k0)
	sw      s6, 21*4(k0)
	sw      s7, 22*4(k0)
	sw      v0, 23*4(k0)
	sw      v1, 24*4(k0)
	sw      AT, 25*4(k0)
	sw      ra, 26*4(k0)
	sw      gp, 27*4(k0)
	sw      fp, 28*4(k0)
	sw      sp, 29*4(k0)

	/*You can get help form doc----UnCode-See_MIPS_Run-2nd_edition-Chinese-All-20071231.pdf
	Page48~49.*/
	MMFC0(27, 12, 0)                 /*SR.*/
	ehb
	sw      k1, 30*4(k0)

	MMFC0(27, 13, 0)                 /*Cause.*/
	ehb
	sw      k1, 31*4(k0)

	MMFC0(27, 14, 0)                 /*EPC.*/
	ehb
	sw      k1, 32*4(k0)

	MMFC0(27, 9, 0)                  /*Count.*/
	ehb
	sw      k1, 33*4(k0)

	MMFC0(27, 11, 0)                 /*Compare.*/
	ehb
	sw      k1, 34*4(k0)

	MMFC0(27, 8, 0)                  /*BadVaddr.*/
	ehb
	sw      k1, 35*4(k0)

	MMFC0(27, 4, 0)                  /*Context.*/
	ehb
	sw      k1, 36*4(k0)

	MMFC0(27, 10, 0)                 /*EntryHi.*/
	ehb
	sw      k1, 37*4(k0)

	MMFC0(27, 2, 0)                  /*EntryLo0.*/
	ehb
	sw      k1, 38*4(k0)

	MMFC0(27, 3, 0)                  /*EntryLo1.*/
	ehb
	sw      k1, 39*4(k0)

	MMFC0(27, 0, 0)                  /*Index.*/
	ehb
	sw      k1, 40*4(k0)

	MMFC0(27, 5, 0)                  /*PageMask.*/
	ehb
	sw      k1, 41*4(k0)

	MMFC0(27, 1, 0)                  /*Random.*/
	ehb
	sw      k1, 42*4(k0)

	MMFC0(27, 6, 0)                  /*Wired.*/
	ehb
	sw      k1, 43*4(k0)

	MMFC0(27, 15, 0)                 /*PRId.*/
	ehb
	sw      k1, 44*4(k0)

	MMFC0(27, 16, 0)                 /*Config.*/
	ehb
	sw      k1, 45*4(k0)

	MMFC0(27, 16, 1)                 /*Config1.*/
	ehb
	sw      k1, 46*4(k0)

	MMFC0(27, 16, 2)                 /*Config2.*/
	ehb
	sw      k1, 47*4(k0)

	MMFC0(27, 16, 3)                 /*Config3.*/
	ehb
	sw      k1, 48*4(k0)

	MMFC0(27, 15, 1)                 /*Ebase.*/
	ehb
	sw      k1, 49*4(k0)

	MMFC0(27, 12, 1)                 /*IntCtl.*/
	ehb
	sw      k1, 50*4(k0)

	MMFC0(27, 12, 2)                 /*SRSCtl.*/
	ehb
	sw      k1, 51*4(k0)

	MMFC0(27, 12, 3)                 /*SRSMap.*/
	ehb
	sw      k1, 52*4(k0)

	MMFC0(27, 27, 0)                 /*CacheERR.*/
	ehb
	sw      k1, 53*4(k0)

	MMFC0(27, 26, 0)                 /*ECC.*/
	ehb
	sw      k1, 54*4(k0)

	MMFC0(27, 30, 0)                 /*ErrorEPC.*/
	ehb
	sw      k1, 55*4(k0)

	MMFC0(27, 28, 0)                 /*TagLo.*/
	ehb
	sw      k1, 56*4(k0)

	MMFC0(27, 28, 1)                 /*DataLo.*/
	ehb
	sw      k1, 57*4(k0)

	MMFC0(27, 29, 0)                 /*TagHi.*/
	ehb
	sw      k1, 58*4(k0)

	MMFC0(27, 29, 1)                 /*DataHi.*/
	ehb
	sw      k1, 59*4(k0)

	MMFC0(27, 23, 0)                 /*Debug.*/
	ehb
	sw      k1, 60*4(k0)

	MMFC0(27, 24, 0)                 /*DEPC.*/
	ehb
	sw      k1, 61*4(k0)

	MMFC0(27, 31, 0)                 /*DESAVE.*/
	ehb
	sw      k1, 62*4(k0)

	MMFC0(27, 18, 0)                 /*WatchLo.*/
	ehb
	sw      k1, 63*4(k0)

	MMFC0(27, 19, 0)                 /*WatchHi.*/
	ehb
	sw      k1, 64*4(k0)

	MMFC0(27, 25, 0)                 /*PerfCtl.*/
	ehb
	sw      k1, 65*4(k0)

	MMFC0(27, 25, 1)                 /*PerfCnt.*/
	ehb
	sw      k1, 66*4(k0)

	MMFC0(27, 17, 0)                 /*LLAddr.*/
	ehb
	sw      k1, 67*4(k0)

	MMFC0(27, 7, 0)                  /*HWREna.*/
	ehb
	sw      k1, 68*4(k0)

	jal             ali_cache_flush_all
	nop

	li              t0, RETENTION_PMU_RAM_START
	jr               t0
	nop
	nop
	nop
	nop
	.set at
END(ali_str_finish_suspend)


LEAF(ali_str_resume)
	.set noat
#if 1/*Reserved for debug, don't delete it.*/
	jal     pm_uart_debug    /*Output char '\n'*/
	li      a0, '\n'
	jal     pm_uart_debug    /*Output char '\r'*/
	li      a0, '\r'

	/*Output "BACK", it means that STB recovered from standby.*/
	jal     pm_uart_debug    /*Output char 'B'*/
	li      a0, 'B'

	jal     pm_uart_debug    /*Output char 'A'*/
	li      a0, 'A'

	jal     pm_uart_debug    /*Output char 'C'*/
	li      a0, 'C'

	jal     pm_uart_debug    /*Output char 'K'*/
	li      a0, 'K'

	jal     pm_uart_debug    /*Output char '\n'*/
	li      a0, '\n'
	jal     pm_uart_debug    /*Output char '\r'*/
	li      a0, '\r'
#endif

	/*Init cache code must run in DDR buffer.*/
	la              t1, _NON_CACHE_start
	lui             t2, 0xA000
	ori            t2, 0x0000
	or            t1, t2
	jr              t1
	nop

_NON_CACHE_start:
	/*CPU init.*/
	addiu $1,  $0, 0x0
	addiu $2,  $1, 0x0   
	addiu $3,  $2, 0x0   
	addiu $4,  $3, 0x0   
	addiu $5,  $4, 0x0   
	addiu $6,  $5, 0x0   
	addiu $7,  $6, 0x0   
	addiu $8,  $7, 0x0   
	addiu $9,  $8, 0x0   
	addiu $10, $9, 0x0  
	addiu $11, $10, 0x0 
	addiu $12,  $11, 0x0 
	addiu $13,  $12, 0x0 
	addiu $14,  $13, 0x0 
	addiu $15,  $14, 0x0 
	addiu $16,  $15, 0x0 
	addiu $17,  $16, 0x0 
	addiu $18,  $17, 0x0 
	addiu $19,  $18, 0x0 
	addiu $20,  $19, 0x0 
	addiu $21,  $20, 0x0 
	addiu $22,  $21, 0x0 
	addiu $23,  $22, 0x0 
	addiu $24,  $23, 0x0 
	addiu $25,  $24, 0x0 
	addiu $26,  $25, 0x0 
	addiu $27,  $26, 0x0 
	addiu $28,  $27, 0x0 
	addiu $29,  $28, 0x0 
	addiu $30,  $29, 0x0 
	addiu $31,  $30, 0x0
	mfc0 t0, C0_CONFIG
        ehb
        li t1, ~0x7
        and t0, t1
        ori t0, t0, 3
        mtc0 t0, C0_CONFIG/*cacheable, write-back.*/
        ehb

        MMFC0(8,16,7)      
        ori     t0, 0x100/*set config7 ES[8] bit.*/
        MMTC0(8,16,7)                    

#ifdef CONFIG_MIPS_CPU_SCACHE
	/*L2 Cache enable: Clear config2 bit 12.*/
	MMFC0(8,16,2)/*0x40088002, mfc0 t0, Config2.*/
	ehb
	li              t1, 0xFFFFEFFF/*Clear Config2 bit 12.*/
	and           t0, t1
	ehb
	MMTC0(8,16,2)/*0x40888002, mtc0 t0, Config2.*/
	ehb

	/* L2 Cache initialization */
	MMFC0(10, 16, 2)/*C0_Config2.*/
	#mfc0      $10, $16, 2
	ehb

	/*Isolate L2$ Line Size.*/
	#ext    $11, $10, 4, 4/*S_Config2SL, W_Config2SL.*/
	sll        $11, $10, 24
	srl        $11, 28

	/*Skip ahead if No L2$.*/
	beq	    $11, $0, no_l2cache
	li          $14, 2
	sllv       $11, $14, $11/*Now have true L2$ line size in bytes.*/

	/*Isolate L2$ Sets per Way.*/
	#ext     $12, $10, 8, 4/*S_Config2SS, W_Config2SS.*/
	sll         $12, $10, 20
	srl        $12, 28
	li          $14, 64
	sllv       $12, $14, $12/*L2$ Sets per way.*/

	/*Isolate L2$ Associativity.*/
	#L2$ Assoc (-1)
	#ext     $13, $10, 0, 4/*S_Config2SA, W_Config2SA.*/
	sll         $13, $10, 28
	srl        $13, 28
	add      $13, 1
	mul      $12, $12, $13/*Get total number of sets.*/
	lui        $14, 0x8000/*Get a KSeg0 address for cacheops.*/

	/*Clear L23TagLo/L23TagHi registers.*/
	MMTC0(0, 28, 4)
	ehb
	MMTC0(0, 29, 4)
	ehb

	#mtc0   $0, $28, 4
	#mtc0   $0, $29, 4
	move     $15, $12

	/*L2$ Index Store Tag Cache Op.*/
	/*Will invalidate the tag entry, clear the lock bit, and clear the LRF bit.*/
1:    
	.set  push
	.set  noreorder    
	cache    0xB, 0($14)/*SCIndexStTag.*/
	add       $15, -1/*Decrement set counter.*/
	bne       $15, $0, 1b
	add       $14, $11/*Get next line address.*/
	.set       pop 
no_l2cache:
	nop    
#else
	/* L2 bypass: set config2 bit 12 as one */
	MMFC0(8,16,2)/*0x40088002, mfc0 t0, Config2.*/
	ehb
	li          t1, 0x00001000/*Set Config2 bit 12 as one.*/
	or        t0, t1
	ehb
	MMTC0(8,16,2)/*0x40888002, mtc0 t0, Config2.*/
	ehb
#endif

        /* Watchpoint debug is disabled by default. 
        However, because MIPS 24KE/MIPS74KF processor supports up to 4 watchpoints, 
        clearance of all 4 pairs of WatchLo/WatchHi registers is required 
        if one wants to disable watch exception for assurance in initialization sequence.
        */
        MMTC0(0,18,0)/*Disable watch exception for Instruction 0.*/
        ehb/*Hazard Barrier.*/
        MMTC0(0,18,1)/*Disable watch exception for Instruction 1.*/
        ehb/*Hazard Barrier.*/
        MMTC0(0,18,2)      #Disable watch exception for Data 0.*/
        ehb/*Hazard Barrier.*/
        MMTC0(0,18,3)      #Disable watch exception for Data 1.*/
        ehb/*Hazard Barrier.*/
        li t0, 0x7
        MMTC0(8,19,0)/*Clear watch status bits.*/
        ehb/*Hazard Barrier.*/
        MMTC0(8,19,1)/*Clear watch status bits.*/
        ehb/*Hazard Barrier.*/
        MMTC0(8,19,2)/*Clear watch status bits.*/
        ehb/*Hazard Barrier.*/
        MMTC0(8,19,3)/*Clear watch status bits.*/
        ehb/*Hazard Barrier.*/
        mtc0 zero, C0_CAUSE/*Clear cause register.*/
        ehb/*Hazard Barrier.*/
        mtc0 zero, C0_COUNT/*Clear count register.*/
        ehb/*Hazard Barrier.*/

#if 1/*Reserved for debug, don't delete it.*/
	/*Output "BACK", it means that STB recovered from standby.*/
	jal     pm_uart_debug    /*Output char 'C'*/
	li      a0, 'L'

	jal     pm_uart_debug    /*Output char 'A'*/
	li      a0, '2'

	jal     pm_uart_debug    /*Output char 'E'*/
	li      a0, 'E'

	jal     pm_uart_debug    /*Output char 'N'*/
	li      a0, 'N'

	jal     pm_uart_debug    /*Output char '\n'*/
	li      a0, '\n'
	jal     pm_uart_debug    /*Output char '\r'*/
	li      a0, '\r'
#endif

	jal      _invalidate_cache/*init. inst. caches.*/
	li        a0, 0
	jal      _invalidate_cache/*init. data caches.*/
	li        a0, 1

#if 1/*Reserved for debug, don't delete it.*/
	/*Output "CACHE", it means that STB cache init done.*/
	jal     pm_uart_debug    /*Output char 'C'*/
	li      a0, 'C'

	jal     pm_uart_debug    /*Output char 'A'*/
	li      a0, 'A'

	jal     pm_uart_debug    /*Output char 'C'*/
	li      a0, 'C'

	jal     pm_uart_debug    /*Output char 'H'*/
	li      a0, 'H'

	jal     pm_uart_debug    /*Output char 'E'*/
	li      a0, 'E'

	jal     pm_uart_debug    /*Output char '\n'*/
	li      a0, '\n'
	jal     pm_uart_debug    /*Output char '\r'*/
	li      a0, '\r'
#endif

	/*remap the high memory address to memory but not flash.*/
	li        t0, 0xB8000220
	li        t1, (1<<24)
	sw     t1, 0(t0)
	li        k0, STR_CPU_REG_BACKUP
	lw      t0, 1*4(k0)
	lw      t1, 2*4(k0)
	lw      t2, 3*4(k0)
	lw      t3, 4*4(k0)
	lw      t4, 5*4(k0)
	lw      t5, 6*4(k0)
	lw      t6, 7*4(k0)
	lw      t7, 8*4(k0)
	lw      t8, 9*4(k0)
	lw      t9, 10*4(k0)
	lw      a0, 11*4(k0)
	lw      a1, 12*4(k0)
	lw      a2, 13*4(k0)
	lw      a3, 14*4(k0)
	lw      s0, 15*4(k0)
	lw      s1, 16*4(k0)
	lw      s2, 17*4(k0)
	lw      s3, 18*4(k0)
	lw      s4, 19*4(k0)
	lw      s5, 20*4(k0)
	lw      s6, 21*4(k0)
	lw      s7, 22*4(k0)
	lw      v0, 23*4(k0)
	lw      v1, 24*4(k0)
	lw      AT, 25*4(k0)
	lw      ra, 26*4(k0)
	lw      gp, 27*4(k0)
	lw      fp, 28*4(k0)
	lw      sp, 29*4(k0)

	/*You can get help form doc----UnCode-See_MIPS_Run-2nd_edition-Chinese-All-20071231.pdf
	Page48~49.*/
	lw       k1, 30*4(k0)
	MMTC0(27, 12, 0)                 /*SR.*/
	ehb

	lw       k1, 31*4(k0)
	MMTC0(27, 13, 0)                 /*Cause.*/
	ehb

	lw       k1, 32*4(k0)
	MMTC0(27, 14, 0)                 /*EPC.*/
	ehb

	lw       k1, 33*4(k0)
	MMTC0(27, 9, 0)                  /*Count.*/
	ehb

	lw       k1, 34*4(k0)
	MMTC0(27, 11, 0)                 /*Compare.*/
	ehb

	lw       k1, 35*4(k0)
	MMTC0(27, 8, 0)                  /*BadVaddr.*/
	ehb

	lw       k1, 36*4(k0)
	MMTC0(27, 4, 0)                  /*Context.*/
	ehb

	lw       k1, 37*4(k0)
	MMTC0(27, 10, 0)                 /*EntryHi.*/
	ehb

	lw       k1, 38*4(k0)
	MMTC0(27, 2, 0)                  /*EntryLo0.*/
	ehb

	lw       k1, 39*4(k0)
	MMTC0(27, 3, 0)                  /*EntryLo1.*/
	ehb

	lw       k1, 40*4(k0)
	MMTC0(27, 0, 0)                  /*Index.*/
	ehb

	lw       k1, 41*4(k0)
	MMTC0(27, 5, 0)                  /*PageMask.*/
	ehb

	lw       k1, 42*4(k0)
	MMTC0(27, 1, 0)                  /*Random.*/
	ehb

	lw       k1, 43*4(k0)
	MMTC0(27, 6, 0)                  /*Wired.*/
	ehb

	lw       k1, 44*4(k0)
	MMTC0(27, 15, 0)                 /*PRId.*/
	ehb

	lw       k1, 45*4(k0)
	MMTC0(27, 16, 0)                 /*Config.*/
	ehb

	lw       k1, 46*4(k0)
	MMTC0(27, 16, 1)                 /*Config1.*/
	ehb

	lw       k1, 47*4(k0)
	MMTC0(27, 16, 2)                 /*Config2.*/
	ehb

	lw       k1, 48*4(k0)
	MMTC0(27, 16, 3)                 /*Config3.*/
	ehb

	lw       k1, 49*4(k0)
	MMTC0(27, 15, 1)                 /*Ebase.*/
	ehb

	lw       k1, 50*4(k0)
	MMTC0(27, 12, 1)                 /*IntCtl.*/
	ehb

	lw       k1, 51*4(k0)
	MMTC0(27, 12, 2)                 /*SRSCtl.*/
	ehb

	lw       k1, 52*4(k0)
	MMTC0(27, 12, 3)                 /*SRSMap.*/
	ehb

	lw       k1, 53*4(k0)
	MMTC0(27, 27, 0)                 /*CacheERR.*/
	ehb

	lw       k1, 54*4(k0)
	MMTC0(27, 26, 0)                 /*ECC.*/
	ehb

	lw       k1, 55*4(k0)
	MMTC0(27, 30, 0)                 /*ErrorEPC.*/
	ehb

	lw       k1, 56*4(k0)
	MMTC0(27, 28, 0)                 /*TagLo.*/
	ehb

	lw       k1, 57*4(k0)
	MMTC0(27, 28, 1)                 /*DataLo.*/
	ehb

	lw       k1, 58*4(k0)
	MMTC0(27, 29, 0)                 /*TagHi.*/
	ehb

	lw       k1, 59*4(k0)
	MMTC0(27, 29, 1)                 /*DataHi.*/
	ehb

	lw       k1, 60*4(k0)
	MMTC0(27, 23, 0)                 /*Debug.*/
	ehb

	lw       k1, 61*4(k0)
	MMTC0(27, 24, 0)                 /*DEPC.*/
	ehb

	lw       k1, 62*4(k0)
	MMTC0(27, 31, 0)                 /*DESAVE.*/
	ehb

	lw       k1, 63*4(k0)
	MMTC0(27, 18, 0)                 /*WatchLo.*/
	ehb

	lw       k1, 64*4(k0)
	MMTC0(27, 19, 0)                 /*WatchHi.*/
	ehb

	lw       k1, 65*4(k0)
	MMTC0(27, 25, 0)                 /*PerfCtl.*/
	ehb

	lw       k1, 66*4(k0)
	MMTC0(27, 25, 1)                 /*PerfCnt.*/
	ehb

	lw       k1, 67*4(k0)
	MMTC0(27, 17, 0)                 /*LLAddr.*/
	ehb

	lw       k1, 68*4(k0)
	MMTC0(27, 7, 0)                  /*HWREna.*/
	ehb

	jr        ra
	nop
	.set at
END(ali_str_resume)
//============================================================================//

	.globl  _writeback_all_d_cache
	.ent    _writeback_all_d_cache
_writeback_all_d_cache:
	/*size d-cache by reading C0_CONFIG_IC*/
	.set    noreorder

	MMFC0(9,16,1)
	ehb
	andi       t6, t1, CFG_DC_MASK
	srl         t6, 13
	addiu     t6, t6, 10/*DC size=12+DC, divided by 2 for 2-way set.*/

	andi       t7, t1, CFG_DB_MASK
	srl         t7, 10
	addi       t7, 1/*line size=2^(DB+4).*/

	li           v0, 1
	sllv        t9, v0, t6/*t9= dcache set size in unit of byte.*/
	sub       t6, t6, t7
	sllv        t6, v0, t6/*t6= dcache set size in unit of d-cache line.*/
	sllv        t7, v0, t7/*t7= dcache line size in unit of byte.*/
	sll          v0, t9, 1/*return _cache_size.*/
	move     v1, t7
	add       t9, t9, t2/*0x80000000+_dcache_set_size.*/

	/*write back d-cache*/
2:
	addi       t6, -1

	cache    0x1, 0x0(t2)/*Index_Write_Back_Invalidate set0.*/
	cache    0x1, 0x0(t9)/*Index_Write_Back_Invalidate set1.*/

	add       t2, t7/*increment addr. by line size.*/
	add       t9, t7

	bgtz      t6, 2b/*loop until all cache invalidated.*/
	nop

	jr ra
	nop
	.set    reorder
	.end    _writeback_all_d_cache

#--------------------------------------------------------------------
#   Function_Name: ali_cache_flush_all
#--------------------------------------------------------------------
	.globl  ali_cache_flush_all
	.ent    ali_cache_flush_all
ali_cache_flush_all:
	li            t0, 0x80000000
	addiu      t1, t0, 0x8000/*Start addess + dcache_set_size.*/
	move      t2, t1
	nop

FLUSH_ALL_D:
	.set mips3
	cache     Index_Writeback_Inv_D, 0x0(t0)
	.set mips1
	addiu      t0, 32
	nop
	blt          t0, t2, FLUSH_ALL_D
	nop

	.set mips3        
	sync
	.set mips1
	nop

#ifdef CONFIG_MIPS_CPU_SCACHE
	li            t0, 0x80000000
	addiu      t1, t0, 0x8000/*Start addess + dcache_set_size.*/
	move      t2, t1

FLUSH_ALL_SD:
	.set mips3
	cache     Index_Writeback_Inv_SD, 0x0(t0)
	.set mips1
	addiu      t0, 32
	blt          t0, t2, FLUSH_ALL_SD
	nop

	.set mips3        
	sync
	.set mips1
	nop
#endif

	jr         ra
	nop
	.end    ali_cache_flush_all

/*
*void pm_uart_debug(UINT32 data)
*a0: print data
*print debug information
*be careful: the value of t0 will be corrupted since wen don't have stack to use now
*/
    .globl  pm_uart_debug
    .ent    pm_uart_debug
pm_uart_debug:
	/*delay some time to wait for uart output.*/
	li              t1, 0x4000
1:
	subu         t1, 1
	nop
	bgez         t1, 1b
	nop

	lui            t0, 0xB801
	ori            t0, 0x8300
	sb            a0, 0(t0)
	nop

	/*delay some time to wait for uart output.*/
	li              t1, 0x4000
2:
	subu         t1, 1
	nop
	bgez         t1, 2b
	nop

	jr             ra
	nop
	.end	pm_uart_debug

 /*-------------------------------------------------------------------
**_invalidate_cache: invalidates the given cache
*/
	.align  2
	.globl  _invalidate_cache
	.ent    _invalidate_cache
_invalidate_cache:
	/*transition to kseg1 from undetermined kernel segment.*/
	la         t1, 1f
	or        t1, 0xa0000000
	jr         t1
	nop

1:
 	/*Determine how big the I$ is.*/
	.word    0x400a8001/*mfc0 $10, $16, 1.*/
	ehb
	bnez     a0, 10f/*if not i-cache, invalidate d-cache.*/
	nop

	/*Isolate I$ Line Size.*/
	sll         $11, $10, 10
	srl         $11, 29

	/*Skip ahead if No I$.*/
	beq       $11, $0, 10f
	nop

	li          $14, 2
	sllv       $11, $14, $11/*Now have true I$ line size in bytes.*/

	sll        $12, $10, 7
	srl        $12, 29
	li          $14, 64
	sllv       $12, $14, $12/*I$ Sets per way.*/

	sll         $13, $10, 13
	srl        $13, 29/*I$ Assoc (-1).*/
	add      $13, 1
	mul      $12, $12, $13/*Total number of sets.*/

	lui        $14, 0x8000/*Get a KSeg0 address for cacheops.*/

	/*Clear TagLo/TagHi registers.*/
	mtc0     $0, $28
    	ehb
	mtc0     $0, $29
    	ehb

	move     $15, $12

	/*Index Store Tag Cache Op
	# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit.*/
1:
	.set mips3
	cache      0x8, 0($14)
	.set mips1
	addiu       $15, -1/*Decrement set counter.*/

	bne         $15, $0, 1b
	add         $14, $11/*Get next line address.*/

	/*Now go through and invalidate the D$
	# Now that the I$ has been flushed, the rest of the code can be
	# moved to kseg0 and run from the cache to go faster.*/
10:	
	addi         a0,-1
	bnez        a0,10f
	
	/*Isolate D$ Line Size.*/
	sll           $11, $10, 19
	srl           $11, 29

	/*Skip ahead if No D$.*/
	beq         $11, $0, 10f
	nop

	li            $14, 2
	sllv         $11, $14, $11/*Now have true D$ line size in bytes.*/

	sll          $12, $10, 16
	srl          $12, 29
	li            $14, 64
	sllv         $12, $14, $12/*D$ Sets per way.*/

	sll          $13, $10, 22
	srl         $13, 29/*D$ Assoc (-1).*/
	add       $13, 1

	mul       $12, $12, $13/*Get total number of sets.*/
	
	lui          $14, 0x8000/*Get a KSeg0 address for cacheops.*/

	/*Clear TagLo/TagHi registers.*/
	mtc0       $0, $28
    	ehb
	mtc0       $0, $29
    	ehb
	#mtc0     $0, $28, 2
	MMTC0(0,28,2)
    	ehb
	#mtc0     $0, $29, 2
	MMTC0(0,29,2)
    	ehb
	move       $15, $12

	/*Index Store Tag Cache Op
	# Will invalidate the tag entry, clear the lock bit, and clear the LRF bit.*/
1:
	.set mips3
	cache       0x9, 0($14)
	.set mips1
	addiu        $15, -1/*Decrement set counter.*/

	bne          $15, $0, 1b
	add          $14, $11/*Get next line address.*/

10:
	jr             ra
	nop
	.end    _invalidate_cache

/*
*void clear_C0_EBase(void)
*clear EBASE reg value.
*/
    .globl  clear_C0_EBase
    .ent    clear_C0_EBase
clear_C0_EBase:
	move         v1, ra

	li              t0, 0x0
	MMTC0(8, 15, 1)
	ehb
	nop

	move         ra, v1
	jr              ra
	nop
	nop
	.end clear_C0_EBase


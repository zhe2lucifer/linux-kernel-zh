/**
* Copyright (c) 2011,Ali Corp.
* All rights reserved.
*
* FileName     : m39_sleep.S
* Verison      : 1.0
* Author       : Chuhua Tang/Eric Cai
* Date         : 2016-04-01
* Description  : The file is to make standby to support ddr self refresh.
*/
#include <linux/linkage.h>
#include <asm/smp_scu.h>
#include <asm/memory.h>
#include <asm/asm-offsets.h>
#include <asm/hardware/cache-l2x0.h>
#include "m3921_sleep.h"
//==========================================================================================//

#define ALI_STR_DEBUG
#ifndef L2X0_CTRL_EN
#define L2X0_CTRL_EN	1
#endif
//==========================================================================================//

#ifdef CONFIG_CACHE_L2X0
	.globl	l2x0_saved_regs_addr
l2x0_saved_regs_addr:
	.long	0
#endif

	.globl	saved_reg_110
saved_reg_110:
	.long	0

	.globl	secondary_starup_phys_addr
secondary_starup_phys_addr:
	.long	0

#ifdef CONFIG_CACHE_L2X0
.macro l2_cache_resume, tmp1, tmp2, tmp3, phys_l2x0_saved_regs
	W(adr)	\tmp1, \phys_l2x0_saved_regs
	ldr	\tmp1, [\tmp1]
	ldr	\tmp2, [\tmp1, #L2X0_R_PHY_BASE]
	ldr	\tmp3, [\tmp2, #L2X0_CTRL]
	tst	\tmp3, #L2X0_CTRL_EN
	bne	exit_l2_resume
	ldr	\tmp3, [\tmp1, #L2X0_R_TAG_LATENCY]
	str	\tmp3, [\tmp2, #L2X0_TAG_LATENCY_CTRL]
	ldr	\tmp3, [\tmp1, #L2X0_R_DATA_LATENCY]
	str	\tmp3, [\tmp2, #L2X0_DATA_LATENCY_CTRL]
	ldr	\tmp3, [\tmp1, #L2X0_R_PREFETCH_CTRL]
	str	\tmp3, [\tmp2, #L2X0_PREFETCH_CTRL]
	ldr	\tmp3, [\tmp1, #L2X0_R_PWR_CTRL]
	str	\tmp3, [\tmp2, #L2X0_POWER_CTRL]
	ldr	\tmp3, [\tmp1, #L2X0_R_AUX_CTRL]
	str	\tmp3, [\tmp2, #L2X0_AUX_CTRL]
	mov	\tmp3, #0xff
	str	\tmp3, [\tmp2, #L2X0_INV_WAY]

inv_wait:
	ldr	\tmp3, [\tmp2, #L2X0_INV_WAY]
	mov	\tmp1, #0xff
	ands	\tmp3, \tmp3, \tmp1
	bne	inv_wait
	mov	\tmp3, #0x0
	str	\tmp3, [\tmp2, #L2X0_CACHE_SYNC]

wait_sync:
	ldr	\tmp3, [\tmp2, #L2X0_CACHE_SYNC]
	ands	\tmp3, \tmp3, #0x1
	bne	wait_sync
	mov	\tmp3, #L2X0_CTRL_EN
	str	\tmp3, [\tmp2, #L2X0_CTRL]

exit_l2_resume:
.endm
#else /* CONFIG_CACHE_L2X0 */
.macro l2_cache_resume, tmp1, tmp2, tmp3, phys_l2x0_saved_regs
.endm
#endif /* CONFIG_CACHE_L2X0 */

/* loads a 32-bit value into a register without a data access */
.macro mov32, reg, val
	movw	\reg, #:lower16:\val
	movt	\reg, #:upper16:\val
.endm

.macro	DO_SMC
	dsb
//	smc	#0
	dsb
.endm

/*
 * =============================
 * == CPU suspend finisher ==
 * =============================
 *
 * void omap4_finish_suspend(unsigned long cpu_state)
 *
 * This function code saves the CPU context and performs the CPU
 * power down sequence. Calling WFI effectively changes the CPU
 * power domains states to the desired target power state.
 *
 * @cpu_state : contains context save state (r0)
 *	0 - No context lost
 * 	1 - CPUx L1 and logic lost: MPUSS CSWR
 * 	2 - CPUx L1 and logic lost + GIC lost: MPUSS OSWR
 *	3 - CPUx L1 and logic lost + GIC + L2 lost: MPUSS OFF
 * @return: This function never returns for CPU OFF and DORMANT power states.
 * Post WFI, CPU transitions to DORMANT or OFF power state and on wake-up
 * from this follows a full CPU reset path via ROM code to CPU restore code.
 * The restore function pointer is stored at CPUx_WAKEUP_NS_PA_ADDR_OFFSET.
 * It returns to the caller for CPU INACTIVE and ON power states or in case
 * CPU failed to transition to targeted OFF/DORMANT state.
 */
ENTRY(ali_3921_finish_suspend)
	stmfd	sp!, {lr}

	bl	v7_flush_dcache_all
	/*
	 * Clear the SCTLR.C bit to prevent further data cache
	 * allocation. Clearing SCTLR.C would make all the data accesses
	 * strongly ordered and would not hit the cache.
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 2)		@ Disable the C bit
	mcr	p15, 0, r0, c1, c0, 0
	isb

	/*
	 * Invalidate L1 data cache. Even though only invalidate is
	 * necessary exported flush API is used here. Doing clean
	 * on already clean cache would be almost NOP.
	 */
	bl	v7_flush_dcache_all

	/*
	 * Switch the CPU from Symmetric Multiprocessing (SMP) mode
	 * to AsymmetricMultiprocessing (AMP) mode by programming
	 * the SCU power status to DORMANT or OFF mode.
	 * This enables the CPU to be taken out of coherency by
	 * preventing the CPU from receiving cache, TLB, or BTB
	 * maintenance operations broadcast by other CPUs in the cluster.
	 */
	mrc	p15, 0, r0, c1, c1, 2		@ Read NSACR data
	tst	r0, #(1 << 18)
	mrcne	p15, 0, r0, c1, c0, 1
	bicne	r0, r0, #(1 << 6)		@ Disable SMP bit
	mcrne	p15, 0, r0, c1, c0, 1
	isb
	dsb
	/*
	 * Clean and invalidate the L2 cache.
	 * Common cache-l2x0.c functions can't be used here since it
	 * uses spinlocks. We are out of coherency here with data cache
	 * disabled. The spinlock implementation uses exclusive load/store
	 * instruction which can fail without data cache being enabled.
	 * OMAP4 hardware doesn't support exclusive monitor which can
	 * overcome exclusive access issue. Because of this, CPU can
	 * lead to deadlock.
	 */
	bl	ali_m3921_get_l2cache_base
	mov	r2, r0
	ldr	r0, =0xffff
	str	r0, [r2, #L2X0_CLEAN_INV_WAY]
wait:
	ldr	r0, [r2, #L2X0_CLEAN_INV_WAY]
	ldr	r1, =0xffff
	ands	r0, r0, r1
	bne	wait
l2x_sync:
	bl	ali_m3921_get_l2cache_base
	mov	r2, r0
	mov	r0, #0x0
	str	r0, [r2, #L2X0_CACHE_SYNC]
sync:
	ldr	r0, [r2, #L2X0_CACHE_SYNC]
	ands	r0, r0, #0x1
	bne	sync

do_WFI:
	bl	ali_3921_do_suspend

	/*
	 * CPU is here when it failed to enter OFF/DORMANT or
	 * no low power state was attempted.
	 */
	mrc	p15, 0, r0, c1, c0, 0
	tst	r0, #(1 << 2)			@ Check C bit enabled?
	orreq	r0, r0, #(1 << 2)		@ Enable the C bit
	mcreq	p15, 0, r0, c1, c0, 0
	isb

	/*
	 * Ensure the CPU power state is set to NORMAL in
	 * SCU power state so that CPU is back in coherency.
	 * In non-coherent mode CPU can lock-up and lead to
	 * system deadlock.
	 */
	mrc	p15, 0, r0, c1, c0, 1
	tst	r0, #(1 << 6)			@ Check SMP bit enabled?
	orreq	r0, r0, #(1 << 6)
	mcreq	p15, 0, r0, c1, c0, 1
	isb
	mov	r0, #SCU_PM_NORMAL
	mov	r1, #0x00
	stmfd   r13!, {r4-r12, r14}
//	ldr	r12, =OMAP4_MON_SCU_PWR_INDEX
	DO_SMC
	ldmfd   r13!, {r4-r12, r14}
	isb
	dsb
	ldmfd	sp!, {pc}
ENDPROC(ali_3921_finish_suspend)


ENTRY(ali_3921_do_suspend)
	//stmfd	sp!, {lr}

	/*
	 * Execute an ISB instruction to ensure that all of the
	 * CP15 register changes have been committed.
	 */
	isb

	/*
	 * Execute a barrier instruction to ensure that all cache,
	 * TLB and branch predictor maintenance operations issued
	 * by any CPU in the cluster have completed.
	 */
	dsb
	dmb

	/*disable mmu.*/
	dsb
	mrc p15, 0, r1, c1, c0, 0
	bic r1, r1, #0x1
	mcr p15, 0, r1, c1, c0, 0

	movw	r0,0x5000
	movt	r0,0x1805

	/*jump to physical address.*/
	mov pc, r0
	nop
ENDPROC(ali_3921_do_suspend)

ENTRY(ali_3921_resume)
#if 0
	dsb
	/*disable mmu.*/
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x1
	mcr p15, 0, r0, c1, c0, 0
	dsb

	/*invalidate L1 icache, tlb, bp array.*/
	mov r0, #0                
	mcr p15, 0, r0, c8, c7, 0   @ invalidate TLB
	mcr p15, 0, r0, c7, c5, 0   @ invalidate icache
	mcr p15, 0, r0, c7, c5, 6   @ invalidate BP array
	dsb

	/*disable cache, alignment check, high vector base.*/
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x00003000     @ clear bits 13 (-VI-)
	bic r0, r0, #0x00000007     @ clear bits 2:0 (-CAM)
	mcr p15, 0, r0, c1, c0, 0
	dsb
#endif

	mrc       p15, 0, r5, c0, c0, 5 @ Read Multiprocessor Affinity Register
	and       r5, r5, #0x3 @ Extract CPU ID bits
	cmp     r5, #1
	bne       reset_core1

	/*
	* Restore reset handler and wait for boot cpu to wake
	* core 1 and lead it to secondary_startup whose address
	* stored in 0x18000054.
	*/
	mov32   r0, 0x80000000
	mov32   r1, 0x18054020
	ldmia     r1, {r4-r7}
	stmia     r0, {r4-r7}

	/*Enable Gic to make sure that Core1 can get Gic interrupt from core0.*/
	mov32   r0, 0x1bf00100
	mov      r1, #1
	str       r1, [r0], #0
	add      r0, r0, #4
	mov      r1, #0xf0
	str       r1, [r0], #0
	
	movw    r0, #0
	movw    r1, #0xf34
	movw    r2, #0
	
	nop
	nop
	dsb
	wfi 
	nop
	nop
	nop
	nop

	adr       r1, secondary_starup_phys_addr
	ldr        r1, [r1]
	mov      pc, r1

reset_core1:
        /*
	 * Since security bootrom jumps to this function directly, 
	 * and it can't disable itsself, so just disbale bootrom here.
	 */	 
	mov32   r0, 0x1a000000
	movw    r1, #0x01
	strb      r1, [r0]
	dsb

	/*
	* Disable BOOTROM SRAM here, it is unsafe if ali_3921_resume located at 0x200000 
	*/	
	mov32   r0, 0x18000110
	mov32   r1, 0x18054010
	ldr        r1, [r1]
	str        r1, [r0]
	dsb

	/*
	* It is necessary for as bootrom, otherwise page table will be corrputed by l1 cache
	* writeback
	*/
	bl         ali_v7_invalidate_l1
		
	mov32   r2, 0x00100000
1:
	subs	     r2, r2, #0x1
	bne	     1b

	/*
	* Install core 1 reset handler to physical memory zero,
	* then reset core 1 and wait for some time.
	*/	
	mov32   r0, 0x80000000
	mov32   r1, 0x18054020
	ldmia     r0, {r4-r7}
	stmia     r1, {r4-r7}
	adr       r1, core1_reset_handler
	ldmia     r1, {r4-r7}
	stmia     r0, {r4-r7}

	/*Reset core1.*/
	mov32   r0, 0x18000250
	ldr        r1, [r0]
	orr        r1, r1, #0x40
	str       r1, [r0]
	dsb
	bic       r1, r1, #0x40
	str       r1, [r0]
	dsb

	/*Security boorom keep core1 reset, so clear it.*/
	mov32  r0, 0x18000220
	ldr        r1, [r0]
	bic       r1, #0x10
	str       r1, [r0]
	dsb

	mov32  r0, 0x00004000
1:
	subs     r0, r0, #1
	bne      1b
	b         core1_reset_exit

core1_reset_handler:
	mov32   r0, 0x1805400c
	ldr        r1, [r0]
	mov      pc, r1
	isb

core1_reset_exit:
	/*Enable SCU.*/
	bl         ali_m3921_get_per_base
	ldr        r1, [r0]
	orr        r1, r1, #1
	str       r1, [r0]

	/*Set L2 cache clock.*/
	mov32   r0, 0x18000094
	ldr        r1, [r0]
	bic        r1, r1, #0xc000000
	orr        r1, r1, #0x8000000 @ L2_TCLK_RATIO_10
	str        r1, [r0]
	ldr        r1, [r0]
	bic       r1, r1, #0x3000000
	orr       r1, r1, #0x2000000 @ L2_DCLK_RATIO_10
	str       r1, [r0]

	/*L2 cache resume & re-enable.*/
	l2_cache_resume r0, r1, r2, l2x0_saved_regs_addr

	nop
	nop
	nop

	b          cpu_resume	
ENDPROC(ali_3921_resume)
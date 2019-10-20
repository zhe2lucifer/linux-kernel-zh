 #--------------------------------------------------------------------
#
#  Copyright (C) 2005 Acer Laboratories Inc.  All Rights Reserved.
#
#  File:   chip.S
#
#  Contents: About chip support function. Those function should running in RAM.
#
#  History:
#      Date          By         Reason
#   ===========    =======   ========================
#   1/17/2006      Justin Wu     Split from boot.S
#--------------------------------------------------------------------

#include "mips.h"
#include "machine.h"
#include <alidefinition/adf_sysdef.h>


#ifndef CONFIG_ARM


# -----------------------------///////////------------------------------------

#define MMFC0(rt,rd,sel)		\
	 .word	 0x40000000|(rt<<16)|(rd<<11)|sel
 
#define MMTC0(rt,rd,sel)		\
	 .word	 0x40800000|(rt<<16)|(rd<<11)|sel

	
 # --------------------------------------------------------------------
 #
 #   Function_Name: read_CP0_config2
 #   Description: Read CP0 reigster -- config2
 #   Arguments: None.
 #   Return Value: the value of config2 register
 #
 # --------------------------------------------------------------------
	.globl	read_CP0_config2
	.ent		read_CP0_config2
	
read_CP0_config2:
#mfc0	v0, C0_COUNT
	MMFC0(2,16,2)	 
	#HB
	nop
	nop
	nop
	nop

	jr		ra
	nop

	.end	read_CP0_config2	
#endif	


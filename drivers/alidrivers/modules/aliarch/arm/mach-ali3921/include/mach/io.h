/*
 * arch/arm/mach-ali3921/include/mach/io.h
 */

#ifndef __MACH_ALI_S3921_IO_H
#define __MACH_ALI_S3921_IO_H

#define IO_SPACE_LIMIT 0xffffffff

#ifndef __ASSEMBLER__
static inline void __iomem *__io(unsigned long addr)
{
	return (void __iomem *)addr;
}
#define __io(a)         __io(a)

#endif

#endif

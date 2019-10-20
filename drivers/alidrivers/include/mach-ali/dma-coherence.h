/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Ralf Baechle <ralf@linux-mips.org>
 *                     Deji Aribuki <deji.aribuki@alitech.com>
 *                     Christian Ruppert <christian.ruppert@alitech.com>
 *
 */
#ifndef __ASM_MACH_GENERIC_DMA_COHERENCE_H
#define __ASM_MACH_GENERIC_DMA_COHERENCE_H

#if defined(CONFIG_ALI_CHIP_CAP210)
	#define ALI_MAIN_MEMORY_MASK  0x7fffffff 
	#define ALI_LOW_MEMORY_LIMIT  0X17ffffff
	#define ALI_HIGH_MEMORY_OFFS  0x80000000 
#elif defined(CONFIG_ALI_M3505)
	#define ALI_MAIN_MEMORY_MASK  0x1fffffff 
	#define ALI_LOW_MEMORY_LIMIT  0X17ffffff
	#define ALI_HIGH_MEMORY_OFFS  0x80000000 
#elif defined(CONFIG_ALI_CHIP_M823)
	#define ALI_MAIN_MEMORY_MASK  0x1fffffff 
	#define ALI_LOW_MEMORY_LIMIT  0X17ffffff
	#define ALI_HIGH_MEMORY_OFFS  0x20000000  
#else
	#define ALI_MAIN_MEMORY_MASK  0x1fffffff 
	#define ALI_LOW_MEMORY_LIMIT  0X17ffffff
	#define ALI_HIGH_MEMORY_OFFS  0x20000000
#endif

                                                      
struct device;

static inline dma_addr_t plat_map_dma_mem(struct device *dev, void *addr,                          
		size_t size)                                                                                      
{                                                                                                  
	return virt_to_phys(addr) & ALI_MAIN_MEMORY_MASK;                                              
}                                                                                                  

static inline dma_addr_t plat_map_dma_mem_page(struct device *dev,                                 
		struct page *page)                                                                                
{                                                                                                  
	return page_to_phys(page) & ALI_MAIN_MEMORY_MASK;                                              
}                                                                                                  

static inline unsigned long plat_dma_addr_to_phys(struct device *dev,                              
		dma_addr_t dma_addr)                                                                              
{                                                                                                  
	if (dma_addr > ALI_LOW_MEMORY_LIMIT)                                                           
		return dma_addr + ALI_HIGH_MEMORY_OFFS;                                                      
	else                                                                                              
		return dma_addr;                                                                                
}                                                                                                  

static inline void plat_unmap_dma_mem(struct device *dev, dma_addr_t dma_addr,
		size_t size, enum dma_data_direction direction)
{
}

static inline int plat_dma_supported(struct device *dev, u64 mask)
{
	/*
	 * we fall back to GFP_DMA when the mask isn't all 1s,
	 * so we can't guarantee allocations that must be
	 * within a tighter range than GFP_DMA..
	 */
	if (mask < DMA_BIT_MASK(24))
		return 0;

	return 1;
}

static inline void plat_extra_sync_for_device(struct device *dev)
{
}

static inline int plat_dma_mapping_error(struct device *dev,                                       
		dma_addr_t dma_addr)                                                                     
{                                                                                                  
	return 0;                                                                                         
}                                                                                                  

static inline int plat_device_is_coherent(struct device *dev)
{
#ifdef CONFIG_DMA_COHERENT
	return 1;
#else
	return coherentio;
#endif
}

#ifdef CONFIG_SWIOTLB                                                                              
static inline dma_addr_t phys_to_dma(struct device *dev, phys_addr_t paddr)                        
{                                                                                                  
	return paddr;                                                                                     
}                                                                                                  

static inline phys_addr_t dma_to_phys(struct device *dev, dma_addr_t daddr)                        
{                                                                                                  
	return daddr;                                                                                     
}                                                                                                  
#endif                                                                                             

#endif /* __ASM_MACH_GENERIC_DMA_COHERENCE_H */


/*********************************
 *  (C)
 *  ALi (ZH) Corporation. 2010 Copyright (C)
 *  (C)
 *  File: m36_irq.c
 *  Description:
 *  (S)
 *  History:(M)
 *	Date				Author			Comment
 *	====				======		=======
 * 0.2010.06.03			Sam			Create
 * 1.2015.04.10			Wen			Clearing
 * 2.2015.05.12			Martin			Clearing
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*
 ********************/
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
#include <linux/slab.h>
 /* INTC register offsets */
#define INTC_REG_STATUS0	0x00
#define INTC_REG_STATUS1	0x04
#define INTC_REG_EN0	0x08
#define INTC_REG_EN1	0x0c
#define ALI_CPU_IP2	(MIPS_CPU_IRQ_BASE + 2)
#define ALI_CPU_IP3	(MIPS_CPU_IRQ_BASE + 3)
#define ALI_CPU_IP4	(MIPS_CPU_IRQ_BASE + 4)
#define ALI_CPU_IP5	(MIPS_CPU_IRQ_BASE + 5)
#define ALI_CPU_IP6	(MIPS_CPU_IRQ_BASE + 6)
#define ALI_CPU_IRQ_COUNTER	(MIPS_CPU_IRQ_BASE + 7)

 /* we have 64 SoC irqs */
#define ALI_INTC_IRQ_COUNT	64
#define IRQ_GROUP 32
#define MBOX_MASK 0xf0000000
#define MBOX_IRQ0      62
#define MBOX_IRQ1      63
#define MBOX_REG_EN    0x0C
#define MBOX_REG_ST    0x07
#define MBOX_IRQ0_MASK 0x04
#define MBOX_IRQ1_MASK 0x08
#define MBOX_IRQ_MASK  0xC

struct ali_gic_state {
	void __iomem *ali_intc_membase;
	void __iomem *ali_mbox_membase;
	int irq_base;
};

#ifndef CONFIG_OF
static void __iomem *ali_intc_membase;
static void __iomem *ali_mbox_membase;
#define ALI_IRQ_BASE	8
#define ALI_IRQ2_BASE	40

#endif

static inline void ali_intc_setbit(u32 val, void *membase, unsigned reg)
{
	u32 tmpval = ioread32(membase + (reg)) | (BIT(val));
	iowrite32(tmpval, (membase + (reg)));
}

static inline void ali_intc_clrbit(u32 val, void *membase, unsigned reg)
{
	u32 tmpval = ioread32(membase + (reg)) & (~(BIT(val)));
	iowrite32(tmpval, (membase + (reg)));
}

#ifdef CONFIG_ENABLE_RPC

static inline void mbx_irq_handler(void *mbox_membase, int irq_base)
{
	unsigned long rpc_status;
	unsigned long rpc_enable;
	int irq_num = 0;
	rpc_enable = ioread32(mbox_membase + MBOX_REG_EN);
	rpc_status = ioread32(mbox_membase + MBOX_REG_ST);
	rpc_status &= MBOX_IRQ_MASK;
	if (rpc_status & MBOX_IRQ1_MASK) {
		if (rpc_enable & (1 << (MBOX_IRQ1-32)))
			irq_num = (MBOX_IRQ1+irq_base);
	}
	if (rpc_status & MBOX_IRQ0_MASK) {
		if (rpc_enable & (1 << (MBOX_IRQ0-32)))
			irq_num = (MBOX_IRQ0+irq_base);
	}
	if (irq_num == 0)
		return;
	do_IRQ(irq_num);
}
#endif

#ifdef CONFIG_OF
static void sys_irq_enable(struct irq_data *d)
{
	unsigned int irq = d->irq;
	struct irq_domain *domain = d->domain;
	struct ali_gic_state *p_gic = (struct ali_gic_state *)domain->host_data;
	irq -= p_gic->irq_base;


	if (irq >= IRQ_GROUP)
		ali_intc_setbit(irq, p_gic->ali_intc_membase, INTC_REG_EN1);
	else
		ali_intc_setbit(irq, p_gic->ali_intc_membase, INTC_REG_EN0);
}

static void sys_irq_disable(struct irq_data *d)
{
	unsigned int irq = d->irq;
	struct irq_domain *domain = d->domain;
	struct ali_gic_state *p_gic = (struct ali_gic_state *)domain->host_data;
	irq -= p_gic->irq_base;


	if (irq >= IRQ_GROUP)
		ali_intc_clrbit(irq, p_gic->ali_intc_membase, INTC_REG_EN1);
	else
		ali_intc_clrbit(irq, p_gic->ali_intc_membase, INTC_REG_EN0);
}
#else

static void sys_irq_enable(struct irq_data *d)
{
	unsigned int irq = d->irq;
	irq -= 8;


	if (irq >= IRQ_GROUP)
		ali_intc_setbit(irq, ali_intc_membase, INTC_REG_EN1);
	else
		ali_intc_setbit(irq, ali_intc_membase, INTC_REG_EN0);
}

static void sys_irq_disable(struct irq_data *d)
{
	unsigned int irq = d->irq;
	irq -= 8;


	if (irq >= IRQ_GROUP)
		ali_intc_clrbit(irq, ali_intc_membase, INTC_REG_EN1);
	else
		ali_intc_clrbit(irq, ali_intc_membase, INTC_REG_EN0);
}
#endif

static unsigned int sys_irq_startup(struct irq_data *d)
{
	sys_irq_enable(d);
	return 0;
}
static void sys_irq_end(struct irq_data *d)
{
	unsigned int mask = IRQD_IRQ_DISABLED | IRQD_IRQ_INPROGRESS;
	if (!(d->state_use_accessors & mask))
		sys_irq_enable(d);
}

#ifdef CONFIG_OF
static void ali_intc_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	u32 intc_st0, intc_st1;
	unsigned long intc_en0, intc_en1;
	struct irq_domain *domain = irq_get_handler_data(irq);
	struct ali_gic_state *p_gic = (struct ali_gic_state *)domain->host_data;

	intc_st0 = ioread32(p_gic->ali_intc_membase + INTC_REG_STATUS0);
	intc_st1 = ioread32(p_gic->ali_intc_membase + INTC_REG_STATUS1);
	intc_en0 = ioread32(p_gic->ali_intc_membase + INTC_REG_EN0);
	intc_en1 = ioread32(p_gic->ali_intc_membase + INTC_REG_EN1);
	intc_st0 &= intc_en0;
	intc_st1 &= intc_en1;
#ifdef CONFIG_ENABLE_RPC
	mbx_irq_handler(p_gic->ali_mbox_membase, p_gic->irq_base);
#endif
	if (intc_st0 > 0)
		do_IRQ(irq_find_mapping(domain,
			(int)(p_gic->irq_base+__ffs(intc_st0))));

	if (intc_st1 > 0)
		do_IRQ(irq_find_mapping(domain,
			(int)(p_gic->irq_base + IRQ_GROUP + __ffs(intc_st1))));
}
#else
static void sys_irqdispatch(void)
{
	unsigned long intc0_req1, intc0_req2;
	unsigned long intc0_msk1, intc0_msk2;
	unsigned int bit;
	int i = 0;
	intc0_req1 = ioread32(ali_intc_membase + INTC_REG_STATUS0);
	intc0_req2 = ioread32(ali_intc_membase + INTC_REG_STATUS1);
	intc0_msk1 = ioread32(ali_intc_membase + INTC_REG_EN0);
	intc0_msk2 = ioread32(ali_intc_membase + INTC_REG_EN1);
	intc0_req1 &= intc0_msk1;
	intc0_req2 &= intc0_msk2;
#ifdef CONFIG_ENABLE_RPC
		mbx_irq_handler(ali_mbox_membase, 8);
#endif
	if (intc0_req1 > 0)	{
		bit = ALI_IRQ_BASE;
		for (i = 0; i < IRQ_GROUP; i++, bit++) {
			if (intc0_req1 & (1<<i))
				do_IRQ(bit);
		}
	}
	if (intc0_req2 > 0)	{
		bit = ALI_IRQ_BASE + IRQ_GROUP;
		for (i = 0; i < IRQ_GROUP; i++, bit++) {
			if (intc0_req2 & (1<<i))
				do_IRQ(bit);
		}
	}
}
#endif

static struct irq_chip sys_irq_controller = {
	.name		= "M36_sys_irq",
	.irq_ack	= sys_irq_disable,
	.irq_startup	= sys_irq_startup,
	.irq_shutdown	= sys_irq_disable,
	.irq_enable	= sys_irq_enable,
	.irq_disable	= sys_irq_disable,
	.irq_eoi	= sys_irq_end,
};

asmlinkage void plat_irq_dispatch(void)
{
	unsigned int pending = 0;
	pending = read_c0_status() & read_c0_cause();
	if (pending & CAUSEF_IP7)
		do_IRQ(ALI_CPU_IRQ_COUNTER);
	else if (pending & STATUSF_IP2)
		do_IRQ(ALI_CPU_IP2);
	else if ((pending & CAUSEF_IP3))
#ifdef CONFIG_OF
		do_IRQ(ALI_CPU_IP3);
#else
		sys_irqdispatch();
#endif
	else if (pending & STATUSF_IP4)
		do_IRQ(ALI_CPU_IP4);
	else if (pending & STATUSF_IP5)
		do_IRQ(ALI_CPU_IP5);
	else if (pending & STATUSF_IP6)
		do_IRQ(ALI_CPU_IP6);
	else
		spurious_interrupt();
}

#ifdef CONFIG_OF
static int intc_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	irq_set_chip_and_handler(irq, &sys_irq_controller, handle_percpu_irq);
	return 0;
}


static const struct irq_domain_ops irq_domain_ops = {
	.xlate = irq_domain_xlate_onecell,
	.map = intc_map,
};

static int __init intc_of_init(struct device_node *node,
			       struct device_node *parent)
{
	struct resource res;
	struct resource res_mbox;
	struct irq_domain *domain;
	int irq;
	int irq_base;
	struct ali_gic_state *p_ali_gic;
	void __iomem *ali_intc_membase;
	void __iomem *ali_mbox_membase;

	p_ali_gic = kmalloc(sizeof(struct ali_gic_state), GFP_KERNEL);
	if (p_ali_gic == NULL)
		return -ENOMEM;
	if (of_property_read_u32(node, "irq_base", &irq_base))
		panic("Failed to get INTC irq_base");

	irq = irq_of_parse_and_map(node, 0);
	if (!irq)
		panic("Failed to get INTC IRQ");
	if (of_address_to_resource(node, 0, &res))
		panic("Failed to get intc memory range");
	if (request_mem_region(res.start, resource_size(&res),
			res.name) == NULL)
		panic("Failed to remap intc memory");
	ali_intc_membase = ioremap_nocache(res.start,
					resource_size(&res));
	if (!ali_intc_membase)
		panic("Failed to remap intc memory");
	if (of_address_to_resource(node, 1, &res_mbox))
		panic("Failed to get res_mbox memory range");
	if (request_mem_region(res_mbox.start, resource_size(&res_mbox),
				res_mbox.name) == NULL)
		panic("Failed to remap intc memory");
	ali_mbox_membase = ioremap_nocache(res_mbox.start,
					resource_size(&res_mbox));
	if (!ali_mbox_membase)
		panic("Failed to remap mbox memory");

	p_ali_gic->irq_base = irq_base;
	p_ali_gic->ali_mbox_membase = ali_mbox_membase;
	p_ali_gic->ali_intc_membase = ali_intc_membase;
	/* disable all interrupts */
	iowrite32(0, ali_intc_membase + INTC_REG_EN0);
	iowrite32(0, ali_intc_membase + INTC_REG_EN1);

	write_c0_status(read_c0_status() | STATUSF_IP3);
	domain = irq_domain_add_legacy(node, ALI_INTC_IRQ_COUNT,
			irq_base, irq_base, &irq_domain_ops, (void *)p_ali_gic);
	irq_set_chained_handler(irq, ali_intc_irq_handler);
	return irq_set_handler_data(irq, domain);
}

static const struct of_device_id of_irq_ids[] __initdata = {
	{ .compatible = "mips,cpu-interrupt-controller",
		.data = mips_cpu_intc_init },
	{ .compatible = "alitech,generic-intc", .data = intc_of_init },
	{},
};
void __init arch_init_irq(void)
{
	of_irq_init(of_irq_ids);
}
#else

void __init arch_init_irq(void)
{

	int i = 0;
	ali_intc_membase = (void *)0xB8000030;
	ali_mbox_membase = (void *)0xB8040030;

	mips_cpu_irq_init();
	/* init sys irqs */
	for (i = 8; i < 72; i++)
		irq_set_chip_and_handler(i,
			&sys_irq_controller, handle_percpu_irq);

	/* disable all interrupts */
	iowrite32(0, ali_intc_membase + INTC_REG_EN0);
	iowrite32(0, ali_intc_membase + INTC_REG_EN1);

	write_c0_status(read_c0_status() | STATUSF_IP3);
}


#endif

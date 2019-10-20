#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>

int gpio_enable_pin(int gpio)
{
#if 0
	unsigned long flags;

	if (!gpio_is_valid(number))
		return -1;

//    HAL_PINMUX_GPIO_ENABLE(number);
	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_FUNC_ENABLE(number, HAL_GPIO_ENABLE);

	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
#else
	//TODO
	int val;
	if (!gpio_is_valid(gpio))
		return -1;
	printk(KERN_INFO "Enter [ %s %d gpio = %d]\n", __FUNCTION__, __LINE__, gpio);
	val = gpio_request(gpio, NULL);

	return val;
	//return 0;
#endif
}

EXPORT_SYMBOL(gpio_enable_pin);

int __init m36_init_gpio(void)
{
#if 0
	return gpiochip_add(&m36_gpio_chip);
#else
	//TODO
	return 0;
#endif
}

EXPORT_SYMBOL(m36_init_gpio);

int ali_gpio_get_value(unsigned char gpio)
{				//TODO
#if 0
	int val;
	unsigned long flags;

	if (!gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio)) {
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	} else {
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	}

	/* GPIO can never have been requested or set as {in,out}put */
	val = HAL_GPIO_BIT_GET(gpio);

	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return val;
#else
	//TODO
	int val;
	if (!gpio_is_valid(gpio))
		return -1;

	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio)) {
		gpio_direction_output(gpio, 0);
	} else {
		gpio_direction_input(gpio);
	}

	val = gpio_get_value_cansleep(gpio);	//?
	printk(KERN_INFO "Enter [ %s %d gpio = %d,val = %d]\n", __FUNCTION__, __LINE__, gpio, val);
	return val;
#endif
}

EXPORT_SYMBOL(ali_gpio_get_value);

int ali_gpio_set_value(unsigned char gpio, int value)
{
#if 0
	unsigned long flags;

	if (!gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio)) {
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	} else {
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	}

	/* GPIO can never have been requested or set as output */
	HAL_GPIO_BIT_SET(gpio, (value) ? 1 : 0);

	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
#else
#if 0
	//TODO
	printk(KERN_INFO "Enter [ %s %d gpio = %d,]\n", __FUNCTION__, __LINE__, gpio);
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio)) {
		gpio_direction_input(gpio);
	} else {
		gpio_direction_output(gpio, (value) ? 1 : 0);
	}
#endif
	//gpio_set_value_cansleep(gpio, int value)
	return 0;
#endif
}

EXPORT_SYMBOL(ali_gpio_set_value);

int ali_gpio_direction_input(unsigned char gpio)
{
#if 0
	unsigned long flags;

	if (!gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio)) {
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	} else {
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	}

	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
#else
	//TODO
	printk(KERN_INFO "Enter [ %s %d gpio = %d]\n", __FUNCTION__, __LINE__, gpio);
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio)) {
		gpio_direction_output(gpio, 0);
	} else {
		gpio_direction_input(gpio);

	}
	return 0;
#endif
}

EXPORT_SYMBOL(ali_gpio_direction_input);

void gpio_irq_clear(unsigned offset)
{
#if 0
	unsigned long flags;

	//printk("%s->%d\n", __FUNCTION__, offset);

#ifdef ENABLE_M3701C_JTAG_DEBG
	if ((offset == 17) || (offset == 18)
	    || (offset == 4) || (offset == 1)
	    || (offset == 32))
		return;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_INT_CLEAR(offset);

	spin_unlock_irqrestore(&m36_gpio_lock, flags);
#else
	printk(KERN_INFO "Func:gpio_irq_clear is not implment \n");
#endif
}

EXPORT_SYMBOL(gpio_irq_clear);

int gpio_irq_get_status(unsigned offset)
{
#if 0
	unsigned long flags;
	int irq_status = 0;

#ifdef ENABLE_M3701C_JTAG_DEBG
	if ((offset == 17) || (offset == 18)
	    || (offset == 4) || (offset == 1)
	    || (offset == 32))
		return 0;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);
	irq_status = HAL_GPIO_INT_STA_GET(offset);
	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return irq_status;
#else
	return 0;
#endif
}

EXPORT_SYMBOL(gpio_irq_get_status);

#if 0
int gpio_to_irq(unsigned gpio)
{
#if 0
	return INT_ALI_GPIO;
#else
	return 0;
#endif
}

EXPORT_SYMBOL(gpio_to_irq);
#endif

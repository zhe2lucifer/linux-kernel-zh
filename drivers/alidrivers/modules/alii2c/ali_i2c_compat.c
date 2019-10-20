#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <asm/errno.h>
#include <linux/platform_device.h>
#include <asm/mach-ali/typedef.h>
#include <linux/err.h>
//#include <linux/dvb/ali_i2c_scb_gpio.h>
#include <ali_i2c_scb_gpio.h>
#include <linux/i2c.h>
#if 1
int i2c_gpio_read(u32 id, u8 slv_addr, u8 *data, int len);
int i2c_gpio_write(u32 id, u8 slv_addr, u8 *data, int len);
int i2c_gpio_write_read(u32 id, u8 slv_addr, u8 *data, int wlen, int rlen);
#endif

int ali_i2c_scb_write(u32 id, u8 slv_addr, u8 *data, int len)
{
	struct i2c_adapter *adap;
	struct i2c_msg msg;
	int ret;

	adap = i2c_get_adapter(id);
	if (NULL == adap) {
		printk(KERN_ERR "[%s] get adapter fail, id: %u\n", __FUNCTION__, id);
		return -1;
	}

	msg.addr = slv_addr >> 1;
	//msg.addr = slv_addr;
	msg.len = len;
	msg.buf = data;
	msg.flags = 0;

	ret = i2c_transfer(adap, &msg, 1);
	if (1 != ret) {
		printk(KERN_ERR "[%s] i2c_transfer fail, ret: %d\n", __FUNCTION__, ret);
	}
	return (1 == ret) ? 0 : -1;
}

int ali_i2c_scb_read(u32 id, u8 slv_addr, u8 *data, int len)
{
	struct i2c_adapter *adap;
	struct i2c_msg msg;
	int ret;

	adap = i2c_get_adapter(id);
	if (NULL == adap) {
		printk(KERN_ERR "[%s] get adapter fail, id: %u\n", __FUNCTION__, id);
		return -1;
	}

	msg.addr = slv_addr >> 1;
	//msg.addr = slv_addr;
	msg.len = len;
	msg.buf = data;
	msg.flags = I2C_M_RD;

	ret = i2c_transfer(adap, &msg, 1);
	if (1 != ret) {
		printk(KERN_ERR "[%s] i2c_transfer fail, ret: %d\n", __FUNCTION__, ret);
	}
	return (1 == ret) ? 0 : -1;
}

int ali_i2c_scb_write_read(u32 id, u8 slv_addr, u8 *data, int wlen, int len)
{
	struct i2c_adapter *adap;
	struct i2c_msg msgs[2];
	int ret;

	adap = i2c_get_adapter(id);
	if (NULL == adap) {
		printk(KERN_ERR "[%s] get adapter fail, id: %u\n", __FUNCTION__, id);
		return -1;
	}

	msgs[0].addr = slv_addr >> 1;
	//msgs[0].addr = slv_addr;
	msgs[0].len = wlen;
	msgs[0].buf = data;
	msgs[0].flags = 0;

	msgs[1].addr = slv_addr >> 1;
	//msgs[1].addr = slv_addr;
	msgs[1].len = len;
	msgs[1].buf = data;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(adap, msgs, 2);
	if (2 != ret) {
		printk(KERN_ERR "[%s] i2c_transfer fail, ret: %d\n", __FUNCTION__, ret);
	} else {
		//printk("0x%x, data[0]: %x\n", (int)&msgs[1], data[0]);
	}
	return (2 == ret) ? 0 : -1;
}

int ali_i2c_read(u32 id, u8 slv_addr, u8 *data, int len)
{
    u32 id_minor = 0;
    int ret = -1;
    id_minor = id & 0x0f;
    if(id & I2C_TYPE_GPIO){ //gpio i2c
        ret = i2c_gpio_read(id_minor, slv_addr, data, len);
    }else{ //scb i2c
        ret = ali_i2c_scb_read(id_minor, slv_addr, data, len);
    } 
	return ret;
}
EXPORT_SYMBOL(ali_i2c_read);

int ali_i2c_write(u32 id, u8 slv_addr, u8 *data, int len)
{
    u32 id_minor = 0;
    int ret = -1;
    id_minor = id & 0x0f;
    if(id & I2C_TYPE_GPIO){ 
        ret = i2c_gpio_write(id_minor, slv_addr, data, len);
    }else{ 
        ret = ali_i2c_scb_write(id_minor, slv_addr, data, len);
    } 
	return ret;
}
EXPORT_SYMBOL(ali_i2c_write);

int ali_i2c_write_read(u32 id, u8 slv_addr, u8 *data, int wlen, int rlen)
{
    u32 id_minor = 0;
    int ret = -1;
    id_minor = id & 0x0f;
    if(id & I2C_TYPE_GPIO){ 
        ret = i2c_gpio_write_read(id_minor, slv_addr, data, wlen, rlen);
        //printk("Here!\n");
    }else{
        //printk("Here1!\n");
        ret = ali_i2c_scb_write_read(id_minor, slv_addr, data, wlen, rlen);
    } 
	return ret;
}
EXPORT_SYMBOL(ali_i2c_write_read);



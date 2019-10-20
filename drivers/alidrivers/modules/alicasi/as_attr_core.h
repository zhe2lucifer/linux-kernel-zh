#ifndef _AS_ATTR_CORE_H
#define _AS_ATTR_CORE_H

#include <linux/cdev.h>
#include <linux/ali_dsc.h>
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

#define MAX_AS_ATTR_DEVS	4	/* Maximum number of as attribute devices */

struct ali_casi_kl_attr {
	struct kl_key_cell *key_cell;
	int crypt_mode;
	int algo;
	int level;
};

struct ali_casi_dsc_attr {
	struct ca_session_attr dsc_ses_attr;
};

/* Device structure */
struct as_attr_device {
	char *name;
	int id;
	struct device *device;
	struct cdev cdev;
	struct mutex lock;
	unsigned long status;
	void *driver_data;
	struct ali_casi_kl_attr kl_attr;
	struct ali_casi_dsc_attr dsc_attr;
	const struct file_operations *ops;
	struct list_head list_node;
/* Bit numbers for status flags */
#define AS_ATTR_ACTIVE			0	/* Is the as_attr running/active */
#define AS_ATTR_DEV_OPEN		1	/* Opened via /dev/?#id */
#define AS_ATTR_UNREGISTERED	2	/* Has the device been unregistered */
}; 

int as_attr_register_device(struct as_attr_device *as_attr_dev);
void as_attr_unregister_device(struct as_attr_device *as_attr_dev);

#endif //_AS_ATTR_CORE_H
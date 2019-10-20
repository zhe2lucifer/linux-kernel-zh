#ifndef _CASI_DEV_H
#define _CASI_DEV_H

#include "as_attr_core.h"
#include <linux/semaphore.h>

struct ali_casi_decrypt_data {
	void *see_dev_hdl;
	__u16 stream_id;
	__u8 *input;
	__u32 data_len;
};

struct ali_casi_entry {
	struct device		*dev;
	struct semaphore sem;
	struct as_attr_device as_attr_dev;
	const struct casi_see_ops *see_ops;
	__s32 see_chan;
};



#endif //_CASI_DEV_H


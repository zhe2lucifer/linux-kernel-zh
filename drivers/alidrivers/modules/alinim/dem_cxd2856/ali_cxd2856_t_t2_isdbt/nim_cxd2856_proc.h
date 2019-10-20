#ifndef NIM_CXD2856_PROC_H
#define NIM_CXD2856_PROC_H
#include <linux/platform_device.h>
#include <linux/proc_fs.h>

	
int  nim_cxd2856_proc_init(struct platform_device * pdev);
int  nim_cxd2856_proc_exit(struct platform_device * pdev);

#endif 
